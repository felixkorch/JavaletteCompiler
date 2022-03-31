function preExecuteWindows()
    os.execute("bnfc-2.9.4.exe -m -l -p bnfc --cpp -o bnfc src/Javalette.cf")
    os.execute("xcopy /y src\\Javalette.y bnfc")
    os.execute(".\\win_flex\\win_flex.exe -Pjavalette_ -o bnfc/Lexer.C bnfc/Javalette.l")
    os.execute(".\\win_flex\\win_bison.exe -t -pjavalette_ bnfc/Javalette.y -o bnfc/Parser.C")
end

workspace "JavaletteCompiler"
    architecture "x86_64"
    startproject "jlc"

    configurations {
        "Debug",
        "Release"
    }

    flags {
        "MultiProcessorCompile"
    }

    compileas "C++"

    outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

    project "jlc-lib"
        kind "StaticLib"
        staticruntime "off"
        language "C++"
        cppdialect "C++11"

        targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
        objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

        files {
            "src/**.h",
            "src/**.cpp",
            "bnfc/**.C",
            "bnfc/**.H"
        }

        removefiles { "src/Main.cpp", "bnfc/Test.C"}

        filter "system:not windows"
            prebuildcommands { "bnfc -m -l -p bnfc --cpp -o bnfc src/Javalette.cf && cp -f src/Javalette.y bnfc/"}
            prebuildcommands { "flex -Pjavalette_ -o bnfc/Lexer.C bnfc/Javalette.l"}
            prebuildcommands { "bison -t -pjavalette_ bnfc/Javalette.y -o bnfc/Parser.C" }

        includedirs {
            "%{wks.location}"
        }


    project "jlc"
        kind "ConsoleApp"
        language "C++"
        cppdialect "C++11"

        targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
        objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

        files {
            "src/Main.cpp"
        }

        includedirs {
            "%{wks.location}"
        }

        links {
            "jlc-lib"
        }

    project "sandbox"
        kind "ConsoleApp"
        language "C++"
        cppdialect "C++11"

        targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
        objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

        files {
            "sandbox/Box1.cpp"
        }

        includedirs {
            "%{wks.location}"
        }

        links {
            "jlc-lib"
        }