#pragma once

#include "setup.hpp"

#include "main/MainEnums.hpp"

#include "game/FighterEnums.hpp"
#include "game/Physics.hpp"

#include <sqee/objects/Armature.hpp>
#include <sqee/vk/SwapBuffer.hpp>

namespace sts {

//============================================================================//

class Fighter final : sq::NonCopyable
{
public: //====================================================//

    using AnimMode = FighterAnimMode;
    using EdgeStopMode = FighterEdgeStopMode;

    //--------------------------------------------------------//

    /// One animation with related metadata.
    struct Animation
    {
        sq::Armature::Animation anim;
        AnimMode mode;

        /// Will the animation repeat when reaching the end.
        bool is_looping() const
        {
            return mode == AnimMode::Loop || mode == AnimMode::WalkLoop || mode == AnimMode::DashLoop;
        }

        /// Is the animation just a T-Pose because it failed to load.
        bool is_fallback() const
        {
            return anim.frameCount == 1u;
        }

        const SmallString& get_key() const
        {
            return *std::prev(reinterpret_cast<const SmallString*>(this));
        }
    };

    //--------------------------------------------------------//

    /// Stats that generally don't change during a game.
    struct Attributes
    {
        // todo: change to camelCase for consistency

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
        bool vertigo = false;
        bool onGround = true;
        bool onPlatform = false;

        float moveMobility = 0.f;
        float moveSpeed = 0.f;

        float damage = 0.f;
        float shield = SHIELD_MAX_HP;
        float launchSpeed = 0.f;

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

    Fighter(FightWorld& world, FighterEnum type, uint8_t index);

    ~Fighter();

    //--------------------------------------------------------//

    FightWorld& world;

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

    /// Call the appropriate function based on current state.
    void apply_hit_generic(const HitBlob& hit, const HurtBlob& hurt);

    /// Called when hit while on the ground.
    //void apply_hit_ground(const HitBlob& hit, const HurtBlob& hurt);

    /// Called when hit while in the air.
    //void apply_hit_air(const HitBlob& hit, const HurtBlob& hurt);

    /// Called when hit while shield is active.
    //void apply_hit_shield(const HitBlob& hit, const HurtBlob& hurt);

    /// Called when shield is broken by attack or decay.
    void apply_shield_break();

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

    //-- wren methods ----------------------------------------//

    WrenHandle* wren_get_library() { return mLibraryHandle; }

    void wren_log(StringView message);

    void wren_cxx_clear_action() { activeAction = nullptr; }

    void wren_cxx_assign_action(SmallString key);

    void wren_cxx_assign_state(SmallString key);

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

private: //===================================================//

    /// Used for making sure facing changes look good.
    enum class RotateMode : uint8_t
    {
        Auto       = 0,
        Clockwise  = 1 << 0,
        Slow       = 1 << 1,
        Animation  = 1 << 2,
        Playing    = 1 << 3,
        Done       = 1 << 4
    };

    friend RotateMode operator|(RotateMode a, RotateMode b) { return RotateMode(uint8_t(a) | uint8_t(b)); }
    friend RotateMode operator&(RotateMode a, RotateMode b) { return RotateMode(uint8_t(a) & uint8_t(b)); }

    //-- init methods, called by constructor -----------------//

    void initialise_armature();

    void initialise_attributes();

    void initialise_hurtblobs();

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

    void update_action();

    void update_state();

    void update_animation();

    void update_frozen();

    //--------------------------------------------------------//

    sq::Armature mArmature;

    sq::SwapBuffer mSkellyUbo;
    sq::Swapper<vk::DescriptorSet> mDescriptorSet;

    std::map<TinyString, HurtBlob> mHurtBlobs;
    std::map<SmallString, Animation> mAnimations;
    std::map<SmallString, FighterAction> mActions;
    std::map<SmallString, FighterState> mStates;

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

WRENPLUS_TRAITS_HEADER(sts::Fighter::Attributes)
WRENPLUS_TRAITS_HEADER(sts::Fighter::Variables)
WRENPLUS_TRAITS_HEADER(sts::Fighter)
