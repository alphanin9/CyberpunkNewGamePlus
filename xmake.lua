set_project("New Game+")
set_version("1.3.1", { build = "%y%m%d%H" })

set_plat("windows")
set_arch("x64")
set_languages("c++latest")

set_symbols("debug")
set_strip("all")
set_optimize("fastest")
set_runtimes("MD")
add_cxxflags("/GR-")

-- RED4ext.SDK, RedLib, ArchiveXL, TweakXL and SharedPunk come from the CP2077
-- package registry.
add_repositories("cp2077-repo https://gitlab.com/alphanin9/cp2077-xmake-repo.git")

add_requires("lz4", "hopscotch-map", "safetyhook", "semver", "wil")
add_requires("red4ext-sdk", "red-lib", "archive-xl", "tweak-xl", "sharedpunk")

local cp2077_path = os.getenv("CP2077_PATH")

target("New Game+")
    set_default(true)
    set_kind("shared")
    set_filename("NewGamePlus.dll")
    set_warnings("more")
    add_files("src/**.cpp", "src/**.rc")
    add_headerfiles("src/**.hpp")
    add_includedirs("src/")
    add_packages("lz4", "hopscotch-map", "safetyhook", "semver", "wil",
                 "red4ext-sdk", "red-lib", "archive-xl", "tweak-xl", "sharedpunk")
    add_syslinks("Version", "User32")
    add_defines("WINVER=0x0601", "WIN32_LEAN_AND_MEAN", "NOMINMAX")
    set_configdir("src")
    add_configfiles("config/ProjectTemplate.hpp.in", { prefixdir = "Config" })
    add_configfiles("config/ProjectMetadata.rc.in", { prefixdir = "Config" })
    set_configvar("NAME", "New Game+")
    set_configvar("DESC", "New Game+ for Cyberpunk 2077")
    set_configvar("AUTHOR_NAME", "not_alphanine")
    add_cxxflags("/Oi", "/Os", "/GL")
    add_ldflags("/LTCG")
    set_rundir(path.join(cp2077_path, "bin", "x64"))
    on_package( function (target)
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
        local target_file = target:targetfile()
        local plugin_folder = path.join(cp2077_path, "red4ext/plugins/NewGamePlus/")

        os.mkdir(plugin_folder)

        os.cp(target_file, plugin_folder)
        os.cp(path.join(
            path.directory(target_file),
            path.basename(target_file) .. ".pdb" -- Evil hack #2
        ), plugin_folder)

        cprint("${bright green}Installed plugin to " .. plugin_folder)
    end)
    on_run( function (target)
        os.run(path.join(cp2077_path, "bin", "x64", "Cyberpunk2077.exe"))
    end)

add_rules("plugin.vsxmake.autoupdate")