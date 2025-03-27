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
 * Copyright (C) 2024 Ardinugraha
 */

#include <stdio.h>
#include <time.h>

#define GRIDFLUX_ERR 1
#define GRIDFLUX_WARN 2
#define GRIDFLUX_INFO 3
#define GRIDFLUX_DBG 4

#define CURRENT_LOG_LEVEL GRIDFLUX_DBG

#define CHANGE_X (1 << 0)
#define CHANGE_Y (1 << 1)
#define CHANGE_WIDTH (1 << 2)
#define CHANGE_HEIGHT (1 << 3)

#define RED "\x1b[31m"
#define YELLOW "\x1b[33m"
#define GREEN "\x1b[32m"
#define BLUE "\x1b[34m"
#define RESET "\x1b[0m"

#define LOG(level, fmt, ...)                                                   \
  do {                                                                         \
    if (level <= CURRENT_LOG_LEVEL) {                                          \
      time_t rawtime;                                                          \
      struct tm *timeinfo;                                                     \
      char timeStr[20];                                                        \
                                                                               \
      time(&rawtime);                                                          \
      timeinfo = localtime(&rawtime);                                          \
      strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);       \
                                                                               \
      const char *color = RESET;                                               \
      const char *level_str = "UNKNOWN";                                       \
                                                                               \
      switch (level) {                                                         \
      case GRIDFLUX_ERR:                                                       \
        color = RED;                                                           \
        level_str = "ERROR";                                                   \
        break;                                                                 \
      case GRIDFLUX_WARN:                                                      \
        color = YELLOW;                                                        \
        level_str = "WARNING";                                                 \
        break;                                                                 \
      case GRIDFLUX_INFO:                                                      \
        color = GREEN;                                                         \
        level_str = "INFO";                                                    \
        break;                                                                 \
      case GRIDFLUX_DBG:                                                       \
        color = BLUE;                                                          \
        level_str = "DEBUG";                                                   \
        break;                                                                 \
      }                                                                        \
                                                                               \
      printf("%s[%s] [%s] [%s:%d] %s(): " fmt RESET "\n", color, timeStr,      \
             level_str, __FILE__, __LINE__, __func__, ##__VA_ARGS__);          \
    }                                                                          \
  } while (0)
