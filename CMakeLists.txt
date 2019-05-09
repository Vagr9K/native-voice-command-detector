cmake_minimum_required(VERSION 3.13)

project (detector)

# Build settings
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Build paths
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}")
set(CMAKE_DEBUG_POSTFIX "")

# Detect and set the logging level depending on build type
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
else()
    add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
endif()

# Add the nlohmann/json as a dependency and disable test builds
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(deps/json)

# Set N-API and Node Addon API include directories
set(CMAKE_JS_INC "${CMAKE_SOURCE_DIR}/node_modules/node-addon-api;${CMAKE_SOURCE_DIR}/node_modules/node-addon-api/src;${CMAKE_SOURCE_DIR}/node_modules/napi-thread-safe-callback/")

# Set 3rd party include directories
set (CPPBASE64_DIR "${CMAKE_SOURCE_DIR}/deps/cpp-base64")
set (JSON_INC "${CMAKE_SOURCE_DIR}/deps/json/include")
set (THREADPOOL_INC "${CMAKE_SOURCE_DIR}/deps/ThreadPool")
set (SPDLOG_INC "${CMAKE_SOURCE_DIR}/deps/spdlog/include")
set (PORCUPINE_INC "${CMAKE_SOURCE_DIR}/deps/Porcupine/include")
set (PORCUPINE_LIB "${CMAKE_SOURCE_DIR}/deps/Porcupine/lib/linux/x86_64/libpv_porcupine.a")

# Set include directories
include_directories(${CMAKE_JS_INC} ${THREADPOOL_INC} ${PORCUPINE_INC} ${JSON_INC} ${CPPBASE64_DIR} ${SPDLOG_INC} /usr/include/opus /usr/include)

# Add source files
file(GLOB LIB_SOURCE_FILES "${CPPBASE64_DIR}/*.cpp")
file(GLOB_RECURSE SOURCE_FILES "src/**.cpp")

# Create the shared library
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${LIB_SOURCE_FILES})

# Target settings
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

# Link
target_link_libraries(${PROJECT_NAME} /usr/lib/libopus.so /usr/lib/libopusenc.a /usr/lib/libcurl.so /usr/lib/libssl.so /usr/lib/libcrypto.so ${PORCUPINE_LIB} nlohmann_json::nlohmann_json -lm)
