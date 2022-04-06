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
    src/BaseVisitor.cpp
    src/BaseVisitor.h
    src/TypeChecker.cpp
    src/TypeChecker.h
    src/TypeError.h
    src/Util.h
)

add_library(jlc-lib ${jlc-lib_SOURCE})

target_include_directories(jlc-lib
	PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
)

set_target_properties(jlc-lib PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_LIST_DIR}/bin/Debug/${PROJECT_NAME}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_LIST_DIR}/bin/Release/${PROJECT_NAME}
)

add_executable(${PROJECT_NAME} src/Main.cpp)
target_link_libraries(${PROJECT_NAME} jlc-lib)

add_subdirectory(sandbox)