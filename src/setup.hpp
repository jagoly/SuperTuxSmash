#pragma once

#include <sqee/setup.hpp> // IWYU pragma: export

#include <sqee/core/Algorithms.hpp> // IWYU pragma: export
#include <sqee/core/EnumHelper.hpp> // IWYU pragma: export
#include <sqee/core/Macros.hpp> // IWYU pragma: export
#include <sqee/core/Types.hpp> // IWYU pragma: export
#include <sqee/core/Utilities.hpp> // IWYU pragma: export

#include <sqee/misc/PoolTools.hpp> // IWYU pragma: export
#include <sqee/misc/StackVector.hpp> // IWYU pragma: export

#include <fmt/format.h> // IWYU pragma: export

//============================================================================//

namespace sts {

using namespace fmt::literals;
using namespace sq::coretypes;

using sq::PoolMap;
using sq::StackVector;

namespace algo = sq::algo;
namespace maths = sq::maths;

class Action;
class Camera;
class Controller;
class FightWorld;
class Fighter;
class ParticleSystem;
class RenderObject;
class Renderer;
class SmashApp;
class Stage;

struct Emitter;
struct GameSetup;
struct HitBlob;
struct HurtBlob;
struct Options;

class EditorScene;
struct DebugGui;

} // namespace sts
