-- Modules.lua
--
-- This file defines the modules of mupfel. Currently there exist these:
-- * core (the mupfel engine, library only)
-- * app (a demo application aiming to show all of mupfel's functionality)
-- * tests (a testframework based on catch2)
--
--   premake5 --file=Build.lua vs2026                     -- everything
--   premake5 --file=Build.lua --modules=app vs2026       -- engine + game only
--   premake5 --file=Build.lua --modules=tests vs2026     -- engine + unit tests
--
-- Names are the keys of the Modules table below, or "all"; anything else aborts generation with the
-- valid list rather than silently producing a solution missing a project. Order and whitespace don't
-- matter and the parse is case-insensitive ("--modules=App, CORE" is fine).
--
-- The selection is closed over `requires`, so asking for a module always brings in what it links
-- against. App and Tests link Core, which means **Core is part of every possible
-- selection** -- and so is every vendored library Core links (Ping, spdlog, imgui, box2d), which is
-- why Vendor/Build-Vendor.lua only bothers gating catch2.
--
-- Adding a module later is one entry below plus its Build-*.lua; nothing else in the build reads the
-- option directly.

newoption {
    trigger     = "modules",
    value       = "LIST",
    default     = "all",
    description = "Comma-separated modules to generate: all, core, app, tests (default: all)",
}

-- group        -- workspace group the project is filed under; "" for the top level (App's spot).
-- build_script -- the file Build.lua includes to define it.
-- requires     -- modules that must be in the solution too, because this one links them.
-- startable    -- can be the solution's startup project. Core can't: it's a static lib.
Modules = {
    core = {
        project      = "Core",
        group        = "Engine",
        build_script = "Core/Build-Core.lua",
        requires     = {},
    },
    app = {
        project      = "App",
        group        = "",
        build_script = "App/Build-App.lua",
        requires     = { "core" },
        startable    = true,
    },
    tests = {
        project      = "Tests",
        group        = "Tests",
        build_script = "Tests/Build-Tests.lua",
        requires     = { "core" },
        startable    = true,
    },
}

-- Fixed order for group emission, the startproject fallback and error messages. Lua table iteration
-- is unordered, so nothing may walk Modules with pairs() and expect a stable solution layout.
ModuleOrder = { "core", "app", "tests" }

local selected = {}

local function select_module(name)
    if selected[name] then
        return
    end
    if not Modules[name] then
        error("Unknown module '" .. name .. "' passed to --modules. Valid values: all, "
              .. table.concat(ModuleOrder, ", "), 0)
    end
    selected[name] = true
    for _, required in ipairs(Modules[name].requires) do
        select_module(required)
    end
end

-- _OPTIONS is already populated from the command line by the time this script is loaded; the `or`
-- covers premake applying the option default later than we read it.
for name in (_OPTIONS["modules"] or "all"):gmatch("[^,]+") do
    name = name:match("^%s*(.-)%s*$"):lower()
    if name == "all" then
        for _, module in ipairs(ModuleOrder) do
            select_module(module)
        end
    elseif name ~= "" then
        select_module(name)
    end
end

if not next(selected) then
    error("--modules selected nothing. Valid values: all, " .. table.concat(ModuleOrder, ", "), 0)
end

-- True if `name` is part of this solution. Every conditional in the build scripts asks through this
-- rather than reading _OPTIONS itself, so the `requires` closure above can't be bypassed.
function ModuleSelected(name)
    return selected[name] == true
end

-- The selected modules, in ModuleOrder order.
function SelectedModules()
    local result = {}
    for _, name in ipairs(ModuleOrder) do
        if selected[name] then
            table.insert(result, name)
        end
    end
    return result
end

-- Project the IDE should start on: App when it's in (the historical default), otherwise the first
-- startable module. Returns nil for a Core-only solution, which has nothing runnable in it -- and
-- premake must then not be handed a startproject naming a project that doesn't exist.
function StartProjectName()
    for _, name in ipairs(ModuleOrder) do
        if selected[name] and Modules[name].startable then
            return Modules[name].project
        end
    end
    return nil
end

-- Echoed because --modules silently changes what the solution contains; without this a stale flag in
-- someone's shell history looks like projects vanishing on their own.
print("Modules: " .. table.concat(SelectedModules(), ", "))
