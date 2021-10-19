project(EMPTY)

set(ORG_BUILD_TESTING ${BUILD_TESTING})
set(BUILD_TESTING OFF)

cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_COMPILER /usr/bin/c++)
set(CMAKE_CXX_FLAGS "-m64 -std=c++17")
add_compile_options(-W -Wall -g -fpermissive)

include_directories(include)

###############################
# for hpp
###############################
#add_library(EMPTY INTERFACE)
#target_sources(EMPTY INTERFACE EMPTY.hpp)
#target_include_directories (EMPTY
#INTERFACE
#    ${CMAKE_CURRENT_SOURCE_DIR}/include)

###############################
# for cpp (.so)
###############################
#add_library(EMPTY SHARED EMPTY.cpp)
#install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib)
#target_include_directories (EMPTY
#PUBLIC
#    ${CMAKE_CURRENT_SOURCE_DIR}/include)

###############################
# for doxygen
###############################
option(BUILD_DOC "Build documentation" ON)
find_package(Doxygen)
if (DOXYGEN_FOUND)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile)
    configure_file(${DOXYGEN_OUT} @ONLY)
    add_custom_target(
	EMPTY_doc_doxygen ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM )
else (DOXYGEN_FOUND)
    message("Doxygen need to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)


###############################
# for gtest
###############################
set(BUILD_TESTING ${ORG_BUILD_TESTING})
if (BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()

