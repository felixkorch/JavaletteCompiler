function preExecuteLinux()
    os.execute("bnfc -m -l -p bnfc --cpp -o bnfc src/Javalette.cf && cp -f src/Javalette.y bnfc/")
    os.execute("flex -Pjavalette_ -o bnfc/Lexer.C bnfc/Javalette.l")
    os.execute("bison -t -pjavalette_ bnfc/Javalette.y -o bnfc/Parser.C")
end

local install_name = "partA-2.tar.gz" -- Update each iteration

if _ACTION == "codelite" then
    preExecuteLinux()
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
        
    newaction {
	   trigger     = "install",
	   description = "Install the software",
	   execute = function ()
            os.execute("mkdir -p install && cp Makefile.bk install/Makefile && cp doc lib src install/")
	        os.execute("cd install && tar -czf " .. install_name .. " doc lib src Makefile")
	   end
	}

    newaction {
        trigger     = "clean",
        description = "Clean build artefacts",
        execute = function ()
            os.execute("rm -f *.project")
            os.execute("rm -f *.workspace")
            os.execute("rm -f " .. install_name)
            os.execute("rm -rf bnfc > /dev/null")
        end
     }
 