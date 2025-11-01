CMAKE_MINIMUM_REQUIRED(VERSION 3.12)

PROJECT(ziran)

SET(CMAKE_CXX_STANDARD 17)
ADD_COMPILE_OPTIONS("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
ADD_COMPILE_OPTIONS("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

SET(MYSQL_INCLUDE_DIR "" CACHE STRING "MySQL include path")
IF (WIN32)
    SET(MYSQL_LIBRARY_STATIC "" CACHE STRING "MySQL library (libmysql.lib or similar) path")
ELSE()
    SET(MYSQL_LIBRARY_STATIC "mysqlclient")
ENDIF()

INCLUDE_DIRECTORIES(${MYSQL_INCLUDE_DIR})

FIND_PACKAGE(Threads REQUIRED)

SET(output_dir "${CMAKE_BINARY_DIR}/compiled")

# globally set output directory for single-configuration environments (GCC, ..)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${output_dir}")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${output_dir}")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${output_dir}")

# globally set output directory for multi-configuration environments (MSVS, ..)
FOREACH(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    SET(ORIG_OUT_CONFIG ${OUTPUTCONFIG})
    STRING(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${output_dir}/${ORIG_OUT_CONFIG}")
    SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${output_dir}/${ORIG_OUT_CONFIG}")
    SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${output_dir}/${ORIG_OUT_CONFIG}")
ENDFOREACH(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)

FILE(GLOB_RECURSE core_src ziran-core/*.cpp ziran-core/*.h ziran-core/*.hpp ziran-core/*.c)
FILE(GLOB_RECURSE shared_src ziran-shared/*.cpp ziran-shared/*.h ziran-shared/*.hpp ziran-shared/*.c)

MESSAGE("Configuring core executable")
ADD_EXECUTABLE(ziran-core ${core_src})

MESSAGE("Configuring shared code library")
ADD_LIBRARY(ziran-shared STATIC ${shared_src})
SET_PROPERTY(TARGET ziran-shared PROPERTY POSITION_INDEPENDENT_CODE ON)

IF(WIN32)
	TARGET_LINK_LIBRARIES(ziran-core Ws2_32)
ENDIF()

TARGET_LINK_LIBRARIES(ziran-core ${MYSQL_LIBRARY_STATIC} ${CMAKE_DL_LIBS} Threads::Threads ziran-shared)

ADD_SUBDIRECTORY(ziran-plugins)
