set(TEST_FILES
        TypeCheckerTest
        )

set(TEST_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/test)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)

FetchContent_MakeAvailable(googletest)

foreach (file ${TEST_FILES})

    add_executable(${file} ${file}.cpp)
    target_link_libraries(${file} jlc-lib gtest_main)

endforeach()