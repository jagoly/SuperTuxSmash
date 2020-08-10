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

namespace sts {

//============================================================================//

using namespace fmt::literals;
using namespace sq::coretypes;

namespace algo = sq::algo;
namespace maths = sq::maths;

using sq::PoolMap;
using sq::StackVector;

//============================================================================//

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

//============================================================================//

constexpr const size_t  MAX_FIGHTERS          = 4u;
constexpr const size_t  MAX_HITBLOB_GROUPS    = 8u;
constexpr const size_t  CMD_BUFFER_SIZE       = 8u;
constexpr const uint8_t LANDING_LAG_LIGHT     = 2u;
constexpr const uint8_t LANDING_LAG_HEAVY     = 8u;
constexpr const uint8_t JUMP_DELAY            = 5u;
constexpr const uint8_t LEDGE_CATCH_NOPE_TIME = 48u;
constexpr const uint8_t LEDGE_HANG_MIN_TIME   = 16u;
constexpr const uint8_t MIN_HITSTUN_HEAVY     = 16u;
constexpr const uint8_t MIN_HITSTUN_TUMBLE    = 32u;
constexpr const float   KNOCKBACK_DECAY       = 0.0051f;

//============================================================================//

} // namespace sts
