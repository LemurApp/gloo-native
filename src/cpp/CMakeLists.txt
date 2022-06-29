cmake_minimum_required(VERSION 3.9)
set (CMAKE_CXX_STANDARD 11)

project(gloo_core_cpp VERSION 0.0.1)
include_directories(${CMAKE_JS_INC})
string(APPEND CMAKE_C_FLAGS "-Wall -Wextra -Wno-unused-parameter -Wendif-labels")

file(GLOB_RECURSE SOURCE_FILES *.cpp *.h *.cc *.c)
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})

# Include Node-API wrappers
execute_process(COMMAND node -p "require('node-addon-api').include"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE NODE_ADDON_API_DIR
        )

# need to pay attention to this part
string(REGEX REPLACE "[\r\n\"]" "" NODE_ADDON_API_DIR "${NODE_ADDON_API_DIR}")


find_path( CORE_FOUNDATION_INCLUDES "CoreFoundation/CoreFoundation.h" )

set( EXTRA_INCLUDES ${CORE_FOUNDATION_INCLUDES} ${CORE_AUDIO_INCLUDES} )

find_library( FOUNDATION_FRAMEWORK Foundation )
find_library( CORE_FOUNDATION_FRAMEWORK CoreFoundation )
find_library( CORE_AUDIO_FRAMEWORK CoreAudio )
find_library( AUDIO_TOOLBOX_FRAMEWORK AudioToolbox )
find_library( AVFOUNDATION_FRAMEWORK AVFoundation )
find_library( COREDATA_FRAMEWORK CoreData )
find_library( OBJC_FRAMEWORK objc )

set (
      EXTRA_LIBS
      ${CORE_FOUNDATION_FRAMEWORK}
      ${CORE_AUDIO_FRAMEWORK}
      ${AUDIO_TOOLBOX_FRAMEWORK}
      ${AVFOUNDATION_FRAMEWORK}
      ${COREDATA_FRAMEWORK}
      ${OBJC_FRAMEWORK}
      ${FOUNDATION_FRAMEWORK}
    )

message( STATUS "Framework libraries: ${EXTRA_LIBS}" )

find_path( AVFOUNDATION_INCLUDES   "AVFoundation/AVFoundation.h" )
find_path( AUDIO_TOOLBOX_INCLUDES   "AudioToolbox/AudioToolbox.h" )
find_path( CORE_FOUNDATION_INCLUDES "CoreFoundation/CoreFoundation.h" )

set ( EXTRA_INCLUDES
      ${CORE_FOUNDATION_INCLUDES} ${AVFOUNDATION_INCLUDES}
      ${AUDIO_TOOLBOX_INCLUDES}
    )
message( STATUS "Framework include directories: ${EXTRA_INCLUDES}" )

include_directories( ${EXTRA_INCLUDES} )
target_link_libraries( ${PROJECT_NAME} ${EXTRA_LIBS} )


target_include_directories(${PROJECT_NAME} PRIVATE ${NODE_ADDON_API_DIR})

# Define NAPI_VERSION
add_definitions(-DNAPI_VERSION=3)
