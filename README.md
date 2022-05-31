Javalette compiler
------------------

Description of the Javalette language is depicted in the included "Javalette.tex" in ./doc. <br>
Implemented features:
- Parsing
- Typechecking
- LLVM-Backend

Extensions
----------

-   Arrays
-   Multi-dimensional Arrays

Prerequisites:
--------------

-   bison (\>=3.8.2)
-   flex (\>=2.6.4)
-   bnfc (tested with v2.9.4)
-   g++/clang++ with C++17 support
-   llvm & llvm-dev packages (available from all pkg-managers)
-   python3 (only for testing)

Build instructions: Produces (in root): 'jlc'
------------------------------------------------------------
```
make
```
and to remove build artefacts,
```
make clean
```

Usage (from root):
------------------
```
./jlc <input-file.jl>
```

-   If the input arg is invalid, the program will exit with code 1.
-   If the input arg is empty, the program will start reading from std
    in.
-   A parser-error will print: "ERROR: Parse error on line x"
-   The compiler will then attempt to run the typechecking module, which
    on failed attempt will print the corresponding error to std err
    prepended with 'ERROR:' On success, "OK" will be printed to std err.
-   If type-checking succeeds, the compiler will compile the program to
    LLVM IR and stream it to std-out.

Parsing conflicts:
------------------

There is just 1 shift/reduce conflict, which is the standard dangling
'else', which is present in other languages like C/Java. The bison
parser-generator confirms this and reports it in the logs. Bison as
other parser-generators, will choose the shift action which in practice
means that the 'else'-statement will be attached to the inner 'if' like
so:

if (expr) { if (expr) stmt ; else stmt ;}
