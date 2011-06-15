#!/usr/bin/env lua

-- build-utils/gentokens.lua - gen tokenize lib
--
-- Copyright © 2010 Mason Larobina <mason.larobina@gmail.com>
--
-- This program is free software: you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program.  If not, see <http://www.gnu.org/licenses/>.

tokenize_h = [[
/* This file is autogenerated by build-utils/gentokens.lua */
#ifndef LUAKIT_COMMON_TOKENIZE_H
#define LUAKIT_COMMON_TOKENIZE_H

#include <glib/gtypes.h>

typedef enum luakit_token_t {
    L_TK_UNKNOWN=0, /* (luakit_token_t) NULL == L_TK_UNKNOWN */
    %s
} luakit_token_t;

__attribute__((pure)) enum luakit_token_t l_tokenize(const gchar *s);

#endif
]]

tokenize_c = [[
/* This file is autogenerated by build-utils/gentokens.lua */

#include <glib/ghash.h>
#include "common/tokenize.h"

static GHashTable *tokens = NULL;

typedef struct {
    luakit_token_t tok;
    const gchar *name;
} token_map_t;

token_map_t tokens_table[] = {
    %s
    { 0, NULL },
};

luakit_token_t l_tokenize(const gchar *s)
{
    if (!tokens) {
        tokens = g_hash_table_new(g_str_hash, g_str_equal);
        for (token_map_t *t = tokens_table; t->name; t++)
            g_hash_table_insert(tokens, (gpointer) t->name, (gpointer) t->tok);
    }

    return (luakit_token_t) g_hash_table_lookup(tokens, s);
}
]]

if #arg ~= 2 then
    error("invalid args, usage: gentokens.lua [token list] [out.c/out.h]")
end

-- Load list of tokens
tokens = {}
for token in io.lines(arg[1]) do
    if #token > 0 then
        if not string.match(token, "^[%w_]+$") then
            error(string.format("invalid token: %q", token))
        end
        tokens["L_TK_" .. string.upper(token)] = token
    end
end

if string.match(arg[2], "%.h$") then
    -- Gen list of tokens
    enums = {}
    for k, _ in pairs(tokens) do
        table.insert(enums, string.format("%s,", k))
    end
    table.sort(enums)

    -- Write header file
    fh = io.open(arg[2], "w")
    fh:write(string.format(tokenize_h, table.concat(enums, "\n    ")))
    fh:close()

elseif string.match(arg[2], "%.c$") then
    -- Gen table of { token, "literal" }
    tok_table = {}
    for k, v in pairs(tokens) do
        table.insert(tok_table, string.format('{ %s, "%s" },', k, v))
    end

    -- Write source file
    fh = io.open(arg[2], "w")
    fh:write(string.format(tokenize_c, table.concat(tok_table, "\n    ")))
    fh:close()

else
    error("Unknown action for file: " .. arg[2])
end
