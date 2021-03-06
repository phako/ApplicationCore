PROJECT(ChimeraTK-ApplicationCore)
cmake_minimum_required(VERSION 2.8)

include(cmake/add_dependency.cmake)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules)

set(${PROJECT_NAME}_MAJOR_VERSION 00)
set(${PROJECT_NAME}_MINOR_VERSION 13)
set(${PROJECT_NAME}_PATCH_VERSION 02)
include(cmake/set_version_numbers.cmake)

# Find the DeviceAccess library
# put the min version of deviceaccess into a variable. We need it for the debian packaging scripts
add_dependency(mtca4u-deviceaccess 00.29 REQUIRED)

# Find the ControlSystemAdapter
# put the min version of deviceaccess into a variable. We need it for the debian packaging scripts
add_dependency(ChimeraTK-ControlSystemAdapter 00.11 REQUIRED)

# Find the XML parser library libxml++
FIND_PACKAGE(PkgConfig REQUIRED)
set(LIBXML++_VERSION "libxml++-2.6")
PKG_CHECK_MODULES(LibXML++ REQUIRED ${LIBXML++_VERSION})
PKG_CHECK_MODULES(glib REQUIRED glib-2.0)
include_directories(SYSTEM "${LibXML++_INCLUDEDIR}/${LIBXML++_VERSION}" ${glib_INCLUDE_DIRS} ${LibXML++_INCLUDE_DIRS})

# optional dependency: BOOST unit test framework needed for tests
FIND_PACKAGE(Boost COMPONENTS unit_test_framework)
IF(Boost_UNIT_TEST_FRAMEWORK_FOUND)
  set(TESTING_IS_ENABLED "true")
  ENABLE_TESTING()
ELSE()
  message(" The following packages were not found, so testing will be disabled:")
  message("  * BOOST unit_test_framework")
ENDIF()

# Find BOOST filesystem
# Note: we need to search this after looking for the unit_test_framework, since we must not link against the
#       unit_test_framework library (use header-only)
FIND_PACKAGE(Boost COMPONENTS filesystem REQUIRED)

# optional dependency: the ControlSystemAdapter-DoocsAdapter is needed for the example
add_dependency(ChimeraTK-ControlSystemAdapter-DoocsAdapter 00.06)
IF(ChimeraTK-ControlSystemAdapter-DoocsAdapter_FOUND)
  set(DEMO_IS_ENABLED "true")
  ENABLE_TESTING()
ELSE()
  message(" The following packages were not found, so the example will be disabled:")
  IF(NOT ChimeraTK-ControlSystemAdapter-DoocsAdapter_FOUND)
    message("  * ChimeraTK-ControlSystemAdapter-DoocsAdapter")
  ENDIF()
ENDIF()

# requierd dependency: HDF5 library needed for the MicroDAQ system (TODO make optional!)
FIND_PACKAGE(HDF5 COMPONENTS CXX REQUIRED)
include_directories(SYSTEM ${HDF5_INCLUDE_DIRS})
link_directories(${HDF5_LIBRARY_DIRS})
IF(HDF5_FOUND)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_MICRO_DAQ")
ELSE()
  message(" HDF5 was not found, so the MicroDAQ system will not be built.")
  set(HDF5_LIBRARIES "")
ENDIF()


# Moderate version of the compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -Wextra -Wuninitialized ")

# use -DCMAKE_BUILD_TYPE=Debug in your cmake command to turn on the coverage option
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 --coverage -fno-inline -fno-inline-small-functions -fno-default-inline")

# linker flags
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--export-dynamic ${mtca4u-deviceaccess_LINK_FLAGS}")

include_directories(${CMAKE_SOURCE_DIR}/include)
set(${PROJECT_NAME}_INCLUDE_DIRS ${${PROJECT_NAME}_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include/ ${CMAKE_SOURCE_DIR}/Modules/include/)
aux_source_directory(${CMAKE_SOURCE_DIR}/src library_sources)

# add generic modules
include_directories(${CMAKE_SOURCE_DIR}/Modules/include)
aux_source_directory(${CMAKE_SOURCE_DIR}/Modules/src library_sources)

MACRO( COPY_MAPPING_FILES )
  foreach( FILE_TO_COPY test.xlmap test.dmap )
    configure_file( tests/${FILE_TO_COPY} ${PROJECT_BINARY_DIR}/${FILE_TO_COPY} copyonly)
  endforeach( FILE_TO_COPY )
ENDMACRO( COPY_MAPPING_FILES )

# Create the executables for automated unit testing.
if(TESTING_IS_ENABLED)

  aux_source_directory(${CMAKE_SOURCE_DIR}/tests/executables_src testExecutables)
  foreach( testExecutableSrcFile ${testExecutables})
    #NAME_WE means the base name without path and (longest) extension
    get_filename_component(excutableName ${testExecutableSrcFile} NAME_WE)
    add_executable(${excutableName} ${testExecutableSrcFile})
    # do not link against the boost unit test library, the tests are not written for it!
    target_link_libraries(${excutableName} ${PROJECT_NAME} ${ChimeraTK-ControlSystemAdapter_LIBRARIES}
                                                           ${mtca4u-deviceaccess_LBRARIES} ${HDF5_LIBRARIES})
    add_test(${excutableName} ${excutableName})
  endforeach( testExecutableSrcFile )

  # enable code coverate report
  include(cmake/enable_code_coverage_report.cmake)

  # copy config files
  FILE( COPY ${CMAKE_SOURCE_DIR}/tests/test.map DESTINATION ${PROJECT_BINARY_DIR})
  FILE( COPY ${CMAKE_SOURCE_DIR}/tests/test2.map DESTINATION ${PROJECT_BINARY_DIR})
  FILE( COPY ${CMAKE_SOURCE_DIR}/tests/test.dmap DESTINATION ${PROJECT_BINARY_DIR})
  FILE( COPY ${CMAKE_SOURCE_DIR}/tests/validConfig.xml DESTINATION ${PROJECT_BINARY_DIR})
  FILE( COPY ${CMAKE_SOURCE_DIR}/xmlschema/application.xsd DESTINATION ${PROJECT_BINARY_DIR})

endif(TESTING_IS_ENABLED)

# build example
if(DEMO_IS_ENABLED)

  add_executable(demoApp example/demoApp.cc example/TimerDummyDevice.cc example/DemoDummyDevice.cc)
  set_target_properties(demoApp PROPERTIES COMPILE_FLAGS "${mtca4u-deviceaccess_CXX_FLAGS} ${ControlSystemAdapter_CXX_FLAGS}")
  set_target_properties(demoApp PROPERTIES LINK_FLAGS "${mtca4u-deviceaccess_LINK_FLAGS} ${ControlSystemAdapter_LINK_FLAGS} ${ChimeraTK-ControlSystemAdapter-DoocsAdapter_LINK_FLAGS}")
  target_link_libraries(demoApp ${PROJECT_NAME}
                                ${Boost_LIBRARIES}
                                ${mtca4u-deviceaccess_LIBRARIES}
                                ${ChimeraTK-ControlSystemAdapter_LIBRARIES}
                                ${ChimeraTK-ControlSystemAdapter-DoocsAdapter_LIBRARIES}
                                ${LibXML++_LIBRARIES}
                                ${glib_LIBRARIES}
                                ${HDF5_LIBRARIES})

  add_executable(demoApp-xmlGenerator example/demoApp.cc)
  set_target_properties(demoApp-xmlGenerator PROPERTIES COMPILE_FLAGS "${mtca4u-deviceaccess_CXX_FLAGS} -DGENERATE_XML")
  target_link_libraries(demoApp-xmlGenerator ${PROJECT_NAME}
                                             ${mtca4u-deviceaccess_LIBRARIES}
                                             ${Boost_LIBRARIES}
                                             ${ChimeraTK-ControlSystemAdapter_LIBRARIES}
                                             ${LibXML++_LIBRARIES}
                                             ${glib_LIBRARIES}
                                             ${HDF5_LIBRARIES})
  
  add_custom_target(demoApp-xml ALL DEPENDS demoApp-xmlGenerator
                                    COMMAND ${PROJECT_BINARY_DIR}/demoApp-xmlGenerator
                                    WORKING_DIRECTORY ${PROJECT_BINARY_DIR})

  # copy config files
  FILE( COPY ${CMAKE_SOURCE_DIR}/example/dummy.dmap DESTINATION ${PROJECT_BINARY_DIR})
  FILE( COPY ${CMAKE_SOURCE_DIR}/example/DemoDummy.map DESTINATION ${PROJECT_BINARY_DIR})
  FILE( COPY ${CMAKE_SOURCE_DIR}/example/demoApp.conf DESTINATION ${PROJECT_BINARY_DIR})

endif(DEMO_IS_ENABLED)

# C++ library
add_library(${PROJECT_NAME} SHARED ${library_sources} )
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${${PROJECT_NAME}_FULL_LIBRARY_VERSION}
                                                 SOVERSION ${${PROJECT_NAME}_SOVERSION})
target_link_libraries(${PROJECT_NAME} ${mtca4u-deviceaccess_LIBRARIES}
                                      ${ChimeraTK-ControlSystemAdapter_LIBRARIES}
                                      ${Boost_LIBRARIES}
                                      pthread
                                      ${LibXML++_LIBRARIES}
                                      ${glib_LIBRARIES}
                                      ${HDF5_LIBRARIES})

# do not remove runtime path of the library when installing
set_property(TARGET ${PROJECT_NAME} PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)

# add a target to generate API documentation with Doxygen
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/main.dox.in ${CMAKE_CURRENT_BINARY_DIR}/main.dox @ONLY)
include(cmake/enable_doxygen_documentation.cmake)

# Install the library and the executables
install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin LIBRARY DESTINATION lib )

# all include files go into include/PROJECT_NAME
# The exclusion of ${PROJECT_NAME} prevents the recursive installation of the files just being installed.
# The original headers are in include/*.h, the installed ones in include/PROJECT_NAME/*.h.
install(DIRECTORY ${${PROJECT_NAME}_INCLUDE_DIRS} DESTINATION include/ChimeraTK/ApplicationCore
                                                  FILES_MATCHING PATTERN "*.h"
                                                                 PATTERN ".svn"
                                                  EXCLUDE PATTERN "${PROJECT_NAME}" EXCLUDE)

set(${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/include")
set(${PROJECT_NAME}_LIBRARIES "${PROJECT_NAME} ${ChimeraTK-ControlSystemAdapter_LIBRARIES} ${mtca4u-deviceaccess_LBRARIES} ${HDF5_LIBRARIES}")
set(${PROJECT_NAME}_LIBRARY_DIRS "${CMAKE_INSTALL_PREFIX}/lib")
set(${PROJECT_NAME}_CXX_FLAGS "${mtca4u-deviceaccess_CXX_FLAGS} ${ChimeraTK-ControlSystemAdapter_CXX_FLAGS}")
set(${PROJECT_NAME}_LINK_FLAGS "${mtca4u-deviceaccess_LINK_FLAGS} ${ChimeraTK-ControlSystemAdapter_LINK_FLAGS}")
include(${CMAKE_SOURCE_DIR}/cmake/create_cmake_config_files.cmake)

