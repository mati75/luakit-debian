/*
 * globalconf.h - main config struct
 *
 * Copyright (C) 2010 Mason Larobina <mason.larobina@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef LUAKIT_GLOBALCONF
#define LUAKIT_GLOBALCONF

#define LUAKIT_LUA_LIB_PATH         "/usr/share/luakit/lib"
#define LUAKIT_OBJECT_REGISTRY_KEY  "luakit.object.registry"

#include <glib/gtypes.h>
#include <lua.h>
#include "common/signal.h"

typedef struct {
    /* XDG directories */
    gchar *base_cache_directory;
    gchar *base_config_directory;
    gchar *base_data_directory;
    /* Path to the current config file */
    gchar *confpath;
    /* Lua VM state */
    lua_State *L;
    /* Global signals table */
    signal_t *signals;
    /* Array of windows */
    GPtrArray *windows;
} luakit_t;

luakit_t globalconf;

#endif
// vim: ft=c:et:sw=4:ts=8:sts=4:enc=utf-8:tw=80
