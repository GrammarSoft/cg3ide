set(CMAKE_LEGACY_CYGWIN_WIN32 0)
cmake_minimum_required(VERSION 2.8.3 FATAL_ERROR)
project(cg3ide CXX)

# Release or Debug
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

if(NOT MSVC)
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-deprecated")
	set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Widgets REQUIRED)
include_directories(${Qt5Widgets_INCLUDE_DIRS})
add_definitions(${Qt5Widgets_DEFINITIONS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)

option(OPT_CPACK "Set to ON to generate CPack configuration files" OFF)
if(OPT_CPACK)
	set(CPACK_PACKAGE_NAME "cg3ide")
	set(CPACK_PACKAGE_CONTACT "Tino Didriksen <mail@tinodidriksen.com>")
	set(CPACK_PACKAGE_VENDOR "http://tinodidriksen.com/")
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "CG-3 IDE is an environment for working with Constraint Grammar in an interactive way.")
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5widgets5, cg3")
	set(CPACK_PACKAGE_VERSION "0.5.9292")

	set(CPACK_DEBIAN_BUILD_DEPENDS debhelper cmake qt5-default qt5-qmake qtbase5-dev)
	set(CPACK_DEBIAN_PACKAGE_SOURCE_COPY svn export --force)
	set(CPACK_DEBIAN_CHANGELOG "  * Changelog is available at http://visl.sdu.dk/cg3/changelog.txt\n\n")
	execute_process(COMMAND lsb_release -is
		OUTPUT_VARIABLE _lsb_distribution OUTPUT_STRIP_TRAILING_WHITESPACE
		RESULT_VARIABLE _lsb_release_failed)
	set(CPACK_DEBIAN_DISTRIBUTION_NAME ${_lsb_distribution})
	string(TOLOWER ${CPACK_DEBIAN_DISTRIBUTION_NAME} CPACK_DEBIAN_DISTRIBUTION_NAME)
	if( ${CPACK_DEBIAN_DISTRIBUTION_NAME} STREQUAL "ubuntu" )
		set(CPACK_DEBIAN_DISTRIBUTION_RELEASES precise quantal raring saucy)
	endif()
	set(DPUT_HOST "ppa:tinodidriksen/cg3")
	include(CPack)
	include(DebSourcePPA)
endif()
