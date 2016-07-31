cmake_minimum_required(VERSION 3.0.2)

project(cotton)

include(CheckCXXCompilerFlag)

option(DEBUG "Debug Build" OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake-modules/")

file(GLOB SOURCES src/*.cpp)

find_package(BOOST_IOSTREAMS REQUIRED)
find_package(BOOST_SERIALIZATION REQUIRED)

set(LIBS ${LIBS} ${BOOST_IOSTREAMS_LIBRARIES})
set(LIBS ${LIBS} ${BOOST_SERIALIZATION_LIBRARIES})

set(FLAGS ${FLAGS} -O3 -ftemplate-depth=1024 -Wall -Wno-unused-result)

CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)

if(COMPILER_SUPPORTS_CXX14)
    set(FLAGS ${FLAGS} -std=c++14)
else()
    set(FLAGS ${FLAGS} -std=c++1y)
endif()

if(DEBUG)
    set(FLAGS ${FLAGS} -g)
    set(LIBS ${LIBS} -g)
endif()

add_executable(cotton ${SOURCES})

target_include_directories(cotton PRIVATE "${CMAKE_SOURCE_DIR}/headers/" "${CMAKE_SOURCE_DIR}/program-options/headers" ${BOOST_IOSTREAMS_INCLUDES} ${BOOST_SERIALIZATION_INCLUDES})

target_link_libraries(cotton PRIVATE ${LIBS})

target_compile_options(cotton PRIVATE ${FLAGS})

install(TARGETS cotton DESTINATION bin)
