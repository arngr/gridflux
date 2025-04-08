
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
 * Copyright (C) 2025 Ardinugraha
 */

#ifndef GF_ARRANGE_H
#define GF_ARRANGE_H

typedef void (*gf_set_geometry_func)(void *window, int x, int y, int width,
                                     int height, void *user_data, char *sess);

typedef struct {
  gf_set_geometry_func set_geometry;
  void *user_data;
  char *session;
} gf_split_ctx;

void gf_set_geometry(void *window_ptr, int x, int y, int width, int height,
                     void *user_data, char *session);

void gf_split_window_generic(void **windows, int window_count, int x, int y,
                             int width, int height, int depth,
                             gf_split_ctx *ctx);

#endif
