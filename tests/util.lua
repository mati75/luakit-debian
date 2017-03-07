--- Utilities for use in tests.
--
-- @module tests.util
-- @copyright 2017 Aidan Holm

local posix = require('posix')
local os = require('os')
local filter_array = require("lib.lousy.util").table.filter_array

local M = {}

local function path_is_in_directory(path, dir)
    if path == "." then return true end
    return string.find(path, "^"..dir)
end

local children = {}
function M.spawn(args)
    assert(type(args) == "table" and #args > 0)

    local child = posix.fork()
    if child == 0 then
        local _, err = posix.execx(args)
        print("execx:", err)
        os.exit(0)
    end
    table.insert(children, child)
    return child
end

function M.cleanup()
    for _, child in ipairs(children) do
        posix.wait(child)
    end
end

local git_files

local function get_git_files ()
    if not git_files then
        git_files = {}
        local f = io.popen("git ls-files")
        for line in f:lines() do
            table.insert(git_files, line)
        end
        f:close()
    end

    return git_files
end

function M.find_files(dirs, pattern, excludes)
    assert(type(dirs) == "string" or type(dirs) == "table",
        "Bad search location: expected string or table")
    assert(type(pattern) == "string", "Bad pattern")
    assert(excludes == nil or type(excludes) == "table", "Bad exclusion list")

    if type(dirs) == "string" then dirs = { dirs } end
    for _, dir in ipairs(dirs) do
        assert(type(dir) == "string", "Each search location must be a string")
    end

    if excludes == nil then excludes = {} end

    -- Get list of files tracked by git
    get_git_files()

    -- Filter to those inside the given directories
    local file_list = {}
    for _, file in ipairs(git_files) do
        local dir_match = false
        for _, dir in ipairs(dirs) do
            dir_match = dir_match or path_is_in_directory(file, dir)
        end
        local pat_match = string.find(file, pattern)
        if dir_match and pat_match then
            table.insert(file_list, file)
        end
    end

    -- Remove all files in excludes
    if excludes then
        file_list = filter_array(file_list, function (_, file)
            for _, exclude_pat in ipairs(excludes) do
                if string.find(file, exclude_pat) then return false end
            end
            return true
        end)
    end

    -- Return filtered list
    return file_list
end

function M.format_file_errors(entries)
    assert(type(entries) == "table")

    local sep = "    "

    -- Find file alignment length
    local align = 0
    get_git_files()
    for _, file in ipairs(git_files) do
        align = math.max(align, file:len())
    end

    -- Build output
    local lines = {}
    local prev_file = nil
    for _, entry in ipairs(entries) do
        local file = entry.file ~= prev_file and entry.file or ""
        prev_file = entry.file
        local line = string.format("%-" .. tostring(align) .. "s%s%s", file, sep, entry.err)
        table.insert(lines, line)
    end
    return table.concat(lines, "\n")
end

return M

-- vim: et:sw=4:ts=8:sts=4:tw=80
