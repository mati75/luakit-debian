/*
 * widgets/textbutton.c - gtk button with label
 *
 * Copyright (C) 2010 Mason Larobina <mason.larobina@gmail.com>
 * Copyright (C) 2007-2009 Julien Danjou <julien@danjou.info>
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

#include "luah.h"
#include "widgets/common.h"

static gint
luaH_textbutton_index(lua_State *L, luakit_token_t token)
{
    widget_t *w = luaH_checkudata(L, 1, &widget_class);

    switch (token)
    {
      case L_TK_DESTROY:
        lua_pushcfunction(L, luaH_widget_destroy);
        return 1;

      case L_TK_LABEL:
        lua_pushstring(L, gtk_button_get_label(GTK_BUTTON(w->widget)));
        return 1;

      case L_TK_SHOW:
        lua_pushcfunction(L, luaH_widget_show);
        return 1;

      case L_TK_HIDE:
        lua_pushcfunction(L, luaH_widget_hide);
        return 1;

      default:
        break;
    }
    return 0;
}

static gint
luaH_textbutton_newindex(lua_State *L, luakit_token_t token)
{
    size_t len;
    widget_t *w = luaH_checkudata(L, 1, &widget_class);

    switch(token)
    {
      case L_TK_LABEL:
        gtk_button_set_label(GTK_BUTTON(w->widget),
            luaL_checklstring(L, 3, &len));
        break;

      default:
        return 0;
    }

    return luaH_object_emit_property_signal(L, 1);
}

static void
clicked_cb(GtkWidget *b, widget_t *w)
{
    (void) b;
    lua_State *L = globalconf.L;
    luaH_object_push(L, w->ref);
    luaH_object_emit_signal(L, -1, "clicked", 0, 0);
    lua_pop(L, 1);
}

static void
textbutton_destructor(widget_t *w)
{
    gtk_widget_destroy(w->widget);
}

widget_t *
widget_textbutton(widget_t *w)
{
    w->index = luaH_textbutton_index;
    w->newindex = luaH_textbutton_newindex;
    w->destructor = textbutton_destructor;

    w->widget = gtk_button_new();
    g_object_set_data(G_OBJECT(w->widget), "widget", (gpointer) w);
    gtk_button_set_focus_on_click(GTK_BUTTON(w->widget), FALSE);

    g_object_connect((GObject*)w->widget,
      "signal::clicked",         (GCallback)clicked_cb,    w,
      "signal::focus-in-event",  (GCallback)focus_cb,      w,
      "signal::focus-out-event", (GCallback)focus_cb,      w,
      "signal::parent-set",      (GCallback)parent_set_cb, w,
      NULL);

    gtk_widget_show(w->widget);
    return w;
}

// vim: ft=c:et:sw=4:ts=8:sts=4:enc=utf-8:tw=80
