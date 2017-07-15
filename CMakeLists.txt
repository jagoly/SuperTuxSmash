cmake_minimum_required(VERSION 3.2)
project(SuperTuxSmash_Game)

################################################################################

option(STS_LINUX "Build for Linux" FALSE)
option(STS_WINDOWS "Build for Windows" FALSE)

if (NOT STS_LINUX AND NOT STS_WINDOWS)
    message(FATAL_ERROR "Must specify STS_[PLATFORM] (LINUX, WINDOWS)")
endif ()

if (STS_LINUX)
    add_definitions(-DSTS_LINUX)
elseif (STS_WINDOWS)
    add_definitions(-DSTS_WINDOWS)
endif ()

################################################################################

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif ()

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    add_definitions(-DSQEE_DEBUG)
endif ()

################################################################################

#set(CMAKE_CXX_STANDARD 14)
string(APPEND CMAKE_CXX_FLAGS " -std=c++17")

################################################################################

string(APPEND CMAKE_CXX_FLAGS " -Wall -Wextra")
string(APPEND CMAKE_CXX_FLAGS " -Wno-unused-parameter -Wno-strict-aliasing")
string(APPEND CMAKE_CXX_FLAGS " -Wno-comment -Wno-misleading-indentation")

if (STS_LINUX)
    string(APPEND CMAKE_CXX_FLAGS "")
elseif (STS_WINDOWS)
    string(APPEND CMAKE_CXX_FLAGS " -mwindows -Wa,-mbig-obj")
endif ()

################################################################################

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build")
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules/")

################################################################################

set(SOURCES

  "src/main.cpp"

  #----------------------------------------------------------#

  "src/main/Options.hpp"
  "src/main/Options.cpp"

  "src/main/SmashApp.hpp"
  "src/main/SmashApp.cpp"

  #----------------------------------------------------------#

  "src/render/Renderer.hpp"
  "src/render/Renderer.cpp"

  "src/render/RenderEntity.hpp"
  "src/render/RenderEntity.cpp"

  #----------------------------------------------------------#

  "src/render/fighters/Cheese_Render.hpp"
  "src/render/fighters/Cheese_Render.cpp"

  "src/render/fighters/Sara_Render.hpp"
  "src/render/fighters/Sara_Render.cpp"

  #----------------------------------------------------------#

  "src/game/GameScene.hpp"
  "src/game/GameScene.cpp"

  "src/game/Actions.hpp"
  "src/game/Actions.cpp"

  "src/game/Controller.hpp"
  "src/game/Controller.cpp"

  "src/game/Entity.hpp"
  "src/game/Entity.cpp"

  "src/game/Fighter.hpp"
  "src/game/Fighter.cpp"

  "src/game/Stage.hpp"
  "src/game/Stage.cpp"

  #----------------------------------------------------------#

  "src/game/stages/TestZone_Stage.hpp"
  "src/game/stages/TestZone_Stage.cpp"

  #----------------------------------------------------------#

  "src/game/fighters/Cheese_Actions.hpp"
  "src/game/fighters/Cheese_Actions.cpp"

  "src/game/fighters/Cheese_Fighter.hpp"
  "src/game/fighters/Cheese_Fighter.cpp"

  "src/game/fighters/Sara_Actions.hpp"
  "src/game/fighters/Sara_Actions.cpp"

  "src/game/fighters/Sara_Fighter.hpp"
  "src/game/fighters/Sara_Fighter.cpp"
)

################################################################################

file(GLOB_RECURSE SHADERS "shaders/*.glsl")

################################################################################

include_directories("${PROJECT_SOURCE_DIR}/src")
add_executable(sts-game ${SOURCES} ${SHADERS})

################################################################################

find_package(SQEE REQUIRED)

include_directories(${SQEE_INCLUDE_DIR})
target_link_libraries(sts-game ${SQEE_LIBRARY})

################################################################################

find_package(SFML 2.4 COMPONENTS audio window system REQUIRED)

include_directories(${SFML_INCLUDE_DIR})
target_link_libraries(sts-game ${SFML_LIBRARIES})

################################################################################

file(COPY "assets" DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(COPY "shaders" DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

file(COPY "player1.txt" DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(COPY "player2.txt" DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

