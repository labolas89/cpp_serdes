cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(TEST_SERDES)

# Require c++14 minimum

if(MSVC)
    add_compile_options("$<$<CONFIG:RELEASE>:/O2>")
    add_compile_options("/std:c++14" "/Zc:__cplusplus")
    #add_compile_options("/std:c++latest" "/Zc:__cplusplus" "/experimental:module") # testing
    add_compile_options("/W4")
else()
    add_compile_options("-std=c++14")
    #add_compile_options("-std=c++2a") # testing
    add_compile_options("$<$<CONFIG:RELEASE>:-O3>" "-Wall" "-Wextra" "-Werror")
    
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        add_compile_options("-stdlib=libc++")
    else()
        # For removing warnings from GCC
        add_compile_options("-Wunused-macros" "-Wcast-qual" "-Wno-variadic-macros")
        add_compile_options("-Wlogical-op" "-Wdouble-promotion" "-Wformat" "-Wmissing-include-dirs")
        add_compile_options("-Wpedantic")
        add_compile_options("-Wcast-align" "-Wpacked" "-Wredundant-decls" "-Wvarargs" "-Wvector-operation-performance" "-Wswitch-default")
        add_compile_options("-Wuninitialized" "-Wshadow" "-Wmissing-declarations")
    endif()
endif()

set(CMAKE_USE_RELATIVE_PATHS TRUE)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

file(GLOB SRC_LIST 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

add_executable(${CMAKE_PROJECT_NAME}
    ${SRC_LIST}
)

include (CTest)
add_test(test-0 TEST_SERDES)


