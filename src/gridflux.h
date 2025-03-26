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
      switch (level) {                                                         \
      case GRIDFLUX_ERR:                                                       \
        color = RED;                                                           \
        break;                                                                 \
      case GRIDFLUX_WARN:                                                      \
        color = YELLOW;                                                        \
        break;                                                                 \
      case GRIDFLUX_INFO:                                                      \
        color = GREEN;                                                         \
        break;                                                                 \
      case GRIDFLUX_DBG:                                                       \
        color = BLUE;                                                          \
        break;                                                                 \
      default:                                                                 \
        color = RESET;                                                         \
        break;                                                                 \
      }                                                                        \
      printf("%s[%s] [%s] " fmt RESET, color, timeStr, #level, ##__VA_ARGS__); \
    }                                                                          \
  } while (0)
