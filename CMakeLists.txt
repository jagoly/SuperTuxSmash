project(SuperTuxSmash_Game)
cmake_minimum_required(VERSION 3.0)

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

option(STS_BUNDLED_LIBS "Use bundled libraries from extlibs" TRUE)

################################################################################

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif ()

if (CMAKE_COMPILER_IS_GNUCC)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wextra -Wno-comment")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter -Wno-strict-aliasing")

    if (STS_LINUX)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    elseif (STS_WINDOWS)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows -Wa,-mbig-obj")
    endif ()

else ()

    message(FATAL_ERROR "Non-GNU compilers are not yet supported, please help fix this :)")

endif ()

################################################################################

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build")

################################################################################

set(SOURCES

  "src/main.cpp"

  ############################################################

  "src/main/Options.hpp"
  "src/main/Options.cpp"

  "src/main/SmashApp.hpp"
  "src/main/SmashApp.cpp"

  ############################################################

  "src/scenes/GameScene.hpp"
  "src/scenes/GameScene.cpp"

  "src/scenes/MenuScene.hpp"
  "src/scenes/MenuScene.cpp"

  ############################################################

  "src/game/Actions.hpp"
  "src/game/Actions.cpp"

  "src/game/Controller.hpp"
  "src/game/Controller.cpp"

  "src/game/Fighter.hpp"
  "src/game/Fighter.cpp"

  "src/game/Misc.hpp"
  "src/game/Misc.cpp"

  "src/game/Renderer.hpp"
  "src/game/Renderer.cpp"

  "src/game/Stage.hpp"
  "src/game/Stage.cpp"

  ############################################################

  "src/game/fighters/Cheese/Fighter.hpp"
  "src/game/fighters/Cheese/Fighter.cpp"

  "src/game/fighters/Sara/Fighter.hpp"
  "src/game/fighters/Sara/Fighter.cpp"

  ############################################################

  "src/game/stages/TestZone/Stage.hpp"
  "src/game/stages/TestZone/Stage.cpp"

)

################################################################################

if (STS_BUNDLED_LIBS)

    include_directories("${CMAKE_SOURCE_DIR}/extlibs/include")

    if (STS_LINUX)
        set(EXTLIBS "${CMAKE_SOURCE_DIR}/extlibs/libs/linux")
    elseif (STS_WINDOWS)
        set(EXTLIBS "${CMAKE_SOURCE_DIR}/extlibs/libs/windows")
    endif ()

    list(APPEND CMAKE_FIND_ROOT_PATH "${EXTLIBS}" "${EXTLIBS}/..")

endif ()

################################################################################

include_directories("${CMAKE_SOURCE_DIR}/src")

add_executable(sts-game ${SOURCES})

################################################################################

find_library(SQEE_LIB sqee /)

find_library(SFML_AUDIO_LIB sfml-audio /)
find_library(SFML_WINDOW_LIB sfml-window /)
find_library(SFML_SYSTEM_LIB sfml-system /)

target_link_libraries(sts-game ${SQEE_LIB} ${SFML_AUDIO_LIB} ${SFML_WINDOW_LIB} ${SFML_SYSTEM_LIB})

################################################################################

file(COPY "assets" DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(COPY "shaders" DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

file(COPY "player1.txt" DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(COPY "player2.txt" DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

