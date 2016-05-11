find_path(BOOST_SERIALIZATION_INCLUDES "boost/serialization/serialization.hpp"
          HINTS ${CMAKE_INSTALL_PREFIX}
          PATH_SUFFIXES include)

find_library(BOOST_SERIALIZATION_LIBRARIES "boost_serialization" "boost_serialization-mt"
             HINTS ${CMAKE_INSTALL_PREFIX}
             PATH_SUFFIXES lib lib64 lib32)

if(BOOST_SERIALIZATION_INCLUDES AND BOOST_SERIALIZATION_LIBRARIES)
    set(BOOST_SERIALIZATION_FOUND TRUE)
else()
    set(BOOST_SERIALIZATION_FOUND FALSE)
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(BOOST_SERIALIZATION DEFAULT_MSG BOOST_SERIALIZATION_LIBRARIES BOOST_SERIALIZATION_INCLUDES)
