set_project("New Game+")
set_version("1.1.1", {build="%y%m%d%h"})

set_arch("x64")
set_languages("c++latest")
add_cxxflags("/MP /GR- /EHsc")

-- Make the binary small, optimized and not static linked
set_symbols("debug")
set_strip("all")
set_optimize("faster")
add_cxxflags("/Zi /Ob2 /Oi /GL")
set_runtimes("MD")

add_requires("lz4", "minhook", "wil")

target("New Game+")
    set_default(true)
    set_kind("shared")
    set_filename("NewGamePlus.dll")
    set_warnings("more")
    add_files("src/**.cpp", "src/**.rc")
    add_headerfiles("src/**.hpp")
    add_includedirs("src/")
    add_deps("red4ext.sdk", "redlib", "archivexl", "tweakxl")
    add_packages("lz4", "minhook", "wil")
    add_syslinks("Version", "User32")
    add_defines("WINVER=0x0601", "WIN32_LEAN_AND_MEAN", "NOMINMAX")
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
        
        local targetfile = target:targetfile()

        os.cp(targetfile, "packaging/red4ext/plugins/NewGamePlus")
        os.mkdir("packaging_pdb/red4ext/plugins/NewGamePlus")

        os.cp(path.join(
            path.directory(targetfile), 
            path.basename(targetfile)..".pdb" -- Evil hack
        ), "packaging_pdb/red4ext/plugins/NewGamePlus")
    end)

target("red4ext.sdk")
    set_default(false)
    set_kind("headeronly")
    set_group("deps")
    add_headerfiles("deps/red4ext.sdk/include/**.hpp")
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