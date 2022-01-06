#pragma once

#include <sqee/setup.hpp> // IWYU pragma: export

#include <sqee/core/Algorithms.hpp> // IWYU pragma: export
#include <sqee/core/EnumHelper.hpp> // IWYU pragma: export
#include <sqee/core/Macros.hpp> // IWYU pragma: export
#include <sqee/core/Types.hpp> // IWYU pragma: export
#include <sqee/core/Utilities.hpp> // IWYU pragma: export

#include <sqee/misc/StackVector.hpp> // IWYU pragma: export

#include <sqee/app/WrenForward.hpp> // IWYU pragma: export

#include <fmt/format.h> // IWYU pragma: export

namespace sts {

//============================================================================//

using namespace fmt::literals;
using namespace sq::coretypes;

namespace algo = sq::algo;
namespace maths = sq::maths;

using sq::StackString;
using sq::StackVector;

//============================================================================//

struct DebugGui;
struct Emitter;
struct GameSetup;
struct HitBlob;
struct HurtBlob;
struct Ledge;
struct LocalDiamond;
struct MoveAttempt;
struct Options;
struct SoundEffect;
struct VisualEffect;

class Camera;
class Controller;
class EditorCamera;
class EditorScene;
class EffectSystem;
class Fighter;
class FighterAction;
class FighterState;
class ParticleSystem;
class Renderer;
class ResourceCaches;
class SmashApp;
class Stage;
class StandardCamera;
class World;

//============================================================================//

/// Simple AAAB which SuperTuxSmash uses in many places.
template <class Type>
struct MinMax { Type min = Type(+INFINITY), max = Type(-INFINITY); };

//============================================================================//

/// Maximum number of players and fighters in one game.
constexpr const size_t MAX_FIGHTERS = 4u;

/// Number of frames that input commands are buffered for.
constexpr const size_t CMD_BUFFER_SIZE = 9u;

/// Maximum number of bones in a visual effect.
constexpr const size_t MAX_EFFECT_BONES = 8u;

/// Maximum number of visual effects that can be active at once.
constexpr const size_t MAX_ACTIVE_EFFECTS = 16u;

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

/// Maximum number of particles that can be on screen at once.
constexpr const size_t MAX_PARTICLES = 8192u;

/// Maximum number of thick lines that can be on screen at once.
constexpr const size_t MAX_THICK_LINES = 1024u;

/// Maximum number of thin lines that can be on screen at once.
constexpr const size_t MAX_THIN_LINES = 2048u;

//============================================================================//

constexpr const uint SKYBOX_SIZE     = 2048u;
constexpr const uint IRRADIANCE_SIZE = 32u;
constexpr const uint RADIANCE_SIZE   = 128u;
constexpr const uint RADIANCE_LEVELS = 6u;

constexpr const uint SHADOW_MAP_BASE_SIZE = 2048u;

//============================================================================//

} // namespace sts

WRENPLUS_TRAITS_HEADER(sq::coretypes::Vec2I)
WRENPLUS_TRAITS_HEADER(sq::coretypes::Vec2F)
