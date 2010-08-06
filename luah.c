/*
 * luah.c - Lua helper functions
 *
 * Copyright (C) 2010 Mason Larobina <mason.larobina@gmail.com>
 * Copyright (C) 2008-2009 Julien Danjou <julien@danjou.info>
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

#include <gtk/gtk.h>
#include <basedir_fs.h>
#include <stdlib.h>
#include "common/util.h"
#include "common/lualib.h"
#include "luakit.h"
#include "widget.h"
#include "luah.h"

void
luaH_modifier_table_push(lua_State *L, guint state) {
    gint i = 1;
    lua_newtable(L);
    if (state & GDK_MODIFIER_MASK) {

#define MODKEY(key, name)           \
    if (state & GDK_##key##_MASK) { \
        lua_pushstring(L, name);    \
        lua_rawseti(L, -2, i++);    \
    }

        MODKEY(SHIFT, "Shift");
        MODKEY(LOCK, "Lock");
        MODKEY(CONTROL, "Control");
        MODKEY(MOD1, "Mod1");
        MODKEY(MOD2, "Mod2");
        MODKEY(MOD3, "Mod3");
        MODKEY(MOD4, "Mod4");
        MODKEY(MOD5, "Mod5");

#undef MODKEY

    }
}

void
luaH_keystr_push(lua_State *L, guint keyval)
{
    gchar ucs[7];
    guint ulen;
    guint32 ukval = gdk_keyval_to_unicode(keyval);

    /* check for printable unicode character */
    if (g_unichar_isgraph(ukval)) {
        ulen = g_unichar_to_utf8(ukval, ucs);
        ucs[ulen] = 0;
        lua_pushstring(L, ucs);
    }
    /* sent keysym for non-printable characters */
    else
        lua_pushstring(L, gdk_keyval_name(keyval));
}

/* UTF-8 aware string length computing.
 * Returns the number of elements pushed on the stack. */
static gint
luaH_utf8_strlen(lua_State *L)
{
    const gchar *cmd  = luaL_checkstring(L, 1);
    lua_pushnumber(L, (ssize_t) g_utf8_strlen(NONULL(cmd), -1));
    return 1;
}

/* Overload standard Lua next function to use __next key on metatable.
 * Returns the number of elements pushed on stack. */
static gint
luaHe_next(lua_State *L)
{
    if(luaL_getmetafield(L, 1, "__next")) {
        lua_insert(L, 1);
        lua_call(L, lua_gettop(L) - 1, LUA_MULTRET);
        return lua_gettop(L);
    }
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 2);
    if(lua_next(L, 1))
        return 2;
    lua_pushnil(L);
    return 1;
}

/* Overload lua_next() function by using __next metatable field to get
 * next elements. `idx` is the index number of elements in stack.
 * Returns 1 if more elements to come, 0 otherwise. */
gint
luaH_next(lua_State *L, gint idx)
{
    if(luaL_getmetafield(L, idx, "__next")) {
        /* if idx is relative, reduce it since we got __next */
        if(idx < 0) idx--;
        /* copy table and then move key */
        lua_pushvalue(L, idx);
        lua_pushvalue(L, -3);
        lua_remove(L, -4);
        lua_pcall(L, 2, 2, 0);
        /* next returned nil, it's the end */
        if(lua_isnil(L, -1)) {
            /* remove nil */
            lua_pop(L, 2);
            return 0;
        }
        return 1;
    }
    else if(lua_istable(L, idx))
        return lua_next(L, idx);
    /* remove the key */
    lua_pop(L, 1);
    return 0;
}

/* Generic pairs function.
 * Returns the number of elements pushed on stack. */
static gint
luaH_generic_pairs(lua_State *L)
{
    lua_pushvalue(L, lua_upvalueindex(1));  /* return generator, */
    lua_pushvalue(L, 1);  /* state, */
    lua_pushnil(L);  /* and initial value */
    return 3;
}

/* Overload standard pairs function to use __pairs field of metatables.
 * Returns the number of elements pushed on stack. */
static gint
luaHe_pairs(lua_State *L)
{
    if(luaL_getmetafield(L, 1, "__pairs")) {
        lua_insert(L, 1);
        lua_call(L, lua_gettop(L) - 1, LUA_MULTRET);
        return lua_gettop(L);
    }
    luaL_checktype(L, 1, LUA_TTABLE);
    return luaH_generic_pairs(L);
}

static gint
luaH_ipairs_aux(lua_State *L)
{
    gint i = luaL_checkint(L, 2) + 1;
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_pushinteger(L, i);
    lua_rawgeti(L, 1, i);
    return (lua_isnil(L, -1)) ? 0 : 2;
}

/* Overload standard ipairs function to use __ipairs field of metatables.
 * Returns the number of elements pushed on stack. */
static gint
luaHe_ipairs(lua_State *L)
{
    if(luaL_getmetafield(L, 1, "__ipairs")) {
        lua_insert(L, 1);
        lua_call(L, lua_gettop(L) - 1, LUA_MULTRET);
        return lua_gettop(L);
    }

    luaL_checktype(L, 1, LUA_TTABLE);
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_pushvalue(L, 1);
    lua_pushinteger(L, 0);  /* and initial value */
    return 3;
}

/* Enhanced type() function which recognize luakit objects.
 * \param L The Lua VM state.
 * \return The number of arguments pushed on the stack.
 */
static gint
luaHe_type(lua_State *L)
{
    luaL_checkany(L, 1);
    lua_pushstring(L, luaH_typename(L, 1));
    return 1;
}


/* Fix up and add handy standard lib functions */
static void
luaH_fixups(lua_State *L)
{
    /* export string.wlen */
    lua_getglobal(L, "string");
    lua_pushcfunction(L, &luaH_utf8_strlen);
    lua_setfield(L, -2, "wlen");
    lua_pop(L, 1);
    /* replace next */
    lua_pushliteral(L, "next");
    lua_pushcfunction(L, luaHe_next);
    lua_settable(L, LUA_GLOBALSINDEX);
    /* replace pairs */
    lua_pushliteral(L, "pairs");
    lua_pushcfunction(L, luaHe_next);
    lua_pushcclosure(L, luaHe_pairs, 1); /* pairs get next as upvalue */
    lua_settable(L, LUA_GLOBALSINDEX);
    /* replace ipairs */
    lua_pushliteral(L, "ipairs");
    lua_pushcfunction(L, luaH_ipairs_aux);
    lua_pushcclosure(L, luaHe_ipairs, 1);
    lua_settable(L, LUA_GLOBALSINDEX);
    /* replace type */
    lua_pushliteral(L, "type");
    lua_pushcfunction(L, luaHe_type);
    lua_settable(L, LUA_GLOBALSINDEX);
}

/* Look for an item: table, function, etc.
 * \param L The Lua VM state.
 * \param item The pointer item.
 */
gboolean
luaH_hasitem(lua_State *L, gconstpointer item)
{
    lua_pushnil(L);
    while(luaH_next(L, -2)) {
        if(lua_topointer(L, -1) == item) {
            /* remove value and key */
            lua_pop(L, 2);
            return TRUE;
        }
        if(lua_istable(L, -1))
            if(luaH_hasitem(L, item)) {
                /* remove key and value */
                lua_pop(L, 2);
                return TRUE;
            }
        /* remove value */
        lua_pop(L, 1);
    }
    return FALSE;
}

/* Browse a table pushed on top of the index, and put all its table and
 * sub-table ginto an array.
 * \param L The Lua VM state.
 * \param elems The elements array.
 * \return False if we encounter an elements already in list.
 */
static gboolean
luaH_isloop_check(lua_State *L, GPtrArray *elems)
{
    if(lua_istable(L, -1)) {
        gconstpointer object = lua_topointer(L, -1);

        /* Check that the object table is not already in the list */
        for(guint i = 0; i < elems->len; i++)
            if(elems->pdata[i] == object)
                return FALSE;

        /* push the table in the elements list */
        g_ptr_array_add(elems, (gpointer) object);

        /* look every object in the "table" */
        lua_pushnil(L);
        while(luaH_next(L, -2)) {
            if(!luaH_isloop_check(L, elems)) {
                /* remove key and value */
                lua_pop(L, 2);
                return FALSE;
            }
            /* remove value, keep key for next iteration */
            lua_pop(L, 1);
        }
    }
    return TRUE;
}

/* Check if a table is a loop. When using tables as direct acyclic digram,
 * this is useful.
 * \param L The Lua VM state.
 * \param idx The index of the table in the stack
 * \return True if the table loops.
 */
gboolean
luaH_isloop(lua_State *L, gint idx)
{
    /* elems is an elements array that we will fill with all array we
     * encounter while browsing the tables */
    GPtrArray *elems = g_ptr_array_new();

    /* push table on top */
    lua_pushvalue(L, idx);

    gboolean ret = luaH_isloop_check(L, elems);

    /* remove pushed table */
    lua_pop(L, 1);

    g_ptr_array_free(elems, TRUE);

    return !ret;
}

/* luakit global table.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lfield font The default font.
 * \lfield font_height The default font height.
 * \lfield conffile The configuration file which has been loaded.
 */
static gint
luaH_luakit_index(lua_State *L)
{
    if(luaH_usemetatable(L, 1, 2))
        return 1;

    size_t len;
    widget_t *w;
    const gchar *prop = luaL_checklstring(L, 2, &len);
    luakit_token_t token = l_tokenize(prop, len);

    switch(token)
    {
      case L_TK_WINDOWS:
        lua_newtable(L);
        for (guint i = 0; i < globalconf.windows->len; i++) {
            w = globalconf.windows->pdata[i];
            luaH_object_push(L, w->ref);
            lua_rawseti(L, -2, i+1);
        }
        return 1;

      default:
        break;
    }
    return 0;
}

/* Newindex function for the luakit global table.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static gint
luaH_luakit_newindex(lua_State *L)
{
    if(luaH_usemetatable(L, 1, 2))
        return 1;

    size_t len;
    const gchar *buf = luaL_checklstring(L, 2, &len);

    debug("Luakit newindex %s", buf);

    return 0;
}

/* Add a global signal.
 * Returns the number of elements pushed on stack.
 * \luastack
 * \lparam A string with the event name.
 * \lparam The function to call.
 */
static gint
luaH_luakit_add_signal(lua_State *L)
{
    const gchar *name = luaL_checkstring(L, 1);
    luaH_checkfunction(L, 2);
    signal_add(globalconf.signals, name, luaH_object_ref(L, 2));
    return 0;
}

/* Remove a global signal.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A string with the event name.
 * \lparam The function to call.
 */
static gint
luaH_luakit_remove_signal(lua_State *L)
{
    const gchar *name = luaL_checkstring(L, 1);
    luaH_checkfunction(L, 2);
    gpointer func = (gpointer) lua_topointer(L, 2);
    signal_remove(globalconf.signals, name, func);
    luaH_object_unref(L, (void *) func);
    return 0;
}

/* Emit a global signal.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A string with the event name.
 * \lparam The function to call.
 */
static gint
luaH_luakit_emit_signal(lua_State *L)
{
    signal_object_emit(L, globalconf.signals, luaL_checkstring(L, 1), lua_gettop(L) - 1);
    return 0;
}

static gint
luaH_panic(lua_State *L)
{
    warn("unprotected error in call to Lua API (%s)", lua_tostring(L, -1));
    return 0;
}

static gint
luaH_quit(lua_State *L)
{
    (void) L;
    gtk_main_quit();
    return 0;
}

static gint
luaH_dofunction_on_error(lua_State *L)
{
    /* duplicate string error */
    lua_pushvalue(L, -1);
    /* emit error signal */
    signal_object_emit(L, globalconf.signals, "debug::error", 1);

    if(!luaL_dostring(L, "return debug.traceback(\"error while running function\", 3)"))
    {
        /* Move traceback before error */
        lua_insert(L, -2);
        /* Insert sentence */
        lua_pushliteral(L, "\nerror: ");
        /* Move it before error */
        lua_insert(L, -2);
        lua_concat(L, 3);
    }
    return 1;
}

void
luaH_init(xdgHandle *xdg)
{
    lua_State *L;

    static const struct luaL_reg luakit_lib[] = {
        { "quit", luaH_quit },
        { "add_signal", luaH_luakit_add_signal },
        { "remove_signal", luaH_luakit_remove_signal },
        { "emit_signal", luaH_luakit_emit_signal },
        { "__index", luaH_luakit_index },
        { "__newindex", luaH_luakit_newindex },
        { NULL, NULL }
    };

    /* Lua VM init */
    L = globalconf.L = luaL_newstate();

    /* Set panic fuction */
    lua_atpanic(L, luaH_panic);

    /* Set error handling function */
    lualib_dofunction_on_error = luaH_dofunction_on_error;

    luaL_openlibs(L);

    luaH_fixups(L);

    luaH_object_setup(L);

    /* Export luakit lib */
    luaH_openlib(L, "luakit", luakit_lib, luakit_lib);

    /* Export widget */
    widget_class_setup(L);

    /* add Lua search paths */
    lua_getglobal(L, "package");
    if(LUA_TTABLE != lua_type(L, 1)) {
        warn("package is not a table");
        return;
    }
    lua_getfield(L, 1, "path");
    if(LUA_TSTRING != lua_type(L, 2)) {
        warn("package.path is not a string");
        lua_pop(L, 1);
        return;
    }

    /* allows for testing luakit in the project directory
     * (i.e. `./luakit -c rc.lua ...` */
    lua_pushliteral(L, ";./lib/?.lua;");
    lua_pushliteral(L, ";./lib/?/init.lua");
    lua_concat(L, 2);

    /* add XDG_CONFIG_DIR as an include path */
    const gchar * const *xdgconfigdirs = xdgSearchableConfigDirectories(xdg);
    for(; *xdgconfigdirs; xdgconfigdirs++)
    {
        size_t len = l_strlen(*xdgconfigdirs);
        lua_pushliteral(L, ";");
        lua_pushlstring(L, *xdgconfigdirs, len);
        lua_pushliteral(L, "/luakit/?.lua");
        lua_concat(L, 3);

        lua_pushliteral(L, ";");
        lua_pushlstring(L, *xdgconfigdirs, len);
        lua_pushliteral(L, "/luakit/?/init.lua");
        lua_concat(L, 3);

        lua_concat(L, 3); /* concatenate with package.path */
    }

    /* add Lua lib path (/usr/share/luakit/lib by default) */
    lua_pushliteral(L, ";" LUAKIT_LUA_LIB_PATH "/?.lua");
    lua_pushliteral(L, ";" LUAKIT_LUA_LIB_PATH "/?/init.lua");
    lua_concat(L, 3); /* concatenate with package.path */
    lua_setfield(L, 1, "path"); /* package.path = "concatenated string" */
}

gboolean
luaH_loadrc(const gchar *confpath, gboolean run)
{
    lua_State *L = globalconf.L;

    if(!luaL_loadfile(L, confpath)) {
        if(run) {
            if(lua_pcall(L, 0, LUA_MULTRET, 0)) {
                g_fprintf(stderr, "%s\n", lua_tostring(L, -1));
            } else
                return TRUE;
        } else
            lua_pop(L, 1);
        return TRUE;
    } else
        g_fprintf(stderr, "%s\n", lua_tostring(L, -1));
    return FALSE;
}

/* Load a configuration file.
 *
 * param xdg An xdg handle to use to get XDG basedir.
 * param confpatharg The configuration file to load.
 * param run Run the configuration file.
 */
gboolean
luaH_parserc(xdgHandle* xdg, const gchar *confpatharg, gboolean run)
{
    gchar *confpath = NULL;
    gboolean ret = FALSE;

    /* try to load, return if it's ok */
    if(confpatharg) {
        debug("Attempting to load rc file: %s", confpatharg);
        if(luaH_loadrc(confpatharg, run))
            ret = TRUE;
        goto bailout;
    }
    confpath = xdgConfigFind("luakit/rc.lua", xdg);
    gchar *tmp = confpath;

    /* confpath is "string1\0string2\0string3\0\0" */
    while(*tmp) {
        debug("Loading rc file: %s", tmp);
        if(luaH_loadrc(tmp, run)) {
            globalconf.confpath = g_strdup(tmp);
            ret = TRUE;
            goto bailout;
        } else if(!run)
            goto bailout;
        tmp += l_strlen(tmp) + 1;
    }

bailout:

    if (confpath) g_free(confpath);
    return ret;
}

gint
luaH_class_index_miss_property(lua_State *L, lua_object_t *obj)
{
    (void) obj;
    signal_object_emit(L, globalconf.signals, "debug::index::miss", 2);
    return 0;
}

gint
luaH_class_newindex_miss_property(lua_State *L, lua_object_t *obj)
{
    (void) obj;
    signal_object_emit(L, globalconf.signals, "debug::newindex::miss", 3);
    return 0;
}

// vim: ft=c:et:sw=4:ts=8:sts=4:enc=utf-8:tw=80
