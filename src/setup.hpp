#pragma once

#include <sqee/setup.hpp> // IWYU pragma: export

#include <sqee/core/Algorithms.hpp> // IWYU pragma: export
#include <sqee/core/EnumHelper.hpp> // IWYU pragma: export
#include <sqee/core/Macros.hpp> // IWYU pragma: export
#include <sqee/core/Types.hpp> // IWYU pragma: export
#include <sqee/core/Utilities.hpp> // IWYU pragma: export

//#include <sqee/misc/PoolTools.hpp> // IWYU pragma: export
#include <sqee/misc/StackVector.hpp> // IWYU pragma: export

#include <fmt/format.h> // IWYU pragma: export

#include <memory_resource> // IWYU pragma: export

namespace sts {

//============================================================================//

using namespace fmt::literals;
using namespace sq::coretypes;

namespace algo = sq::algo;
namespace maths = sq::maths;

//using sq::PoolMap;
using sq::StackString;
using sq::StackVector;

//============================================================================//

class MeshCache;
class ProgramCache;
class SoundCache;
class TexArrayCache;
class TextureCache;

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
struct SoundEffect;

class EditorScene;
struct DebugGui;

//============================================================================//

/// Maximum number of players and fighters in one game.
constexpr const size_t MAX_FIGHTERS = 4u;

/// Maximum number of hits in one action.
constexpr const size_t MAX_HITBLOB_GROUPS = 8u;

/// Number of frames that input commands are buffered for.
constexpr const size_t CMD_BUFFER_SIZE = 8u;

/// Length of window to release jump and perform a short hop.
constexpr const uint8_t JUMP_DELAY = 6u;

/// Minimum time after releasing a ledge before being able to catch again.
constexpr const uint8_t LEDGE_CATCH_NOPE_TIME = 48u;

/// Minimum time to hang from a ledge before being able to drop, climb, etc.
constexpr const uint8_t LEDGE_HANG_MIN_TIME = 16u;

/// Minimum amount of hitstun required to cause heavy flinch animations.
constexpr const uint8_t MIN_HITSTUN_HEAVY = 16u;

/// Minimum amount of hitstun required to cause tumbling.
constexpr const uint8_t MIN_HITSTUN_TUMBLE = 32u;

/// Rate that speed decays after being launched into the air.
constexpr const float KNOCKBACK_DECAY = 0.0051f;

/// Maxiumum amount of damage a shield can take before breaking.
constexpr const float SHIELD_MAX_HP = 50.f;

/// Rate that shield loses hp when it is active.
constexpr const float SHIELD_DECAY = 0.15f;

/// Rate that shield gains hp when it is not active.
constexpr const float SHIELD_REGEN = 0.08f;

/// Base amount of shield stun applied by hits regardless of damage.
constexpr const float SHIELD_STUN_BASE = 2.f;

/// Multiply with hit damage to apply shield stun when hit.
constexpr const float SHIELD_STUN_FACTOR = 1.f;

/// Base amount of push back for a fighter hit while shielding.
constexpr const float SHIELD_PUSH_HURT_BASE = 0.035f;

/// Multiply with hit damage to get the push back for a fighter hit while shielding.
constexpr const float SHIELD_PUSH_HURT_FACTOR = 0.008f;

/// Base amount of push back for a fighter hitting a shield.
constexpr const float SHIELD_PUSH_HIT_BASE = 0.005f;

/// Multiply with hit damage to get the push back for a fighter hitting a shield.
constexpr const float SHIELD_PUSH_HIT_FACTOR = 0.004f;

//============================================================================//

} // namespace sts
