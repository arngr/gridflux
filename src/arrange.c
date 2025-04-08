#include "arrange.h"
#include "gridflux.h"
#include "xsession.h"
#include <string.h>

void gf_split_window_generic(void **windows, int window_count, int x, int y,
                             int width, int height, int depth,
                             gf_split_ctx *ctx) {
  if (window_count <= 0)
    return;

  if (window_count == 1 && windows[0] != NULL && ctx->set_geometry != NULL) {
    ctx->set_geometry(windows[0], x, y, width, height, ctx->user_data,
                      ctx->session);
    return;
  }

  int split_vertically = depth % 2 == 0;
  int left_count = window_count / 2;
  int right_count = window_count - left_count;

  if (split_vertically) {
    int left_width = width / 2;
    int right_width = width - left_width;

    gf_split_window_generic(windows, left_count, x, y, left_width, height,
                            depth + 1, ctx);
    gf_split_window_generic(windows + left_count, right_count, x + left_width,
                            y, right_width, height, depth + 1, ctx);
  } else {
    int top_height = height / 2;
    int bottom_height = height - top_height;

    gf_split_window_generic(windows, left_count, x, y, width, top_height,
                            depth + 1, ctx);
    gf_split_window_generic(windows + left_count, right_count, x,
                            y + top_height, width, bottom_height, depth + 1,
                            ctx);
  }
}

void gf_set_geometry(void *window_ptr, int x, int y, int width, int height,
                     void *user_data, char *session) {
  if (!window_ptr || !user_data) {
    LOG(GF_ERR, " NULL Pointer detected \n");
    return;
  }

  if (strcmp(session, GF_X11) == 0) {
    Display *display = (Display *)user_data;
    Window window = *(Window *)window_ptr;

    wm_x_set_geometry(display, window, StaticGravity,
                      CHANGE_X | CHANGE_Y | CHANGE_WIDTH | CHANGE_HEIGHT |
                          APPLY_PADDING,
                      x, y, width, height);
  }
}
