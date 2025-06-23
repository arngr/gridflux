
#ifndef ATOMS_H
#define ATOMS_H

#define ERR_DISPLAY_NULL "Display is NULL"
#define ERR_WINDOW_INVALID "Invalid window"
#define ERR_DIMENSIONS_NULL "Width or height pointer is NULL"
#define ERR_BAD_WINDOW "Bad window"
#define ERR_UNKNOWN "Unknown error"
#define ERR_MSG_NULL "Message is NULL"
#define ERR_SEND_MSG_FAIL "Fail to send message"
#define ERR_FAIL_ALLOCATE "Fail to allocate"

#include <X11/Xlib.h>

typedef struct {
  Atom wm_state;
  Atom net_wm_state;
  Atom net_wm_max_horz;
  Atom net_wm_max_vert;
  Atom net_wm_desktop;
  Atom net_wm_type;
  Atom net_wm_tooltip;
  Atom net_wm_notification;
  Atom net_wm_toolbar;
  Atom net_wm_normal;
  Atom net_wm_hidden;
  Atom net_wm_popup_menu;
  Atom net_wm_utility;

  Atom client_list;
  Atom client_list_stack;
  Atom num_of_desktop;
  Atom net_curr_desktop;

  Atom motif_wm_hints;
  Atom net_wm_modal;
  Atom net_wm_skip_taskbar;
  Atom gtk_frame_extents;
  Atom net_frame_extents;
  Atom net_moveresize_window;
} gf_atom_type;

extern gf_atom_type atoms;
void gf_init_atom(Display *display);

#endif // ATOMS_H
