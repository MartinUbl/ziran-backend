CMAKE_MINIMUM_REQUIRED(VERSION 3.12)

SET(PROJ "clang-tidy")

FILE(GLOB SRC_FILES "src/*.cpp" "src/*.c" "src/*.h")
IF(WIN32)
	FILE(GLOB SRC_WIN_FILES "src/win/*.cpp" "src/win/*.h" "src/win/*.def")
	SET(SRC_FILES "${SRC_FILES};${SRC_WIN_FILES}")
ENDIF()

ADD_LIBRARY(${PROJ} SHARED ${SRC_FILES})

TARGET_LINK_LIBRARIES(${PROJ} ziran-shared)
