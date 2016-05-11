find_path(BOOST_IOSTREAMS_INCLUDES "boost/iostreams/stream.hpp"
          HINTS ${CMAKE_INSTALL_PREFIX}
          PATH_SUFFIXES include)

find_library(BOOST_IOSTREAMS_LIBRARIES "boost_iostreams" "boost_iostreams-mt"
             HINTS ${CMAKE_INSTALL_PREFIX}
             PATH_SUFFIXES lib lib64 lib32)

if(BOOST_IOSTREAMS_INCLUDES AND BOOST_IOSTREAMS_LIBRARIES)
    set(BOOST_IOSTREAMS_FOUND TRUE)
else()
    set(BOOST_IOSTREAMS_FOUND FALSE)
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(BOOST_IOSTREAMS DEFAULT_MSG BOOST_IOSTREAMS_LIBRARIES BOOST_IOSTREAMS_INCLUDES)
