
#include "atoms.h"
#include <X11/Xlib.h>

gf_atom_type atoms;

void gf_init_atom(Display *display) {
  atoms.wm_state = XInternAtom(display, "WM_STATE", False);
  atoms.net_wm_state = XInternAtom(display, "_NET_WM_STATE", False);
  atoms.net_wm_max_horz =
      XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
  atoms.net_wm_max_vert =
      XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
  atoms.net_wm_desktop = XInternAtom(display, "_NET_WM_DESKTOP", True);
  atoms.net_wm_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
  atoms.net_wm_tooltip =
      XInternAtom(display, "_NET_WM_WINDOW_TYPE_TOOLTIP", True);
  atoms.net_wm_notification =
      XInternAtom(display, "_NET_WM_WINDOW_TYPE_NOTIFICATION", True);
  atoms.net_wm_toolbar =
      XInternAtom(display, "_NET_WM_WINDOW_TYPE_TOOLBAR", True);
  atoms.net_wm_hidden = XInternAtom(display, "_NET_WM_STATE_HIDDEN", False);
  atoms.net_wm_popup_menu =
      XInternAtom(display, "_NET_WM_WINDOW_TYPE_POPUP_MENU", False);
  atoms.net_wm_normal =
      XInternAtom(display, "_NET_WM_WINDOW_TYPE_NORMAL", True);

  atoms.net_wm_utility =
      XInternAtom(display, "_NET_WM_WINDOW_TYPE_UTILITY", False);

  atoms.client_list = XInternAtom(display, "_NET_CLIENT_LIST", True);
  atoms.client_list_stack =
      XInternAtom(display, "_NET_CLIENT_LIST_STACKING", True);
  atoms.num_of_desktop = XInternAtom(display, "_NET_NUMBER_OF_DESKTOPS", True);
  atoms.net_curr_desktop = XInternAtom(display, "_NET_CURRENT_DESKTOP", True);
  atoms.motif_wm_hints = XInternAtom(display, "_MOTIF_WM_HINTS", False);
  atoms.net_wm_modal = XInternAtom(display, "_NET_WM_STATE_MODAL", False);
  atoms.net_wm_skip_taskbar =
      XInternAtom(display, "_NET_WM_STATE_SKIP_TASKBAR", False);
  atoms.net_frame_extents = XInternAtom(display, "_NET_FRAME_EXTENTS", False);
  atoms.gtk_frame_extents = XInternAtom(display, "_GTK_FRAME_EXTENTS", False);
  atoms.net_moveresize_window =
      XInternAtom(display, "_NET_MOVERESIZE_WINDOW", False);
}
