set_project("New Game+")
set_version("1.2.0", {build="%y%m%d%H"})

set_plat("windows")
set_arch("x64")
set_languages("c++latest")

set_symbols("debug")
set_strip("all")
set_optimize("fastest")
set_runtimes("MD")
add_cxxflags("/GR-")

add_requires("lz4", "hopscotch-map", "safetyhook", "semver", "wil")

includes("deps/sharedpunk/xmake.lua")

local cp2077_path = os.getenv("CP2077_PATH")

target("New Game+")
    set_default(true)
    set_kind("shared")
    set_filename("NewGamePlus.dll")
    set_warnings("more")
    add_files("src/**.cpp", "src/**.rc")
    add_headerfiles("src/**.hpp")
    add_includedirs("src/")
    add_deps("cp2077-shared-data", "red4ext.sdk", "redlib", "archivexl", "tweakxl")
    add_packages("lz4", "hopscotch-map", "safetyhook", "semver", "wil")
    add_syslinks("Version", "User32")
    add_defines("WINVER=0x0601", "WIN32_LEAN_AND_MEAN", "NOMINMAX")
    set_configdir("src")
    add_configfiles("config/ProjectTemplate.hpp.in", {prefixdir="Config"})
    add_configfiles("config/ProjectMetadata.rc.in", {prefixdir="Config"})
    set_configvar("NAME", "New Game+")
    set_configvar("DESC", "New Game+ for Cyberpunk 2077")
    set_configvar("AUTHOR_NAME", "not_alphanine")
    add_cxxflags("/Oi", "/Os", "/GL")
    add_ldflags("/LTCG")
    set_rundir(path.join(cp2077_path, "bin", "x64"))
    on_package(function(target)
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
            path.basename(target_file)..".pdb" -- Evil hack
        ), "packaging_pdb/red4ext/plugins/NewGamePlus")
    end)
    on_install(function(target)
        local target_file = target:targetfile()
        local plugin_folder = path.join(cp2077_path, "red4ext/plugins/NewGamePlus/")

        os.mkdir(plugin_folder)

        os.cp(target_file, plugin_folder)
        os.cp(path.join(
            path.directory(target_file),
            path.basename(target_file)..".pdb" -- Evil hack #2
        ), plugin_folder)

        cprint("${bright green}Installed plugin to "..plugin_folder)
    end)
    on_run(function(target)
        os.run(path.join(cp2077_path, "bin", "x64", "Cyberpunk2077.exe"))
    end)

target("red4ext.sdk")
    set_default(false)
    set_kind("headeronly")
    set_group("deps")
    add_headerfiles("deps/red4ext.sdk/vendor/**.h")
    add_headerfiles("deps/red4ext.sdk/include/**.hpp")
    add_includedirs("deps/red4ext.sdk/vendor/", { public = true })
    add_includedirs("deps/red4ext.sdk/include/", { public = true })

target("redlib")
    set_default(false)
    set_kind("headeronly")
    set_group("deps")
    add_defines("NOMINMAX")
    add_headerfiles("deps/redlib/vendor/**.hpp")
    add_headerfiles("deps/redlib/include/**.hpp")
    add_includedirs("deps/redlib/vendor/", { public = true })
    add_includedirs("deps/redlib/include/", { public = true })

target("archivexl")
    set_default(false)
    set_kind("headeronly")
    set_group("deps")
    add_headerfiles("deps/archivexl/support/red4ext/**.hpp")
    add_includedirs("deps/archivexl/support/red4ext/", { public = true })

target("tweakxl")
    set_default(false)
    set_kind("headeronly")
    set_group("deps")
    add_headerfiles("deps/tweakxl/support/red4ext/**.hpp")
    add_includedirs("deps/tweakxl/support/red4ext/", { public = true })

add_rules("plugin.vsxmake.autoupdate")