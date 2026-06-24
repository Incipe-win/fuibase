add_rules("mode.debug", "mode.release")
set_languages("c++20")

target("fuibase-demo")
    set_kind("binary")
    add_files("examples/demo.cpp")
    add_includedirs("include")
    set_targetdir("$(projectdir)/build")

    if is_plat("linux") then add_links("stdc++fs") end
    if is_mode("release") then set_optimize("fastest"); set_strip("all") end
    if is_mode("debug")   then set_symbols("debug") end
