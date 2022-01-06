#pragma once

#include "setup.hpp"

#include "main/MainEnums.hpp"

#include "game/Physics.hpp"

#include <sqee/objects/Armature.hpp>
#include <sqee/vk/SwapBuffer.hpp>

namespace sts {

//============================================================================//

class Fighter final : sq::NonCopyable
{
public: //====================================================//

    /// Describes when to stop at the edge of a platform.
    enum class EdgeStopMode : uint8_t
    {
        Never,  ///< Never stop at edges.
        Always, ///< Always stop at edges
        Input,  ///< Stop if axis not pushed.
    };

    //--------------------------------------------------------//

    /// One animation with related metadata.
    struct Animation
    {
        sq::Armature::Animation anim;

        bool loop{};     ///< Repeat the animation when reaching the end.
        bool motion{};   ///< Extract offset from bone0 and use it to try to move.
        bool turn{};     ///< Extract rotation from bone2 and apply it to the model matrix.
        bool attach{};   ///< Extract offset from bone0 and move relative to attachPoint.
        bool walk{};     ///< Update the animation based on velocity.x and walkAnimStride.
        bool dash{};     ///< Update the animation based on velocity.x and dashAnimStride.
        bool fallback{}; ///< The animation failed to load and will T-Pose instead.

        const SmallString& get_key() const
        {
            return *std::prev(reinterpret_cast<const SmallString*>(this));
        }
    };

    //--------------------------------------------------------//

    /// Stats that generally don't change during a game.
    struct Attributes
    {
        float walkSpeed   = 0.1f;
        float dashSpeed   = 0.15f;
        float airSpeed    = 0.1f;
        float traction    = 0.005f;
        float airMobility = 0.008f;
        float airFriction = 0.002f;

        float hopHeight     = 1.5f;
        float jumpHeight    = 3.5f;
        float airHopHeight  = 3.5f;
        float gravity       = 0.01f;
        float fallSpeed     = 0.15f;
        float fastFallSpeed = 0.24f;
        float weight        = 100.f;

        float walkAnimStride = 2.f;
        float dashAnimStride = 3.f;

        uint extraJumps    = 2u;
        uint lightLandTime = 4u;
    };

    //--------------------------------------------------------//

    /// Miscellaneous public information and variables.
    struct Variables
    {
        Vec2F position = { 0.f, 0.f };
        Vec2F velocity = { 0.f, 0.f };

        int8_t facing = +1;

        uint8_t extraJumps = 0u;
        uint8_t lightLandTime = 0u;
        uint8_t noCatchTime = 0u;
        uint8_t stunTime = 0u;
        uint8_t freezeTime = 0u;

        EdgeStopMode edgeStop = {};

        bool intangible = false;
        bool fastFall = false;
        bool applyGravity = true;
        bool applyFriction = true;
        bool flinch = false;

        bool onGround = true;
        bool onPlatform = false;

        int8_t edge = 0;

        float moveMobility = 0.f;
        float moveSpeed = 0.f;

        float damage = 0.f;
        float shield = SHIELD_MAX_HP;
        float launchSpeed = 0.f;

        Vec2F attachPoint = { 0.f, 0.f };

        Ledge* ledge = nullptr;
    };

    //--------------------------------------------------------//

    /// Structure for data to interpolate between ticks.
    struct InterpolationData
    {
        Vec3F translation;
        QuatF rotation;
        sq::Armature::Pose pose;
    };

    //--------------------------------------------------------//

    Fighter(World& world, FighterEnum name, uint8_t index);

    ~Fighter();

    //--------------------------------------------------------//

    World& world;

    const FighterEnum name;

    const uint8_t index;

    //--------------------------------------------------------//

    Attributes attributes;

    LocalDiamond localDiamond;

    Variables variables;

    Controller* controller = nullptr;

    FighterAction* activeAction = nullptr;

    FighterState* activeState = nullptr;

    InterpolationData previous, current;

    //--------------------------------------------------------//

    void tick();

    void integrate(float blend);

    //--------------------------------------------------------//

    /// Specify that a state or action has encountered an error.
    void set_error_message(void* cause, String message)
    {
        editorErrorCause = cause;
        editorErrorMessage = std::move(message);
    }

    /// Specify that the error from a state or action has been resolved.
    void clear_error_message(void* cause)
    {
        if (cause == editorErrorCause)
        {
            editorErrorCause = nullptr;
            editorErrorMessage = "";
        }
    }

    //--------------------------------------------------------//

    /// Deserialise a bone name from json.
    int8_t bone_from_json(const JsonValue& json) const;

    /// Serialise a bone name to json.
    JsonValue bone_to_json(int8_t bone) const;

    //--------------------------------------------------------//

    /// Called when a HitBlob hits one of our HurtBlobs.
    void apply_hit(const HitBlob& hit, const HurtBlob& hurt);

    /// Called when passing the stage boundary.
    void pass_boundary();

    //-- private member access methods -----------------------//

    /// Get the fighter's bone heirarchy.
    const sq::Armature& get_armature() const { return mArmature; }

    /// Get the current model matrix.
    const Mat4F& get_model_matrix() const { return mModelMatrix; }

    /// Get the model matrix for the frame being rendered.
    const Mat4F& get_blended_model_matrix() const { return mBlendedModelMatrix; }

    /// Compute current matrix for a bone. If bone is -1, returns model matrix.
    Mat4F get_bone_matrix(int8_t bone) const;

    /// Compute blended matrix for a bone. If bone is -1, returns model matrix.
    Mat4F get_blended_bone_matrix(int8_t bone) const;

    //-- wren methods ----------------------------------------//

    World* wren_get_world() { return &world; }

    WrenHandle* wren_get_library() { return mLibraryHandle; }

    void wren_log(StringView message);

    void wren_cxx_clear_action() { activeAction = nullptr; }

    void wren_cxx_assign_action(SmallString key);

    void wren_cxx_assign_state(TinyString key);

    void wren_reverse_facing_auto();

    void wren_reverse_facing_instant();

    void wren_reverse_facing_slow(bool clockwise, uint8_t time);

    void wren_reverse_facing_animated(bool clockwise);

    bool wren_attempt_ledge_catch();

    void wren_play_animation(SmallString key, uint fade, bool fromStart);

    void wren_set_next_animation(SmallString key, uint fade);

    void wren_reset_collisions();

    void wren_enable_hurtblob(TinyString key);

    void wren_disable_hurtblob(TinyString key);

    void wren_play_sound(SmallString key);

    void wren_cancel_sound(SmallString key);

private: //===================================================//

    /// Used for making sure facing changes look good.
    enum class RotateMode : uint8_t
    {
        Auto       = 0,      ///< Not doing anything special.
        Clockwise  = 1 << 0, ///< Should guided lerp go clockwise or anticlockwise.
        Slow       = 1 << 1, ///< Rotating over a number of frames.
        Animation  = 1 << 2, ///< Fading a turn animation, using guided lerp.
        Playing    = 1 << 3, ///< Already playing a turn animation.
        Done       = 1 << 4, ///< Done, but keep guiding until next tick.
    };

    friend RotateMode operator|(RotateMode a, RotateMode b) { return RotateMode(uint8_t(a) | uint8_t(b)); }
    friend RotateMode operator&(RotateMode a, RotateMode b) { return RotateMode(uint8_t(a) & uint8_t(b)); }

    //-- init methods, called by constructor -----------------//

    void initialise_armature();

    void initialise_attributes();

    void initialise_hurtblobs();

    void initialise_sounds();

    void initialise_animations();

    void initialise_actions();

    void initialise_states();

    //-- methods used internally or by the editor ------------//

    void start_action(FighterAction& action);

    void cancel_action();

    void change_state(FighterState& state);

    void play_animation(const Animation& animation, uint fade, bool fromStart);

    void set_next_animation(const Animation& animation, uint fade);

    void reset_everything();

    //-- update methods, called each tick --------------------//

    void update_movement();

    void update_misc();

    void update_action();

    void update_state();

    void update_animation();

    void update_frozen();

    //--------------------------------------------------------//

    sq::Armature mArmature;

    sq::SwapBuffer mSkellyUbo;
    sq::Swapper<vk::DescriptorSet> mDescriptorSet;

    std::map<TinyString, HurtBlob> mHurtBlobs;
    std::map<SmallString, SoundEffect> mSounds;
    std::map<SmallString, Animation> mAnimations;
    std::map<SmallString, FighterAction> mActions;
    std::map<TinyString, FighterState> mStates;

    WrenHandle* mLibraryHandle = nullptr;

    //--------------------------------------------------------//

    const Animation* mAnimation = nullptr;
    const Animation* mNextAnimation = nullptr;

    uint mFadeFrames = 0u;
    uint mNextFadeFrames = 0u;
    uint mFadeProgress = 0u;

    uint mAnimTimeDiscrete = 0u;
    float mAnimTimeContinuous = 0.f;

    Vec3F mRootMotionPreviousOffset;
    Vec2F mRootMotionTranslate;

    Vec2F mFadeStartPosition;
    QuatF mFadeStartRotation;
    sq::Armature::Pose mFadeStartPose;

    Mat4F mModelMatrix;
    Mat4F mBlendedModelMatrix;

    std::vector<Mat34F> mBoneMatrices;

    RotateMode mRotateMode = RotateMode::Auto;

    uint8_t mRotateSlowTime = 0u;
    uint8_t mRotateSlowProgress = 0u;

    uint8_t mJitterCounter = 0u;

    //-- debug and editor stuff ------------------------------//

    String debugPreviousPoseInfo;
    String debugCurrentPoseInfo;
    String debugAnimationFadeInfo;

    void* editorErrorCause = nullptr;
    String editorErrorMessage = "";

    FighterAction* editorStartAction = nullptr;

    friend DebugGui;
    friend EditorScene;
};

//============================================================================//

} // namespace sts

SQEE_ENUM_HELPER
(
    sts::Fighter::EdgeStopMode,
    Never, Always, Input
)

WRENPLUS_TRAITS_HEADER(sts::Fighter::Attributes)
WRENPLUS_TRAITS_HEADER(sts::Fighter::Variables)
WRENPLUS_TRAITS_HEADER(sts::Fighter)
