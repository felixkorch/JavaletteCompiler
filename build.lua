
function find_replace()
    --
    --  Read the file
    --
    local f = io.open("bnfc/Javalette.y", "r")
    local content = f:read("*all")
    f:close()

    --
    -- Edit the string
    --
    content = string.gsub(content, "error:", "ERROR:")
    content = string.gsub(content, "Bison.H", "bnfc/Bison.H")

    --
    -- Write it out
    --
    local f = io.open("bnfc/Javalette.y", "w")
    f:write(content)
    f:close()
end

function gen()
    os.execute("bnfc -l -p bnfc --cpp -o bnfc src/Frontend/Javalette.cf")
    find_replace()
    os.execute("cp -f bnfc/Javalette.y src/Frontend/")
    os.execute("bnfc --latex -o bnfc src/Frontend/Javalette.cf")
    os.execute("flex -Pjavalette_ -o bnfc/Lexer.C bnfc/Javalette.l")
    os.execute("bison -t -pjavalette_ bnfc/Javalette.y -o bnfc/Parser.C")
end

local install_name = "partC-1.tar.gz" -- Update each iteration

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
        os.execute("cd tester && python3 testing.py ../install/" .. install_name .. " --llvm -x arrays1 arrays2")
    elseif command == "clean" then
        os.execute("rm -rf bnfc install > /dev/null")
    end

end

parse_args {...}
