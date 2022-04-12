set(SANDBOX_FILES
    PrintTypedTree
)

set(SANDBOX_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/sandbox)

foreach (file ${SANDBOX_FILES})

    add_executable(${file} ${file}.cpp)
    target_link_libraries(${file} jlc-lib)
 
endforeach()