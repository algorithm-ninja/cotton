find_path(BOOST_PROGRAM_OPTIONS_INCLUDES "boost/program_options.hpp"
          HINTS ${CMAKE_INSTALL_PREFIX}
          PATH_SUFFIXES include)

find_library(BOOST_PROGRAM_OPTIONS_LIBRARIES "boost_program_options" "boost_program_options-mt"
             HINTS ${CMAKE_INSTALL_PREFIX}
             PATH_SUFFIXES lib lib64 lib32)

if(BOOST_PROGRAM_OPTIONS_INCLUDES AND BOOST_PROGRAM_OPTIONS_LIBRARIES)
    set(BOOST_PROGRAM_OPTIONS_FOUND TRUE)
else()
    set(BOOST_PROGRAM_OPTIONS_FOUND FALSE)
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(BOOST_PROGRAM_OPTIONS DEFAULT_MSG BOOST_PROGRAM_OPTIONS_LIBRARIES BOOST_PROGRAM_OPTIONS_INCLUDES)
