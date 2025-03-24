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

#include "x_session.h"

int main() {
#ifdef __linux
  char *session_type = getenv("XDG_SESSION_TYPE");
  if (session_type != NULL) {
    if (strcmp(session_type, "x11") == 0) {
      run_x_layout();
    } else {
      printf("The session %s type is not supported.\n", session_type);
    }
  } else {
    printf("The session is not set.\n");
  }
#endif
  return 0;
}
