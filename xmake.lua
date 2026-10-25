set_project("New Game+")
set_version("1.4.0", { build = "%y%m%d%H" })

-- Windows x64 stays the default and the release target. On an Apple Silicon Mac the platform follows the host:
--   xmake f -p macosx -a arm64 -m release -y && xmake
if is_host("macosx") then
    set_plat("macosx")
    set_arch("arm64")
else
    set_plat("windows")
    set_arch("x64")
end

if is_plat("macosx") then
    set_languages("c++20")
    -- Hidden by default: Main, Query, Supports and ngplus_registerNatives are exported explicitly.
    set_symbols("hidden")
    set_strip("debug")
    set_optimize("fastest")
else
    set_languages("c++latest")
    set_symbols("debug")
    set_strip("all")
    set_optimize("fastest")
    set_runtimes("MD")
    add_cxxflags("/GR-")
end

-- RED4ext.SDK, RedLib, ArchiveXL, TweakXL and SharedPunk come from the CP2077
-- package registry.
add_repositories("cp2077-repo https://github.com/alphanin9/cp2077-xmake-repo.git")

add_requires("lz4", "hopscotch-map")
if is_plat("windows") then
    add_requires("safetyhook", "semver", "wil")
else
    -- No safetyhook (x86 only), semver (ProjectTemplate.hpp has its own parser on Apple) or wil there.
    -- nlohmann_json reads the save's metadata.9.json (macos/NgpMetadata.cpp).
    add_requires("nlohmann_json")
end
-- On macosx the cp2077-repo packages below resolve to jackmaxwil's macOS SDK and to the patched
-- RedLib / ArchiveXL / TweakXL headers (see the repo's macosx branches).
add_requires("red4ext-sdk", "red-lib", "archive-xl", "tweak-xl", "sharedpunk")

local cp2077_path = os.getenv("CP2077_PATH")

target("New Game+")
    set_default(true)
    set_kind("shared")
    set_warnings("more")
    add_files("src/**.cpp")
    add_headerfiles("src/**.hpp")
    add_includedirs("src/")
    add_packages("lz4", "hopscotch-map",
                 "red4ext-sdk", "red-lib", "archive-xl", "tweak-xl", "sharedpunk")
    if is_plat("windows") then
        set_filename("NewGamePlus.dll")
        add_files("src/**.rc")
        add_packages("safetyhook", "semver", "wil")
        add_syslinks("Version", "User32")
        add_defines("WINVER=0x0601", "WIN32_LEAN_AND_MEAN", "NOMINMAX")
    else
        set_filename("NewGamePlus.dylib")
        add_packages("nlohmann_json")
        -- The Windows-only save parser and test definitions are not part of the Mac target.
        remove_files("src/Parsing/New/**.cpp", "src/GameSystem/TestingDefs.cpp")
        -- Mac glue: session description, save metadata, feature gate, GameInstance chain, host shims.
        add_files("macos/*.cpp")
        -- macos/inc first: the game-system header with the Mac IsSavingLocked ABI (until the SDK has it).
        add_includedirs("macos", "macos/inc")
        add_cxxflags("-fexceptions", "-fms-extensions", "-fvisibility-inlines-hidden",
                     "-Wno-deprecated-declarations", "-Wno-microsoft-template", "-Wno-invalid-offsetof",
                     "-ffile-prefix-map=" .. os.projectdir() .. "=new_game_plus", { force = true })
        -- Completes the SDK types Handle<T> destroys (see the header).
        add_cxxflags("-include", path.join(os.projectdir(), "macos", "MacPrelude.hpp"), { force = true })
        -- The game's symbols are bound at load. macos/check-undefined.sh (below) fails the build when a plugin
        -- symbol is left over, so a function that is declared and never compiled in does not hide here.
        add_shflags("-Wl,-undefined,dynamic_lookup", { force = true })
        -- Only the four plugin entry points are exported. The packages (SharedPunk, lz4) are built without
        -- -fvisibility=hidden and would otherwise leak their symbols, which the CMake build never did.
        add_shflags("-Wl,-exported_symbols_list," .. path.join(os.projectdir(), "macos", "exports.txt"), { force = true })
    end
    set_configdir("src")
    add_configfiles("config/projectTemplate.hpp.in", { prefixdir = "Config" })
    add_configfiles("config/projectMetadata.rc.in", { prefixdir = "Config" })
    set_configvar("NAME", "New Game+")
    set_configvar("DESC", "New Game+ for Cyberpunk 2077")
    set_configvar("AUTHOR_NAME", "not_alphanine")
    if is_plat("windows") then
        add_cxxflags("/Oi", "/Os", "/GL")
        add_ldflags("/LTCG")
        set_rundir(path.join(cp2077_path, "bin", "x64"))
    else
        -- Ad hoc signature, then the export / undefined-symbol check.
        after_link( function (target)
            local script_dir = path.join(os.projectdir(), "macos")
            os.execv("bash", {path.join(script_dir, "fix-install-names.sh"), target:targetfile()})
            os.execv("bash", {path.join(script_dir, "check-undefined.sh"), target:targetfile()})
        end)
    end
    on_package( function (target)
        if not target:is_plat("windows") then
            return -- The Mac layout (Scripts/, archive/Mac/mod) is not packaged by xmake yet, see macos/README.md
        end
        os.rm("packaging/*")
        os.rm("packaging_pdb/*")

        os.mkdir("packaging/red4ext/plugins/NewGamePlus")
        os.mkdir("packaging/red4ext/plugins/NewGamePlus/redscript")
        os.mkdir("packaging/red4ext/plugins/NewGamePlus/tweaks")

        os.cp("LICENSE", "packaging/red4ext/plugins/NewGamePlus")
        os.cp("THIRDPARTY_LICENSES", "packaging/red4ext/plugins/NewGamePlus")

        os.cp("wolvenkit/packed/archive/pc/mod/*", "packaging/red4ext/plugins/NewGamePlus")
        os.cp("scripting/*", "packaging/red4ext/plugins/NewGamePlus/redscript")
        os.cp("tweaks/*", "packaging/red4ext/plugins/NewGamePlus/tweaks")

        local target_file = target:targetfile()

        os.cp(target_file, "packaging/red4ext/plugins/NewGamePlus")
        os.mkdir("packaging_pdb/red4ext/plugins/NewGamePlus")

        os.cp(path.join(
            path.directory(target_file),
            path.basename(target_file) .. ".pdb" -- Evil hack
        ), "packaging_pdb/red4ext/plugins/NewGamePlus")
    end)
    on_install( function (target)
        if not target:is_plat("windows") then
            return
        end
        local target_file = target:targetfile()
        local plugin_folder = path.join(cp2077_path, "red4ext/plugins/NewGamePlus/")

        os.mkdir(plugin_folder)

        os.cp(target_file, plugin_folder)
        os.cp(path.join(
            path.directory(target_file),
            path.basename(target_file) .. ".pdb" -- Evil hack #2
        ), plugin_folder)

        -- Wire up the redscript dir if it isn't there yet, so scripts don't have to be
        -- copied over by hand after every edit. Borrowed from VedalAI/neuro-cyberpunk.
        local redscript_folder = path.join(plugin_folder, "redscript")

        if not os.exists(redscript_folder) then
            -- A real link means edits made in the game directory land back in the repo.
            -- Windows only allows directory symlinks under Developer Mode (or elevation),
            -- so fall back to a plain copy when that isn't available.
            local linked = try { function ()
                os.ln(path.absolute("scripting"), redscript_folder)
                return true
            end }

            if linked then
                cprint("${bright green}Linked " .. redscript_folder .. " -> scripting/")
            else
                os.cp(path.absolute("scripting"), redscript_folder)
                cprint("${bright yellow}Could not symlink the redscript dir (needs Developer Mode), copied it instead")
            end
        elseif not os.islink(redscript_folder) then
            cprint("${bright yellow}" .. redscript_folder .. " is a plain directory, so scripts are not being linked. " ..
                   "Delete it and re-run xmake install to replace it with a link to scripting/")
        end

        cprint("${bright green}Installed plugin to " .. plugin_folder)
    end)
    on_run( function (target)
        if not target:is_plat("windows") then
            return
        end
        os.run(path.join(cp2077_path, "bin", "x64", "Cyberpunk2077.exe"))
    end)

if is_plat("windows") then
    add_rules("plugin.vsxmake.autoupdate")
end
