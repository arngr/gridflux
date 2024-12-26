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
 * Copyright (C) 2024 Ardinugraha
 */

#include <stdio.h>

#define LITTLEWIN_ERR 1
#define LITTLEWIN_WARN 2
#define LITTLEWIN_INFO 3
#define LITTLEWIN_DBG 4

// Current log level (change this to set the level)
#define CURRENT_LOG_LEVEL LITTLEWIN_DBG

#define LOG(level, fmt, ...)                                                   \
  do {                                                                         \
    if (level <= CURRENT_LOG_LEVEL) {                                          \
      printf("[%s] " fmt, #level, ##__VA_ARGS__);                              \
    }                                                                          \
  } while (0)
