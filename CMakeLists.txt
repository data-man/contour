cmake_minimum_required(VERSION 3.10)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(Version)
GetVersionInformation(CONTOUR_VERSION CONTOUR_VERSION_STRING)
file(WRITE "${CMAKE_BINARY_DIR}/version.txt" "${CONTOUR_VERSION_STRING}")

project(contour VERSION "${CONTOUR_VERSION}" LANGUAGES CXX)

# setting defaults
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CheckCXXCompilerFlag)
function(try_add_compile_options FLAG)
    # Remove leading - or / from the flag name.
    string(REGEX REPLACE "^[-/]" "" name ${FLAG})
    # Deletes any ':' because it's invalid variable names.
    string(REGEX REPLACE ":" "" name ${name})
    check_cxx_compiler_flag(${FLAG} ${name})
    if(${name})
        message(STATUS "Adding compiler flag: ${FLAG}.")
        add_compile_options(${FLAG})
    else()
        message(STATUS "Adding compiler flag: ${FLAG} failed.")
    endif()

    # If the optional argument passed, store the result there.
    if(ARGV1)
        set(${ARGV1} ${name} PARENT_SCOPE)
    endif()
endfunction()

if(("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU") OR ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang"))
	try_add_compile_options(-Wall)
	try_add_compile_options(-Wduplicate-enum)
	try_add_compile_options(-Wduplicated-cond)
	try_add_compile_options(-Wextra)
	try_add_compile_options(-Wfinal-dtor-non-final-class)
	try_add_compile_options(-Wimplicit-fallthrough)
	try_add_compile_options(-Wlogical-op)
	try_add_compile_options(-Wmissing-declarations)
	try_add_compile_options(-Wnewline-eof)
	try_add_compile_options(-Wno-unknown-attributes)
	try_add_compile_options(-Wno-unknown-pragmas)
	try_add_compile_options(-Wsign-conversion)
	try_add_compile_options(-Wsuggest-destructor-override)
	try_add_compile_options(-pedantic)
    try_add_compile_options(-Wconversion)
    try_add_compile_options(-fdiagnostics-color=always)
    #try_add_compile_options(-Werror)
    try_add_compile_options(-Wextra-semi)
    try_add_compile_options(-Wpessimizing-move)
    try_add_compile_options(-Wredundant-move)
    try_add_compile_options(-Qunused-arguments)
elseif(DEFINED MSVC)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_USE_MATH_DEFINES)
endif()

option(USE_BOOST_FILESYSTEM "Compiles and links against boost::filesystem instead of std::filesystem [default: OFF]." OFF)

if("${OpenGL_GL_PREFERENCE}" STREQUAL "")
    # possible values are: GLVND, LEGACY
    set(OpenGL_GL_PREFERENCE "GLVND")
endif()

include(EnableCcache)
include(ClangTidy)
include(CIncludeMe)
find_package(Threads)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CONTOUR_TESTING "Enables building of unittests for libterminal [default: ON" ON)
option(CONTOUR_FRONTEND_GUI "Enables GUI frontend." ON)
option(CONTOUR_EXAMPLES "Enables building of example programs. [default: ON]" ON)
option(CONTOUR_COVERAGE "Builds with codecov [default: OFF]" OFF)
option(CONTOUR_SANITIZE "Builds with Address sanitizer enabled [default: OFF]" OFF)

# ----------------------------------------------------------------------------
# code coverage

if(CONTOUR_COVERAGE AND NOT MSVC)
    add_compile_options(-g --coverage)
    set(CMAKE_EXE_LINKER_FLAGS "--coverage ${CMAKE_EXE_LINKER_FLAGS}")
    set(CONTOUR_CODE_COVERAGE_ENABLED ON)
else()
    set(CONTOUR_CODE_COVERAGE_ENABLED OFF)
endif()

if(SANITIZE)
	string(TOLOWER "${SANITIZE}" san)
	if (san STREQUAL "address")
        message(STATUS "Enabling ${san} sanitizer.")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address -fsanitize-address-use-after-scope")
    else()
        message(STATUS "No sanitizer enabled.")
	endif()
endif()

# ----------------------------------------------------------------------------
# 3rdparty dependencies
include(ThirdParties)

ThirdPartiesAdd_Catch2()
ThirdPartiesAdd_fmtlib()
ThirdPartiesAdd_libunicode()
ThirdPartiesAdd_range_v3()
ThirdPartiesAdd_yaml_cpp()

# ----------------------------------------------------------------------------
add_subdirectory(src)

if(CONTOUR_EXAMPLES)
    add_subdirectory(examples)
endif()

# ----------------------------------------------------------------------------
# Summary

if(${USING_BOOST_FILESYSTEM})
    set(USING_FILESYSTEM_API_STRING "boost::filesystem")
else()
    set(USING_FILESYSTEM_API_STRING "C++fs")
endif()

if("${CCACHE}" STREQUAL "")
    set(USING_CCACHE_STRING "OFF")
else()
    set(USING_CCACHE_STRING "${CCACHE}")
endif()

message(STATUS "------------------------------------------------------------------------------")
message(STATUS "    Contour (${CONTOUR_VERSION_STRING})")
message(STATUS "------------------------------------------------------------------------------")
message(STATUS "Build type:                         ${CMAKE_BUILD_TYPE}")
message(STATUS "Build with sanitizer:               ${CONTOUR_SANITIZE}")
message(STATUS "Build unit tests:                   ${CONTOUR_TESTING}")
message(STATUS "Enable with code coverage:          ${CONTOUR_CODE_COVERAGE_ENABLED}")
message(STATUS "Build contour frontend GUI:         ${CONTOUR_FRONTEND_GUI}")
message(STATUS "|> Enable blur effect on KWin:      ${CONTOUR_BLUR_PLATFORM_KWIN}")
message(STATUS "|> Enable performance metrics:      ${CONTOUR_PERF_STATS}")
message(STATUS "|> OpenGL preference:               ${OpenGL_GL_PREFERENCE}")
message(STATUS "Using filesystem API:               ${USING_FILESYSTEM_API_STRING}")
message(STATUS "Using ccache:                       ${USING_CCACHE_STRING}")
message(STATUS "------------------------------------------------------------------------------")
