cmake_minimum_required(VERSION 3.4)

set(SOURCES

  "src/main.cpp"

  "src/DebugGlobals.hpp"
  "src/DebugGlobals.cpp"

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

  "src/game/ActionBuilder.cpp"
  "src/game/ActionBuilder.hpp"

  "src/game/ActionFuncs.cpp"
  "src/game/ActionFuncs.hpp"

  "src/game/Actions.hpp"
  "src/game/Actions.cpp"

  "src/game/Controller.hpp"
  "src/game/Controller.cpp"

  "src/game/Fighter.hpp"
  "src/game/Fighter.cpp"

  "src/game/Stage.hpp"
  "src/game/Stage.cpp"

  "src/game/ParticleSet.hpp"
  "src/game/ParticleSet.cpp"

  #----------------------------------------------------------#

  "src/render/SceneData.hpp"
  "src/render/SceneData.cpp"

  "src/render/DebugRender.hpp"
  "src/render/DebugRender.cpp"

  "src/render/ParticleRender.hpp"
  "src/render/ParticleRender.cpp"

  "src/render/Camera.hpp"
  "src/render/Camera.cpp"

  "src/render/Renderer.hpp"
  "src/render/Renderer.cpp"

  "src/render/RenderObject.hpp"
  "src/render/RenderObject.cpp"

  "src/render/ResourceCaches.hpp"
  "src/render/ResourceCaches.cpp"

  #----------------------------------------------------------#

  "src/stages/TestZone_Stage.hpp"
  "src/stages/TestZone_Stage.cpp"
  "src/stages/TestZone_Render.hpp"
  "src/stages/TestZone_Render.cpp"

  #----------------------------------------------------------#

  "src/fighters/Sara_Fighter.hpp"
  "src/fighters/Sara_Fighter.cpp"
  "src/fighters/Sara_Render.hpp"
  "src/fighters/Sara_Render.cpp"

  "src/fighters/Tux_Fighter.hpp"
  "src/fighters/Tux_Fighter.cpp"
  "src/fighters/Tux_Render.hpp"
  "src/fighters/Tux_Render.cpp"
)
