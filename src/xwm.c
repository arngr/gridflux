/*
 * This file is part of gridflux.
 *
 * gridflux is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * gridflux is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gridflux.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2024 Ardi Nugraha
 */

#include "xwm.h"
#include "ewmh.h"
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
#include <unistd.h>

const int MAX_WIN_OPEN = 8;

static Display *wm_x_initialize_display() {
  int try_index = 0;
  while (1) {
    Display *display = XOpenDisplay(NULL);
    if (!display) {
      try_index++;
      LOG(GF_ERR, ERR_DISPLAY_NULL);
      sleep(1);
    } else {
      return display;
    }
  }
}

static Window wm_x_get_root_window(Display *display) {
  return RootWindow(display, DefaultScreen(display));
}

static int wm_x_error_handler(Display *display, XErrorEvent *error) {
  if (error->error_code == BadWindow) {
    LOG(GF_ERR, ERR_BAD_WINDOW);
  }
  return 0; // Return 0 to prevent the program from terminating
}

static unsigned char *wm_x_get_window_property(Display *display, Window window,
                                               Atom property, Atom type,
                                               unsigned long *nitems,
                                               int *status_out) {
  Atom actual_type;
  int actual_format;
  unsigned long bytes_after;
  unsigned char *data = NULL;

  int status = XGetWindowProperty(display, window, property, 0, (~0L), False,
                                  type, &actual_type, &actual_format, nitems,
                                  &bytes_after, &data);

  if (status_out)
    *status_out = status;

  if (status != Success || !data || *nitems == 0) {
    LOG(GF_ERR, "Failed to fetch property (status=%d, nitems=%lu)", status,
        *nitems);

    if (data)
      XFree(data);
    return NULL;
  }

  return data;
}

static int wm_x_send_client_message(Display *display, Window window,
                                    Atom message_type, Atom *atoms,
                                    int atom_count, long *data) {
  if (message_type == None) {
    LOG(GF_ERR, ERR_MSG_NULL);
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
    LOG(GF_ERR, ERR_SEND_MSG_FAIL);
    return -1;
  }

  XFlush(display);
  return 0;
}

static void wm_x_unmaximize_window(Display *display, Window window) {
  long data[] = {0, atoms.net_wm_max_horz,
                 atoms.net_wm_max_vert}; // Unmaximize state data
  wm_x_send_client_message(display, window, atoms.wm_state, NULL, 3, data);
}

static int wm_x_move_window_to_workspace(Display *display, Window window,
                                         int workspace) {
  long data[] = {workspace, CurrentTime}; // Move to target workspace
  if (wm_x_send_client_message(display, window, atoms.net_wm_desktop, NULL, 2,
                               data) == 0) {
    return 0;
  }
  return 1;
}

static void wm_x_get_window_dimension(Display *display, Window window,
                                      int *width, int *height, int *x, int *y) {
  XWindowAttributes attributes;
  int status = XGetWindowAttributes(display, window, &attributes);

  if (status == 0) {
    LOG(GF_ERR, "Invalid window.\n");
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

int wm_x_excluded_window(Display *display, Window window) {
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;

  if (XGetWindowProperty(display, window, atoms.net_wm_state, 0, 1024, False,
                         XA_ATOM, &actual_type, &actual_format, &nitems,
                         &bytes_after, &prop) != Success ||
      prop == NULL) {
    return 0;
  }

  Atom *states = (Atom *)prop;
  int result = 0;

  // Check for excluded window states
  const Atom excluded_states[] = {
      atoms.net_wm_hidden,       atoms.net_wm_notification,
      atoms.net_wm_popup_menu,   atoms.net_wm_tooltip,
      atoms.net_wm_toolbar,      atoms.net_wm_modal,
      atoms.net_wm_skip_taskbar, atoms.net_wm_utility};

  const size_t excluded_count =
      sizeof(excluded_states) / sizeof(excluded_states[0]);

  for (unsigned long i = 0; i < nitems && !result; i++) {
    for (size_t j = 0; j < excluded_count; j++) {
      if (states[i] == excluded_states[j]) {
        result = 1;
        break;
      }
    }
  }

  XFree(prop);
  return result;
}

void wm_x_set_geometry(Display *display, Window window, int gravity,
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

  int pad = (mask & APPLY_PADDING) ? DEFAULT_PADDING : 0;
  x += pad;
  y += pad;
  width = width > pad * 2 ? width - pad * 2 : width;
  height = height > pad * 2 ? height - pad * 2 : height;

  // Remove decorations using _MOTIF_WM_HINTS (optional)
  if (mask & HINT_NO_DECORATIONS) {
    struct {
      unsigned long flags;
      unsigned long functions;
      unsigned long decorations;
      long input_mode;
      unsigned long status;
    } hints = {2, 0, 0, 0, 0}; // decorations = 0 → no border/titlebar

    XChangeProperty(display, window, atoms.motif_wm_hints, atoms.motif_wm_hints,
                    32, PropModeReplace, (unsigned char *)&hints, 5);
  }

  XWindowChanges changes;
  unsigned int value_mask = 0;

  if ((mask & (CHANGE_X | CHANGE_Y | CHANGE_WIDTH | CHANGE_HEIGHT)) !=
      (CHANGE_X | CHANGE_Y | CHANGE_WIDTH | CHANGE_HEIGHT)) {
    Window root;
    int cur_x, cur_y;
    unsigned int cur_width, cur_height, border, depth;

    XGetGeometry(display, window, &root, &cur_x, &cur_y, &cur_width,
                 &cur_height, &border, &depth);

    changes.x = (mask & CHANGE_X) ? x : cur_x;
    changes.y = (mask & CHANGE_Y) ? y : cur_y;
    changes.width = (mask & CHANGE_WIDTH) ? width : cur_width;
    changes.height = (mask & CHANGE_HEIGHT) ? height : cur_height;
  } else {
    changes.x = x;
    changes.y = y;
    changes.width = width;
    changes.height = height;
  }

  if (mask & CHANGE_X)
    value_mask |= CWX;
  if (mask & CHANGE_Y)
    value_mask |= CWY;
  if (mask & CHANGE_WIDTH)
    value_mask |= CWWidth;
  if (mask & CHANGE_HEIGHT)
    value_mask |= CWHeight;

  XConfigureWindow(display, window, value_mask, &changes);
  XFlush(display);
}

static void wm_x_arrange_window(int window_count, Window windows[],
                                Display *display, int screen) {

  Screen *scr = ScreenOfDisplay(display, screen);
  int screen_width = scr->width - 5;
  int screen_height = scr->height;

  void *wins[MAX_WIN_OPEN];

  for (int i = 0; i < window_count; i++)
    wins[i] = &windows[i];

  gf_split_ctx ctx = {
      .set_geometry = gf_set_geometry, .user_data = display, .session = GF_X11};

  gf_split_window_generic(wins, window_count, 0, 0, screen_width, screen_height,
                          0, &ctx);
}

static Bool wm_x_window_in_workspace(Display *display, Window window,
                                     int workspace_id) {
  if (atoms.net_wm_desktop == None)
    return False;

  unsigned long nitems = 0;
  int status;

  unsigned char *data = wm_x_get_window_property(
      display, window, atoms.net_wm_desktop, XA_CARDINAL, &nitems, &status);

  if (!data || status != Success || nitems < 1)
    return False;

  unsigned long window_workspace_id = *(unsigned long *)data;
  XFree(data);

  return window_workspace_id == (unsigned long)workspace_id;
}

static Window *wm_x_filter_windows(Display *display, Window *windows,
                                   unsigned long *nitems, int workspace_id) {
  if (!windows || !nitems || *nitems == 0)
    return NULL;

  Window *filtered = malloc(sizeof(Window) * (*nitems));
  if (!filtered) {
    LOG(GF_ERR, ERR_FAIL_ALLOCATE);
    return NULL;
  }

  unsigned long count = 0;
  for (unsigned long i = 0; i < *nitems; ++i) {
    Window window = windows[i];
    if (wm_x_excluded_window(display, window))
      continue;

    if (wm_x_window_in_workspace(display, window, workspace_id)) {
      filtered[count++] = window;
    }
  }

  *nitems = count;
  return filtered;
}

static Window *wm_x_get_window_property_list(Display *display, Window root,
                                             Atom atom, unsigned long *nitems) {
  if (!display || !nitems)
    return NULL;

  int status;
  unsigned char *data =
      wm_x_get_window_property(display, root, atom, XA_WINDOW, nitems, &status);

  if (status != Success || !data || *nitems == 0) {
    if (data)
      XFree(data);
    return NULL;
  }

  return (Window *)data;
}

static Window *wm_x_fetch_window_list(Display *display, Window root,
                                      unsigned long *nitems, Atom atom,
                                      int workspace_id) {
  if (!display || !nitems)
    return NULL;

  Window *windows = wm_x_get_window_property_list(display, root, atom, nitems);
  if (!windows || *nitems == 0)
    return NULL;

  Window *filtered =
      wm_x_filter_windows(display, windows, nitems, workspace_id);
  XFree(windows);

  return filtered;
}

static unsigned long int wm_x_get_current_workspace(Display *display,
                                                    Window root) {
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
    LOG(GF_ERR, ERR_BAD_WINDOW);
    XCloseDisplay(display);
    return -1;
  }
}

static Window wm_x_get_last_opened_window(Display *display) {
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

static unsigned long wm_x_get_total_workspace(Display *display, Window root) {
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
      LOG(GF_ERR, ERR_BAD_WINDOW);
    }
  } else {
    LOG(GF_ERR, ERR_BAD_WINDOW);
  }

  return total_workspaces;
}

static int wm_x_get_total_window(Display *display, Window root) {
  unsigned long total_win = 0;
  int total_workspaces = wm_x_get_total_workspace(display, root);

  for (int i = 0; i <= total_workspaces; i++) {
    unsigned long current_window_count = 0;
    Window *winlist = wm_x_fetch_window_list(
        display, root, &current_window_count, atoms.client_list, i);
    if (!winlist)
      continue;
  }

  return total_win;
}

static void wm_x_arrange_dimension(Display *display, Window root,
                                   unsigned long base_win_items,
                                   Window *curr_win_open,
                                   gf_win_info *base_gf_win_info, int screen) {
  for (unsigned long int i = 0; i < base_win_items; i++) {
    gf_win_info *curr_gf_win_info =
        (gf_win_info *)malloc(base_win_items * sizeof(gf_win_info));
    wm_x_get_window_dimension(display, curr_win_open[i],
                              &curr_gf_win_info[i].width,
                              &curr_gf_win_info[i].height, NULL, NULL);

    if (curr_gf_win_info[i].width != base_gf_win_info[i].width ||
        curr_gf_win_info[i].height != base_gf_win_info[i].height) {
      wm_x_unmaximize_window(display, curr_win_open[i]);
      wm_x_arrange_window(base_win_items, curr_win_open, display, screen);

      base_gf_win_info[i].width = curr_gf_win_info[i].width;
      base_gf_win_info[i].height = curr_gf_win_info[i].height;
    }

    free(curr_gf_win_info);
  }
}

static void wm_x_distribute_overflow_window(
    Display *display, int *overflow_workspace, int overflow_workspace_total,
    gf_workspace_info *free_workspace, int free_workspace_total,
    Atom window_list_atom, Window root, int screen) {
  for (int i = 0; i < overflow_workspace_total; i++) {
    unsigned long current_window_count = 0;
    Window *active_windows =
        wm_x_fetch_window_list(display, root, &current_window_count,
                               window_list_atom, overflow_workspace[i]);

    for (int j = 0; j <= free_workspace_total; j++) {
      for (int x = 0; x <= free_workspace[j].available_space; x++) {
        if (current_window_count >= MAX_WIN_OPEN) {
          wm_x_unmaximize_window(display,
                                 active_windows[current_window_count--]);
          wm_x_move_window_to_workspace(display,
                                        active_windows[current_window_count--],
                                        free_workspace[j].workspace_id);
          free_workspace[j].available_space--;
        }
      }
    }

    if (active_windows) {
      wm_x_arrange_window(current_window_count, active_windows, display,
                          screen);
    }
  }
}

static void wm_x_handle_window_overflow(Display *display, Window root,
                                        unsigned long *current_window_count,
                                        Atom window_list_atom,
                                        int total_workspace, int screen) {
  int overflow_workspace[total_workspace];
  int overflow_workspace_total = 0;

  gf_workspace_info free_workspace[total_workspace];
  int free_workspace_total = 0;

  for (int workspace = 0; workspace <= total_workspace; workspace++) {
    Window *active_windows = wm_x_fetch_window_list(
        display, root, current_window_count, window_list_atom, workspace);

    if (*current_window_count >= MAX_WIN_OPEN) {
      overflow_workspace[overflow_workspace_total] = workspace;
      overflow_workspace_total++;
    } else if (*current_window_count + 1 < MAX_WIN_OPEN) {
      int available_space = MAX_WIN_OPEN - *current_window_count;
      free_workspace[free_workspace_total].workspace_id = workspace;
      free_workspace[free_workspace_total].total_window_open =
          *current_window_count;
      free_workspace[free_workspace_total].available_space = available_space;
      free_workspace_total++;
    }
  }
  wm_x_distribute_overflow_window(
      display, overflow_workspace, overflow_workspace_total, free_workspace,
      free_workspace_total, window_list_atom, root, screen);
}

static void wm_x_manage_workspace_window(Display *display, Window root,
                                         unsigned long *previous_window_count,
                                         int total_workspace, int screen) {
  unsigned long current_window_count = 0;
  Window *active_windows;

  for (int workspace = 0; workspace < total_workspace; workspace++) {
    active_windows = wm_x_fetch_window_list(
        display, root, &current_window_count, atoms.client_list, workspace);
    if (!active_windows) {
      continue;
    }

    if (current_window_count > MAX_WIN_OPEN) {
      wm_x_handle_window_overflow(display, root, &current_window_count,
                                  atoms.client_list, total_workspace, screen);
    }
  }
}

static void
wm_x_rearrange_current_workspace(Display *display, Window root,
                                 unsigned long *previous_window_count,
                                 int screen, gf_win_info *window_properties) {
  unsigned long current_window_count = 0;
  int current_workspace = wm_x_get_current_workspace(display, root);
  Window *active_windows =
      wm_x_fetch_window_list(display, root, &current_window_count,
                             atoms.client_list, current_workspace);

  if (current_window_count != *previous_window_count) {
    *previous_window_count = current_window_count;

    for (unsigned long i = 0; i <= current_window_count; i++) {
      if (active_windows[i]) {
        wm_x_unmaximize_window(display, active_windows[i]);
      }
    }

    if (active_windows) {
      wm_x_arrange_window(current_window_count, active_windows, display,
                          screen);
    }
  }

  wm_x_arrange_dimension(display, root, current_window_count, active_windows,
                         window_properties, screen);

  if (active_windows) {
    XFree(active_windows);
  }
}

// Hacky
const char *wm_x_detect_desktop_environment() {
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

static void wm_x_create_new_workspace(Display *display, Window root,
                                      unsigned long new_workspace) {

  XChangeProperty(display, root, atoms.net_curr_desktop, XA_CARDINAL, 32,
                  PropModeReplace, (unsigned char *)&new_workspace, 1);
  XSync(display, False); // Ensure the action is committed
}

// Hacky
static void wm_x_set_workspace(Display *display, Window root,
                               unsigned long workspace) {
  const char *desktop_session = wm_x_detect_desktop_environment();
  if (desktop_session == NULL || strcmp(desktop_session, "Unknown") == 0) {
    LOG(GF_DBG, "No desktop session detected. Exiting.\n");
    return;
  }

  pid_t pid = fork();

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
      wm_x_create_new_workspace(display, root, workspace);
    } else {
      int status;
      waitpid(pid, &status, 0);
      if (WIFEXITED(status)) {
        LOG(GF_DBG, "command exited with status %d\n", WEXITSTATUS(status));
      } else {
        perror("command did not terminate normally\n");
        _exit(EXIT_FAILURE);
      }
    }
  }
}

static void wm_x_manage_window(Display *display, Window root,
                               unsigned long *previous_window_count,
                               gf_win_info *window_properties, int screen) {
  if (!display) {
    LOG(GF_WARN, ERR_DISPLAY_NULL);
    return;
  }

  unsigned long total_workspace = wm_x_get_total_workspace(display, root);

  wm_x_manage_workspace_window(display, root, previous_window_count,
                               total_workspace, screen);
  wm_x_rearrange_current_workspace(display, root, previous_window_count, screen,
                                   window_properties);
}

void wm_x_run_layout() {
  Display *display = wm_x_initialize_display();
  if (!display) {
    LOG(GF_ERR, ERR_DISPLAY_NULL);
    return;
  }

  gf_init_atom(display);

  int screen = DefaultScreen(display);
  Window root = wm_x_get_root_window(display);

  unsigned long base_win_items = 0;
  gf_win_info *base_gf_win_info = NULL;

  // Arrange the first window init
  int base_workspace_num = wm_x_get_current_workspace(display, root);
  Window *windows = wm_x_fetch_window_list(
      display, root, &base_win_items, atoms.client_list, base_workspace_num);
  if (windows) {
    base_gf_win_info =
        (gf_win_info *)malloc(base_win_items * sizeof(gf_win_info));
    if (base_gf_win_info == NULL) {
      LOG(GF_WARN, ERR_FAIL_ALLOCATE);
      free(windows);
      XCloseDisplay(display);
      exit(EXIT_FAILURE);
      return;
    }

    for (unsigned long int i = 0; i < base_win_items; i++) {
      wm_x_unmaximize_window(display, windows[i]);
      wm_x_arrange_window(base_win_items, windows, display, screen);
    }

    XFree(windows);
  }

  unsigned long curr_win_items = 0;
  XSetErrorHandler(wm_x_error_handler);

  while (1) {
    int total_workspaces = wm_x_get_total_workspace(display, root);

    unsigned long total_window = wm_x_get_total_window(display, root);
    int workspace_need = (int)total_window / MAX_WIN_OPEN;

    if (total_workspaces <= workspace_need) {
      wm_x_set_workspace(display, root, workspace_need);
      usleep(20000);
    }

    wm_x_manage_window(display, root, &base_win_items, base_gf_win_info,
                       screen);
    usleep(20000);
  }

  XCloseDisplay(display);
}
