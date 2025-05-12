cmake_minimum_required(VERSION 3.20)
project(GameProject C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Enable debug mode
set(CMAKE_BUILD_TYPE Debug)

# Suppress MSVC warnings for deprecated functions
if(MSVC)
    add_compile_options(/wd4996)
endif()

# Include FetchContent module
include(FetchContent)

# Fetch PhysFS
FetchContent_Declare(
    physfs
    GIT_REPOSITORY https://github.com/icculus/physfs.git
    GIT_TAG main
)
FetchContent_MakeAvailable(physfs)

# Fetch Lua (source only, no CMake build)
FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/lua/lua.git
    GIT_TAG v5.4.7
)
FetchContent_Populate(lua)

# Lua doesn't have a CMake build, so we manually compile it
# List Lua source files (core and libraries, excluding lua.c and luac.c)
set(LUA_SRC
    ${lua_SOURCE_DIR}/lapi.c
    ${lua_SOURCE_DIR}/lauxlib.c
    ${lua_SOURCE_DIR}/lbaselib.c
    ${lua_SOURCE_DIR}/lcode.c
    ${lua_SOURCE_DIR}/lcorolib.c
    ${lua_SOURCE_DIR}/lctype.c
    ${lua_SOURCE_DIR}/ldblib.c
    ${lua_SOURCE_DIR}/ldebug.c
    ${lua_SOURCE_DIR}/ldo.c
    ${lua_SOURCE_DIR}/ldump.c
    ${lua_SOURCE_DIR}/lfunc.c
    ${lua_SOURCE_DIR}/lgc.c
    ${lua_SOURCE_DIR}/linit.c
    ${lua_SOURCE_DIR}/liolib.c
    ${lua_SOURCE_DIR}/llex.c
    ${lua_SOURCE_DIR}/lmathlib.c
    ${lua_SOURCE_DIR}/lmem.c
    ${lua_SOURCE_DIR}/loadlib.c
    ${lua_SOURCE_DIR}/lobject.c
    ${lua_SOURCE_DIR}/lopcodes.c
    ${lua_SOURCE_DIR}/loslib.c
    ${lua_SOURCE_DIR}/lparser.c
    ${lua_SOURCE_DIR}/lstate.c
    ${lua_SOURCE_DIR}/lstring.c
    ${lua_SOURCE_DIR}/lstrlib.c
    ${lua_SOURCE_DIR}/ltable.c
    ${lua_SOURCE_DIR}/ltablib.c
    ${lua_SOURCE_DIR}/ltm.c
    ${lua_SOURCE_DIR}/lundump.c
    ${lua_SOURCE_DIR}/lvm.c
    ${lua_SOURCE_DIR}/lzio.c
    ${lua_SOURCE_DIR}/lutf8lib.c
)

# Create a static Lua library
add_library(lua STATIC ${LUA_SRC})
target_include_directories(lua PUBLIC ${lua_SOURCE_DIR})

# Define project sources
set(GAME_PROJECT_SOURCES
    src/main.c
)

# Create GameProject executable
add_executable(${PROJECT_NAME} ${GAME_PROJECT_SOURCES})

# Include directories for GameProject
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src/include
    ${lua_SOURCE_DIR}
)

# Link libraries for GameProject
target_link_libraries(${PROJECT_NAME} PRIVATE
    physfs
    lua
)

# Link math library only on Unix-like systems
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif()

# Copy physfs.dll to the configuration-specific output directory (e.g., Debug)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:physfs>
        ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/$<TARGET_FILE_NAME:physfs>
    COMMENT "Copying physfs.dll to output directory"
)

# Copy assets to the configuration-specific output directory (e.g., Debug) at build time
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/assets
    COMMENT "Copying assets to output directory"
)

# Create convert_assets executable
set(CONVERT_ASSETS_SOURCES
    src/convert_assets.c
)
add_executable(convert_assets ${CONVERT_ASSETS_SOURCES})
target_include_directories(convert_assets PRIVATE
    ${CMAKE_SOURCE_DIR}/src/include
)
target_link_libraries(convert_assets PRIVATE
    physfs
)

# Copy physfs.dll for convert_assets
add_custom_command(TARGET convert_assets POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:physfs>
        ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/$<TARGET_FILE_NAME:physfs>
    COMMENT "Copying physfs.dll for convert_assets"
)

# Run convert_assets to generate assets.bin
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/assets.bin
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/convert_assets
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/assets.bin
    DEPENDS convert_assets
    COMMENT "Generating assets.bin"
)

# Custom target to ensure assets.bin is built
add_custom_target(generate_assets_bin ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/assets.bin
)

# Make GameProject depend on assets.bin
add_dependencies(${PROJECT_NAME} generate_assets_bin)

# Create load_assets test executable
set(LOAD_ASSETS_SOURCES
    src/load_assets.c
)
add_executable(load_assets ${LOAD_ASSETS_SOURCES})
target_include_directories(load_assets PRIVATE
    ${CMAKE_SOURCE_DIR}/src/include
)
target_link_libraries(load_assets PRIVATE
    physfs
)

# Copy physfs.dll for load_assets
add_custom_command(TARGET load_assets POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:physfs>
        ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/$<TARGET_FILE_NAME:physfs>
    COMMENT "Copying physfs.dll for load_assets"
)

# Add test for load_assets
enable_testing()
add_test(
    NAME load_assets_test
    COMMAND load_assets ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/assets.bin
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>
)

# Custom target for running the GameProject executable
add_custom_target(run
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>
    COMMENT "Running ${PROJECT_NAME}"
)