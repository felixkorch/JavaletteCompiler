
function preExecuteLinux()
    os.execute("bnfc -m -l -p bnfc --cpp -o bnfc src/Javalette.cf && cp -f src/Javalette.y bnfc/")
    os.execute("bnfc --latex -o bnfc src/Javalette.cf")
    os.execute("flex -Pjavalette_ -o bnfc/Lexer.C bnfc/Javalette.l")
    os.execute("bison -t -pjavalette_ bnfc/Javalette.y -o bnfc/Parser.C")
end

function preExecuteWindows()
    os.execute("bnfc.exe -m -l -p bnfc --cpp -o bnfc src/Javalette.cf")
    os.execute("bnfc.exe --latex -o bnfc src/Javalette.cf")
    os.execute("xcopy /y src\\Javalette.y bnfc")
    os.execute("flex.exe -Pjavalette_ -o bnfc/Lexer.C bnfc/Javalette.l")
    os.execute("bison.exe -t -pjavalette_ bnfc/Javalette.y -o bnfc/Parser.C")
end

local install_name = "partA-2.tar.gz" -- Update each iteration

local function parse_args(args)
    if #args ~= 1 then
        error("Only one argument allowed")
    end

    local command = args[1]

    if command == "gen" then
        preExecuteLinux()
    elseif command == "install" then
        os.execute("mkdir -p install && cp Makefile install/ && cp bnfc/Javalette.tex install/doc/ && cp -rf doc lib src install/")
        os.execute("cd install && tar -czf " .. install_name .. " doc lib src Makefile")
    elseif command == "test" then
        os.execute("cd tester && python3 testing.py ../install/" .. install_name)
    elseif command == "clean" then
        os.execute("rm -rf bnfc > /dev/null")
    end

end

parse_args {...}