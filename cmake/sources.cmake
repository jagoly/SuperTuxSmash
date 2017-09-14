cmake_minimum_required(VERSION 3.4)

set(SOURCES

  "src/main.cpp"

  #----------------------------------------------------------#

  "src/main/Options.hpp"
  "src/main/Options.cpp"

  "src/main/SmashApp.hpp"
  "src/main/SmashApp.cpp"

  #----------------------------------------------------------#

  "src/game/forward.hpp"

  "src/game/FightWorld.hpp"
  "src/game/FightWorld.cpp"

  "src/game/Blobs.hpp"
  "src/game/Blobs.cpp"

  "src/game/GameScene.hpp"
  "src/game/GameScene.cpp"

  "src/game/Actions.hpp"
  "src/game/Actions.cpp"

  "src/game/Controller.hpp"
  "src/game/Controller.cpp"

  "src/game/Fighter.hpp"
  "src/game/Fighter.cpp"

  "src/game/Stage.hpp"
  "src/game/Stage.cpp"

  #----------------------------------------------------------#

  "src/render/Renderer.hpp"
  "src/render/Renderer.cpp"

  "src/render/RenderObject.hpp"
  "src/render/RenderObject.cpp"

  #----------------------------------------------------------#

  "src/stages/TestZone_Stage.hpp"
  "src/stages/TestZone_Stage.cpp"
  "src/stages/TestZone_Render.hpp"
  "src/stages/TestZone_Render.cpp"

  #----------------------------------------------------------#

  "src/fighters/Sara_Actions.hpp"
  "src/fighters/Sara_Actions.cpp"
  "src/fighters/Sara_Fighter.hpp"
  "src/fighters/Sara_Fighter.cpp"
  "src/fighters/Sara_Render.hpp"
  "src/fighters/Sara_Render.cpp"

  "src/fighters/Tux_Actions.hpp"
  "src/fighters/Tux_Actions.cpp"
  "src/fighters/Tux_Fighter.hpp"
  "src/fighters/Tux_Fighter.cpp"
  "src/fighters/Tux_Render.hpp"
  "src/fighters/Tux_Render.cpp"
)
