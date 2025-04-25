-- set minimum xmake version
set_xmakever("2.9.8")

-- set project
set_project("caprica")
set_languages("c++23")
set_license("MIT")
set_version("0.3.0")

-- require packages
add_requires("boost", { configs = { filesystem = true, program_options = true, container = true } })
add_requires("pugixml")

namespace("caprica", function()
    -- define targets
    target("caprica", function()
        set_kind("$(kind)")

        -- bind package dependencies
        add_packages("boost", "pugixml", { public = true })

        -- add all source files
        add_files("caprica/**/**.cpp")

        if is_kind("binary") then
            add_files("caprica/**.cpp")
        end

        -- add all header files
        add_includedirs("caprica", { public = true })
        add_headerfiles("(caprica/**.h)")

        -- add flags
        add_cxxflags("cl::/Zc:inline", "cl::/bigobj")
    end)
end)
