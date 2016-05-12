cmake_minimum_required(VERSION 3.0.0)

project(cotton)

option(DEBUG "Debug Build" OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake-modules/")

file(GLOB SOURCES src/*.cpp)

find_package(BOOST_PROGRAM_OPTIONS REQUIRED)
find_package(BOOST_IOSTREAMS REQUIRED)
find_package(BOOST_SERIALIZATION REQUIRED)

set(LIBS ${LIBS} ${BOOST_PROGRAM_OPTIONS_LIBRARIES})
set(LIBS ${LIBS} ${BOOST_IOSTREAMS_LIBRARIES})
set(LIBS ${LIBS} ${BOOST_SERIALIZATION_LIBRARIES})

set(FLAGS ${FLAGS} -std=c++11 -O2 -Wall)

if(DEBUG)
    set(FLAGS ${FLAGS} -g)
    set(LIBS ${LIBS} -g)
endif()

add_executable(cotton ${SOURCES})

target_include_directories(cotton PRIVATE "${CMAKE_SOURCE_DIR}/headers/" ${BOOST_PROGRAM_OPTIONS_INCLUDES} ${BOOST_IOSTREAMS_INCLUDES} ${BOOST_SERIALIZATION_INCLUDES})

target_link_libraries(cotton PRIVATE ${LIBS})

target_compile_options(cotton PRIVATE ${FLAGS})
