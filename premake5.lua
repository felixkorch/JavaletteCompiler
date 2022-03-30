workspace "JavaletteCompiler"
    architecture "x86_64"
    startproject "jlc"

    configurations
    {
        "Debug",
        "Release"
    }

    flags
    {
        "MultiProcessorCompile"
    }

    outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

    project "jlc-lib"
    kind "StaticLib"
    language "C++"
    cppdialect "C++11"

    prebuildcommands {
        "bnfc -m -l -p bnfc --cpp -o bnfc src/Javalette.cf",
        "flex -Pjavalette_ -o bnfc/Lexer.C bnfc/Javalette.l",
        "bison -t -pjavalette_ bnfc/Javalette.y -o bnfc/Parser.C && mv Bison.H bnfc/" }

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/build/" .. outputdir .. "/%{prj.name}")

    files
    {
        "src/**.h",
        "src/**.cpp",
        "bnfc/**.C",
        "bnfc/**.H"
    }

    removefiles { "src/Main.cpp" }

    includedirs
    {
        "%{wks.location}"
    }


    project "jlc"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++11"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/build/" .. outputdir .. "/%{prj.name}")

    files
    {
        "src/Main.cpp"
    }

    includedirs
    {
        "%{wks.location}"
    }

    links
    {
        "jlc-lib"
    }

    project "sandbox"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++11"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/build/" .. outputdir .. "/%{prj.name}")

    files
    {
        "sandbox/Box1.cpp"
    }

    includedirs
    {
        "%{wks.location}"
    }

    links
    {
        "jlc-lib"
    }