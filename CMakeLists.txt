cmake_minimum_required(VERSION 2.6)

PROJECT(jsonpath C)
# Offer the user the choice of overriding the installation directories
set(INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for executables")
set(INSTALL_INCLUDE_DIR include CACHE PATH "Installation directory for header files")

ADD_DEFINITIONS(-Os -Wall -Werror --std=gnu99 -Wmissing-declarations -Wno-error=unused-variable -ffunction-sections -D_GNU_SOURCE)

SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "-Wl,--gc-sections")

IF(APPLE)
  INCLUDE_DIRECTORIES(/opt/local/include)
  LINK_DIRECTORIES(/opt/local/lib)
ENDIF()

IF(DEBUG)
  ADD_DEFINITIONS(-DDEBUG -g3)
ENDIF()

INCLUDE(FindPkgConfig)
pkg_search_module(JSONC required json-c json)
INCLUDE_DIRECTORIES(${JSONC_INCLUDE_DIRS})

ADD_CUSTOM_COMMAND(
  OUTPUT contrib/lemon
  DEPENDS contrib/lemon.c contrib/lempar.c
  COMMAND gcc -o contrib/lemon contrib/lemon.c
  COMMENT "Generating lemon parser generator"
)

ADD_CUSTOM_COMMAND(
  OUTPUT parser.c
  DEPENDS parser.y contrib/lemon
  COMMAND ./contrib/lemon parser.y
  COMMENT "Generating parser.c"
)

SET_PROPERTY(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES "lemon;parser.h;parser.out")
SET_SOURCE_FILES_PROPERTIES("parser.c" PROPERTIES GENERATED TRUE)

ADD_LIBRARY(jsonpath SHARED ast.c lexer.c parser.c matcher.c)
SET_TARGET_PROPERTIES(jsonpath PROPERTIES PUBLIC_HEADER jsonpath.h)
ADD_EXECUTABLE(jsonpathdemo main.c)
TARGET_LINK_LIBRARIES(jsonpathdemo ubox ${JSONC_LIBRARIES} jsonpath)

INSTALL(TARGETS jsonpathdemo RUNTIME DESTINATION bin)
INSTALL(TARGETS jsonpath
	LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
	PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}"
)
