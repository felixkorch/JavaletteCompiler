
function gen()
    os.execute("bnfc -m -l -p bnfc --cpp -o bnfc src/Javalette.cf && cp -f src/Javalette.y bnfc/")
    os.execute("bnfc --latex -o bnfc src/Javalette.cf")
    os.execute("flex -Pjavalette_ -o bnfc/Lexer.C bnfc/Javalette.l")
    os.execute("bison -t -pjavalette_ bnfc/Javalette.y -o bnfc/Parser.C")
end

local install_name = "partB-1.tar.gz" -- Update each iteration

local function parse_args(args)
    if #args ~= 1 then
        error("Only one argument allowed")
    end

    local command = args[1]

    if command == "gen" then
        gen()
    elseif command == "install" then
        os.execute("mkdir -p install && cp -rf doc lib src install/ && cp Makefile install/ && cp bnfc/Javalette.tex install/doc/")
        os.execute("cd install && tar -czf " .. install_name .. " doc lib src Makefile")
    elseif command == "test" then
        os.execute("cd tester && python3 testing.py --llvm ../install/" .. install_name)
    elseif command == "clean" then
        os.execute("rm -rf bnfc install > /dev/null")
    end

end

parse_args {...}
