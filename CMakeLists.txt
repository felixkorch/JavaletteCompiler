project(jlc)

cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 17)

if(!MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
    set(THREADS_PREFER_PTHREAD_FLAG ON)
endif()

set(bnfc_SOURCE
    bnfc/Absyn.C
    bnfc/Absyn.H
    bnfc/Buffer.C
    bnfc/Buffer.H
    bnfc/Parser.C
    bnfc/Lexer.C
    bnfc/Parser.H
    bnfc/ParserError.H
    bnfc/Printer.C
    bnfc/Printer.H
    bnfc/Bison.H
)

set(jlc-lib_SOURCE ${bnfc_SOURCE}
        src/Common/BaseVisitor.cpp
        src/Common/BaseVisitor.h
        src/Frontend/TypeChecker.cpp
        src/Frontend/TypeChecker.h
        src/Frontend/TypeError.h
        src/LLVM-Backend/CodeGen.h
        src/LLVM-Backend/CodeGen.cpp
        src/LLVM-Backend/BinOpBuilder.cpp
        src/LLVM-Backend/BinOpBuilder.h
        src/LLVM-Backend/CodegenEnv.cpp
        src/LLVM-Backend/CodegenEnv.h
        src/LLVM-Backend/ExpBuilder.cpp
        src/LLVM-Backend/ExpBuilder.h
        src/LLVM-Backend/ProgramBuilder.cpp
        src/LLVM-Backend/ProgramBuilder.h
        src/LLVM-Backend/IndexBuilder.h
        src/LLVM-Backend/IndexBuilder.cpp
        src/Common/Util.h
        src/Frontend/Parser.h)

find_package(LLVM CONFIG REQUIRED)
if(VERBOSE)
    message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
    message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
    message(STATUS "Include dirs in: ${LLVM_INCLUDE_DIRS}")

    foreach(lib ${LLVM_AVAILABLE_LIBS})
        message(STATUS ${lib})
    endforeach()

    foreach(def ${LLVM_DEFINITIONS})
        message(STATUS ${def})
    endforeach()
endif()

add_library(jlc-lib ${jlc-lib_SOURCE})

add_definitions(${LLVM_DEFINITIONS})
target_link_libraries(jlc-lib LLVM)


target_include_directories(jlc-lib
	PUBLIC
    ${LLVM_INCLUDE_DIRS}
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
)

set_target_properties(jlc-lib PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_LIST_DIR}/bin/Debug/${PROJECT_NAME}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_LIST_DIR}/bin/Release/${PROJECT_NAME}
)

add_executable(${PROJECT_NAME} src/Main.cpp)
target_link_libraries(${PROJECT_NAME} jlc-lib)

add_subdirectory(sandbox)
add_subdirectory(test)