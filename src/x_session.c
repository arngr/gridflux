/*
 * This file is part of littlewin.
 *
 * littlewin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * littlewin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with littlewin.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2024 Ardi Nugraha
 */

#include "x_session.h"
#include "atoms.h"
#include "gridflux.h"
#include <X11/Xlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

const int WIN_APP_LIMIT = 8;

static Display *initialize_display() {
  int try_index = 0;
  while (1) {
    Display *display = XOpenDisplay(NULL);
    if (!display) {
      try_index++;
      LOG(GRIDFLUX_ERR, ERR_DISPLAY_NULL);
      sleep(1);
    } else {
      return display;
    }
  }
}

static Window get_root_window(Display *display) {
  return RootWindow(display, DefaultScreen(display));
}

static int error_handler(Display *display, XErrorEvent *error) {
  if (error->error_code == BadWindow) {
    LOG(GRIDFLUX_ERR, ERR_BAD_WINDOW);
  }
  return 0; // Return 0 to prevent the program from terminating
}

static int send_client_message(Display *display, Window window,
                               Atom message_type, Atom *atoms, int atom_count,
                               long *data) {
  if (message_type == None) {
    LOG(GRIDFLUX_ERR, ERR_MSG_NULL);
    return -1;
  }

  XClientMessageEvent event = {0};
  event.type = ClientMessage;
  event.window = window;
  event.message_type = message_type;
  event.format = 32;

  for (int i = 0; i < atom_count; i++) {
    event.data.l[i] = data[i];
  }

  if (!XSendEvent(display, DefaultRootWindow(display), False,
                  SubstructureRedirectMask | SubstructureNotifyMask,
                  (XEvent *)&event)) {
    LOG(GRIDFLUX_ERR, ERR_SEND_MSG_FAIL);
    return -1;
  }

  XFlush(display);
  return 0;
}

static void unmaximize_window(Display *display, Window window) {
  long data[] = {0, atoms.net_wm_max_horz,
                 atoms.net_wm_max_vert}; // Unmaximize state data
  send_client_message(display, window, atoms.wm_state, NULL, 3, data);
}

static int move_window_to_workspace(Display *display, Window window,
                                    int workspace) {
  long data[] = {workspace, CurrentTime}; // Move to target workspace
  if (send_client_message(display, window, atoms.net_wm_desktop, NULL, 2,
                          data) == 0) {
    return 0;
  }
  return 1;
}

static void get_window_dimensions(Display *display, Window window, int *width,
                                  int *height, int *x, int *y) {
  XWindowAttributes attributes;
  int status = XGetWindowAttributes(display, window, &attributes);

  if (status == 0) {
    LOG(GRIDFLUX_ERR, "Invalid window.\n");
    return;
  }

  if (x != NULL)
    *x = attributes.x;
  if (y != NULL)
    *y = attributes.y;

  if (width != NULL)
    *width = attributes.width;

  if (height != NULL)
    *height = attributes.height;
}

int is_excluded_window(Display *display, Window window) {
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;

  if (XGetWindowProperty(display, window, atoms.net_wm_state, 0, 1024, False,
                         XA_ATOM, &actual_type, &actual_format, &nitems,
                         &bytes_after, &prop) == Success &&
      prop != NULL) {

    Atom *states = (Atom *)prop;
    int result = 0;

    for (unsigned long i = 0; i < nitems; i++) {
      if (states[i] == atoms.net_wm_hidden ||
          states[i] == atoms.net_wm_notification ||
          states[i] == atoms.net_wm_popup_menu ||
          states[i] == atoms.net_wm_tooltip ||
          states[i] == atoms.net_wm_toolbar ||
          states[i] == atoms.net_wm_utility) {
        result = 1;
        break;
      }
    }

    XFree(prop);
    return result;
  }

  return 0;
}

void window_set_geometry(Display *display, Window window, int gravity,
                         unsigned long mask, int x, int y, int width,
                         int height) {
  XSizeHints hints;
  long supplied_return;

  if (gravity != 0) {
    Status status =
        XGetWMNormalHints(display, window, &hints, &supplied_return);

    if (status == 0) {
      memset(&hints, 0, sizeof(hints));
      hints.flags = 0;
    }

    hints.flags |= PWinGravity;
    hints.win_gravity = gravity;
    XSetWMNormalHints(display, window, &hints);
  }

  if (mask & (CHANGE_X | CHANGE_Y | CHANGE_WIDTH | CHANGE_HEIGHT)) {
    int current_x, current_y;
    unsigned int current_width, current_height, border_width, depth;
    Window root;

    if (mask != (CHANGE_X | CHANGE_Y | CHANGE_WIDTH | CHANGE_HEIGHT)) {
      XGetGeometry(display, window, &root, &current_x, &current_y,
                   &current_width, &current_height, &border_width, &depth);
    } else {
      current_x = current_y = 0;
      current_width = current_height = 1; // Non-zero defaults
    }

    int new_x = (mask & CHANGE_X) ? x : current_x;
    int new_y = (mask & CHANGE_Y) ? y : current_y;
    unsigned int new_width = (mask & CHANGE_WIDTH) ? width : current_width;
    unsigned int new_height = (mask & CHANGE_HEIGHT) ? height : current_height;

    XMoveResizeWindow(display, window, new_x, new_y, new_width, new_height);
  }
}

static void split_window(Window windows[], int window_count, Display *display,
                         int screen, int x, int y, int width, int height,
                         int depth) {
  if (window_count <= 0)
    return;

  if (window_count == 1) {
    window_set_geometry(display, windows[0], StaticGravity,
                        CHANGE_X | CHANGE_Y | CHANGE_WIDTH | CHANGE_HEIGHT, x,
                        y, width, height);
    return;
  }

  int split_vertically = depth % 2 == 0;
  int left_count = window_count / 2;
  int right_count = window_count - left_count;

  if (split_vertically) {
    int left_width = width / 2;
    int right_width = width - left_width;

    split_window(windows, left_count, display, screen, x, y, left_width, height,
                 depth + 1);
    split_window(windows + left_count, right_count, display, screen,
                 x + left_width, y, right_width, height, depth + 1);
  } else {
    int top_height = height / 2;
    int bottom_height = height - top_height;

    split_window(windows, left_count, display, screen, x, y, width, top_height,
                 depth + 1);
    split_window(windows + left_count, right_count, display, screen, x,
                 y + top_height, width, bottom_height, depth + 1);
  }
}

static void arrange_window(int window_count, Window windows[], Display *display,
                           int screen) {
  Screen *scr = ScreenOfDisplay(display, screen);
  int screen_width = scr->width - 5;
  int screen_height = scr->height;

  split_window(windows, window_count, display, screen, 0, 0, screen_width,
               screen_height, 0);
}

static Window *fetch_window_list(Display *display, Window root,
                                 unsigned long *nitems, Atom atom,
                                 int workspace_id) {
  Atom actual_type;
  int actual_format;
  unsigned long bytes_after;
  Window *windows = NULL;

  if (XGetWindowProperty(display, root, atom, 0, (~0L), False, XA_WINDOW,
                         &actual_type, &actual_format, nitems, &bytes_after,
                         (unsigned char **)&windows) != Success) {
    LOG(GRIDFLUX_ERR, ERR_BAD_WINDOW);
    return NULL;
  }

  if (*nitems == 0) {
    LOG(GRIDFLUX_INFO, ERR_BAD_WINDOW);
    if (windows != NULL) {
      XFree(windows);
    }
    return NULL;
  }

  Window *filtered_windows = malloc(sizeof(Window) * (*nitems));
  if (filtered_windows == NULL) {
    LOG(GRIDFLUX_ERR, ERR_FAIL_ALLOCATE);
    XFree(windows);
    return NULL;
  }

  unsigned long filtered_count = 0;

  for (unsigned long i = 0; i < *nitems; i++) {
    Window window = windows[i];

    if (is_excluded_window(display, window))
      continue;

    if (atoms.net_wm_desktop == None) {
      XFree(filtered_windows);
      XFree(windows);
      return NULL;
    }

    Atom actual_type;
    int actual_format;
    unsigned long nitems_returned, bytes_after;
    unsigned char *data = NULL;
    if (XGetWindowProperty(display, window, atoms.net_wm_desktop, 0, 1, False,
                           XA_CARDINAL, &actual_type, &actual_format,
                           &nitems_returned, &bytes_after, &data) == Success &&
        data) {
      unsigned long window_workspace_id = *(unsigned long *)data;
      XFree(data);

      if (window_workspace_id == workspace_id) {
        filtered_windows[filtered_count++] = window;
      }
    }
  }

  *nitems = filtered_count;

  if (windows != NULL) {
    XFree(windows);
  }

  return filtered_windows;
}

static unsigned long int get_current_workspace(Display *display, Window root) {
  unsigned long *desktop = NULL;
  Atom actualType;
  int actualFormat;
  unsigned long nItems, bytesAfter;

  if (XGetWindowProperty(display, root, atoms.net_curr_desktop, 0, 1, False,
                         XA_CARDINAL, &actualType, &actualFormat, &nItems,
                         &bytesAfter, (unsigned char **)&desktop) == Success &&
      desktop) {
    int workspaceNumber = (int)*desktop;
    XFree(desktop);
    return workspaceNumber;
  } else {
    LOG(GRIDFLUX_ERR, ERR_BAD_WINDOW);
    XCloseDisplay(display);
    return -1;
  }
}

static Window get_last_opened_window(Display *display) {
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *data = NULL;

  if (XGetWindowProperty(display, DefaultRootWindow(display),
                         atoms.client_list_stack, 0, (~0L), False, XA_WINDOW,
                         &actual_type, &actual_format, &nitems, &bytes_after,
                         &data) == Success &&
      data) {
    Window *stacking_list = (Window *)data;
    Window last_window =
        stacking_list[nitems - 1]; // The last window in the list
    XFree(data);
    return last_window;
  }

  return 0;
}

static unsigned long get_total_workspace(Display *display, Window root) {
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *data = NULL;
  unsigned long total_workspaces = 0;

  if (XGetWindowProperty(display, root, atoms.num_of_desktop, 0, 1, False,
                         XA_CARDINAL, &actual_type, &actual_format, &nitems,
                         &bytes_after, &data) == Success) {
    if (data) {
      total_workspaces = *(unsigned long *)data;
      XFree(data);
    } else {
      LOG(GRIDFLUX_ERR, ERR_BAD_WINDOW);
    }
  } else {
    LOG(GRIDFLUX_ERR, ERR_BAD_WINDOW);
  }

  return total_workspaces;
}

static int get_total_window(Display *display, Window root) {
  unsigned long total_win = 0;
  int total_workspaces = get_total_workspace(display, root);

  for (int i = 0; i <= total_workspaces; i++) {
    unsigned long current_window_count = 0;
    fetch_window_list(display, root, &current_window_count, atoms.client_list,
                      i);
  }

  return total_win;
}

static void arrange_dimension(Display *display, Window root,
                              unsigned long base_win_items,
                              Window *curr_win_open, win_info *base_win_info,
                              int screen) {
  for (unsigned long int i = 0; i < base_win_items; i++) {
    win_info *curr_win_info =
        (win_info *)malloc(base_win_items * sizeof(win_info));
    get_window_dimensions(display, curr_win_open[i], &curr_win_info[i].width,
                          &curr_win_info[i].height, NULL, NULL);

    if (curr_win_info[i].width != base_win_info[i].width ||
        curr_win_info[i].height != base_win_info[i].height) {
      unmaximize_window(display, curr_win_open[i]);
      arrange_window(base_win_items, curr_win_open, display, screen);

      base_win_info[i].width = curr_win_info[i].width;
      base_win_info[i].height = curr_win_info[i].height;
    }

    free(curr_win_info);
  }
}

static void distribute_overflow_windows(
    Display *display, int *overflow_workspace, int overflow_workspace_total,
    workspace_info *free_workspace, int free_workspace_total,
    Atom window_list_atom, Window root, int screen) {
  for (int i = 0; i < overflow_workspace_total; i++) {
    unsigned long current_window_count = 0;
    Window *active_windows =
        fetch_window_list(display, root, &current_window_count,
                          window_list_atom, overflow_workspace[i]);

    for (int j = 0; j <= free_workspace_total; j++) {
      for (int x = 0; x <= free_workspace[j].available_space; x++) {
        if (current_window_count >= WIN_APP_LIMIT) {
          unmaximize_window(display, active_windows[current_window_count--]);
          move_window_to_workspace(display,
                                   active_windows[current_window_count--],
                                   free_workspace[j].workspace_id);
          free_workspace[j].available_space--;
        }
      }
    }

    if (active_windows) {
      arrange_window(current_window_count, active_windows, display, screen);
    }
  }
}

static void handle_window_overflow(Display *display, Window root,
                                   unsigned long *current_window_count,
                                   Atom window_list_atom, int total_workspace,
                                   int screen) {
  int overflow_workspace[total_workspace];
  int overflow_workspace_total = 0;

  workspace_info free_workspace[total_workspace];
  int free_workspace_total = 0;

  for (int workspace = 0; workspace <= total_workspace; workspace++) {
    Window *active_windows = fetch_window_list(
        display, root, current_window_count, window_list_atom, workspace);

    if (*current_window_count >= WIN_APP_LIMIT) {
      overflow_workspace[overflow_workspace_total] = workspace;
      overflow_workspace_total++;
    } else if (*current_window_count + 1 < WIN_APP_LIMIT) {
      int available_space = WIN_APP_LIMIT - *current_window_count;
      free_workspace[free_workspace_total].workspace_id = workspace;
      free_workspace[free_workspace_total].total_window_open =
          *current_window_count;
      free_workspace[free_workspace_total].available_space = available_space;
      free_workspace_total++;
    }
  }
  distribute_overflow_windows(
      display, overflow_workspace, overflow_workspace_total, free_workspace,
      free_workspace_total, window_list_atom, root, screen);
}

static void manage_workspace_windows(Display *display, Window root,
                                     unsigned long *previous_window_count,
                                     int total_workspace, int screen) {
  unsigned long current_window_count = 0;
  Window *active_windows;

  for (int workspace = 0; workspace < total_workspace; workspace++) {
    active_windows = fetch_window_list(display, root, &current_window_count,
                                       atoms.client_list, workspace);
    if (!active_windows) {
      continue;
    }

    if (current_window_count > WIN_APP_LIMIT) {
      handle_window_overflow(display, root, &current_window_count,
                             atoms.client_list, total_workspace, screen);
    }
  }
}

static void rearrange_current_workspace(Display *display, Window root,
                                        unsigned long *previous_window_count,
                                        int screen,
                                        win_info *window_properties) {
  unsigned long current_window_count = 0;
  int current_workspace = get_current_workspace(display, root);
  Window *active_windows =
      fetch_window_list(display, root, &current_window_count, atoms.client_list,
                        current_workspace);

  if (current_window_count != *previous_window_count) {
    *previous_window_count = current_window_count;

    for (unsigned long i = 0; i <= current_window_count; i++) {
      if (active_windows[i]) {
        unmaximize_window(display, active_windows[i]);
      }
    }

    if (active_windows) {
      arrange_window(current_window_count, active_windows, display, screen);
    }
  }

  arrange_dimension(display, root, current_window_count, active_windows,
                    window_properties, screen);

  if (active_windows) {
    XFree(active_windows);
  }
}

const char *detect_desktop_environment() {
  const char *xdg_current_desktop = getenv("XDG_CURRENT_DESKTOP");
  const char *desktop_session = getenv("DESKTOP_SESSION");
  const char *kde_full_session = getenv("KDE_FULL_SESSION");
  const char *gnome_session_id = getenv("GNOME_DESKTOP_SESSION_ID");

  if (kde_full_session && strcmp(kde_full_session, "true") == 0) {
    return "KDE";
  } else if (gnome_session_id ||
             (xdg_current_desktop && strstr(xdg_current_desktop, "GNOME"))) {
    return "GNOME";
  } else if (xdg_current_desktop) {
    return xdg_current_desktop; // Return the value of XDG_CURRENT_DESKTOP
  } else if (desktop_session) {
    return desktop_session; // Return the value of DESKTOP_SESSION
  } else {
    return "Unknown";
  }
}

static void create_new_workspace(Display *display, Window root,
                                 unsigned long new_workspace) {
  Atom net_current_desktop = XInternAtom(display, "_NET_CURRENT_DESKTOP", True);

  XChangeProperty(display, root, net_current_desktop, XA_CARDINAL, 32,
                  PropModeReplace, (unsigned char *)&new_workspace, 1);
  XSync(display, False); // Ensure the action is committed
}

static void set_workspace(Display *display, Window root,
                          unsigned long workspace) {
  const char *desktop_session = detect_desktop_environment();
  if (desktop_session == NULL || strcmp(desktop_session, "Unknown") == 0) {
    LOG(GRIDFLUX_DBG, "No desktop session detected. Exiting.\n");
    return;
  }

  pid_t pid = fork(); // Create a new process

  if (pid == -1) {
    perror("Failed to fork");
    return;
  }

  if (pid == 0) {
    if (strcmp(desktop_session, "KDE") == 0) {
      if (execlp("qdbus", "qdbus", "org.kde.KWin", "/VirtualDesktopManager",
                 "createDesktop", "1", "LittleWin", NULL) == -1) {
        perror("Error executing qdbus");
        _exit(EXIT_FAILURE);
      }
    } else if (strcmp(desktop_session, "GNOME") == 0) {
      if (execlp("gsettings", "gsettings", "set", "org.gnome.mutter",
                 "dynamic-workspaces", "true", NULL) == -1) {
        perror("Error executing gsettings");
        _exit(EXIT_FAILURE);
      }
      create_new_workspace(display, root, workspace);
    } else {
      int status;
      waitpid(pid, &status, 0);
      if (WIFEXITED(status)) {
        LOG(GRIDFLUX_DBG, "command exited with status %d\n",
            WEXITSTATUS(status));
      } else {
        perror("command did not terminate normally\n");
        _exit(EXIT_FAILURE);
      }
    }
  }
}

static void manage_window(Display *display, Window root,
                          unsigned long *previous_window_count,
                          win_info *window_properties, int screen) {
  if (!display) {
    LOG(GRIDFLUX_WARN, ERR_DISPLAY_NULL);
    return;
  }

  unsigned long total_workspace = get_total_workspace(display, root);

  manage_workspace_windows(display, root, previous_window_count,
                           total_workspace, screen);
  rearrange_current_workspace(display, root, previous_window_count, screen,
                              window_properties);
}

void run_x_layout() {
  Display *display = initialize_display();
  if (!display) {
    LOG(GRIDFLUX_ERR, ERR_DISPLAY_NULL);
    return;
  }

  init_atoms(display);

  int screen = DefaultScreen(display);
  Window root = get_root_window(display);

  unsigned long base_win_items = 0;
  win_info *base_win_info = NULL;

  // Arrange the first window init
  int base_workspace_num = get_current_workspace(display, root);
  Window *windows = fetch_window_list(display, root, &base_win_items,
                                      atoms.client_list, base_workspace_num);
  if (windows) {
    base_win_info = (win_info *)malloc(base_win_items * sizeof(win_info));
    if (base_win_info == NULL) {
      LOG(GRIDFLUX_WARN, ERR_FAIL_ALLOCATE);
      free(windows);
      XCloseDisplay(display);
      exit(EXIT_FAILURE);
      return;
    }

    for (unsigned long int i = 0; i < base_win_items; i++) {
      unmaximize_window(display, windows[i]);
      arrange_window(base_win_items, windows, display, screen);
    }

    XFree(windows);
  }

  unsigned long curr_win_items = 0;
  XSetErrorHandler(error_handler);

  while (1) {
    int total_workspaces = get_total_workspace(display, root);

    unsigned long total_window = get_total_window(display, root);
    int workspace_need = (int)total_window / WIN_APP_LIMIT;

    if (total_workspaces <= workspace_need) {
      set_workspace(display, root, workspace_need);
      usleep(20000);
    }

    manage_window(display, root, &base_win_items, base_win_info, screen);
    usleep(20000);
  }

  XCloseDisplay(display);
}
