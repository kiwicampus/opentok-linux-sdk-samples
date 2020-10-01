PROJECT(custom_video_capturer)

SET(CMAKE_VERBOSE_MAKEFILE ON)

CMAKE_MINIMUM_REQUIRED(VERSION 3.5)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")

# Remove the commented out line below to enable OpenTok debug traces.
# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCONSOLE_LOGGING")

FIND_PACKAGE(Threads)
#find_package(OpenCV REQUIRED)

INCLUDE(FindPkgConfig)

ADD_SUBDIRECTORY(
   ${CMAKE_CURRENT_SOURCE_DIR}/../assets/libopentok_linux_llvm_arm64
    ${CMAKE_CURRENT_BINARY_DIR}/libopentok
)

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/src
    $<TARGET_PROPERTY:libopentok,INTERFACE_INCLUDE_DIRECTORIES>
    #${OpenCV_INCLUDE_DIRS}
    )

ADD_EXECUTABLE(
    ${PROJECT_NAME}
    main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/src/otk_thread.c
)
TARGET_LINK_LIBRARIES(
    ${PROJECT_NAME}
    libopentok
    #${OpenCV_LIBS}
) 
