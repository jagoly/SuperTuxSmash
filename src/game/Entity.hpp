#pragma once

#include "setup.hpp"

#include "game/EntityDef.hpp"

#include "render/AnimPlayer.hpp"

namespace sts {

//============================================================================//

class Entity
{
public: //====================================================//

    /// Structure for data to interpolate between ticks.
    struct InterpolationData
    {
        Vec3F translation;
        QuatF rotation;
    };

    /// Variables that exist for all types of entities.
    struct EntityVars
    {
        Vec2F position = { 0.f, 0.f };
        Vec2F velocity = { 0.f, 0.f };

        int8_t facing = +1;
        uint8_t freezeTime = 0u;

        // todo: replace with a list of entities
        bool hitSomething = false;

        float animTime = 0.f;

        Vec2F attachPoint = { 0.f, 0.f };
    };

    //--------------------------------------------------------//

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

    //--------------------------------------------------------//

    Entity(const EntityDef& def);

    SQEE_COPY_DELETE(Entity)
    SQEE_MOVE_DELETE(Entity)

    virtual ~Entity();

    //--------------------------------------------------------//

    World& world;

    const int32_t eid;

    InterpolationData previous, current;

    //--------------------------------------------------------//

    const AnimPlayer& get_anim_player() const { return mAnimPlayer; }

    const sq::Armature& get_armature() const { return mAnimPlayer.armature; }

    /// Access this entity's active hit blobs.
    std::vector<HitBlob>& get_hit_blobs() { return mHitBlobs; }

    /// List of entities that we've hit since the last reset.
    StackVector<int32_t, 15u>& get_ignore_collisions() { return mIgnoreCollisions; }

    //--------------------------------------------------------//

    /// Compute a bone's matrix for the current tick. If index is -1, returns model matrix.
    Mat4F get_bone_matrix(int8_t index) const;

    /// Compute a bone's matrix for the frame being rendered. If bone is -1, returns model matrix.
    Mat4F get_blended_bone_matrix(int8_t index) const;

    //--------------------------------------------------------//

    virtual const EntityDef& get_def() const = 0;

    virtual EntityVars& get_vars() = 0;

    virtual const EntityVars& get_vars() const = 0;

    //-- wren methods ----------------------------------------//

    const TinyString& wren_get_name() { return get_def().name; }

    World* wren_get_world() { return &world; }

    void wren_reverse_facing_auto();

    void wren_reverse_facing_instant();

    void wren_reverse_facing_slow(bool clockwise, uint8_t time);

    void wren_reverse_facing_animated(bool clockwise);

    void wren_reset_collisions();

    void wren_play_animation(SmallString key, uint fade, bool fromStart);

    void wren_set_next_animation(SmallString key, uint fade);

    int32_t wren_play_sound(SmallString key, bool stopWithAction);

    void impl_wren_enable_hitblobs(const std::map<TinyString, HitBlobDef>& blobs, StringView prefix);

    int32_t impl_wren_play_effect(const std::map<TinyString, VisualEffectDef>& effects, TinyString key);

    void impl_wren_emit_particles(const std::map<TinyString, Emitter>& emitters, TinyString key);

protected: //=================================================//

    //-- methods used internally or by the editor ------------//

    void play_animation(const Animation& animation, uint fade, bool fromStart);

    void set_next_animation(const Animation& animation, uint fade);

    //-- methods called by derived classes -------------------//

    void update_animation();

    void integrate_base(float blend);

    //--------------------------------------------------------//

    AnimPlayer mAnimPlayer;

    Mat4F mModelMatrix;

    StackVector<int32_t, 7u> mTransientSounds;
    StackVector<int32_t, 7u> mTransientEffects;

    std::vector<HitBlob> mHitBlobs;

    StackVector<int32_t, 15u> mIgnoreCollisions;

    const Animation* mNextAnimation = nullptr;

    uint mFadeFrames = 0u;
    uint mNextFadeFrames = 0u;
    uint mFadeProgress = 0u;

    Vec3F mRootMotionPreviousOffset;
    Vec2F mRootMotionTranslate;

    Vec2F mFadeStartPosition;
    QuatF mFadeStartRotation;
    sq::AnimSample mFadeStartSample;

    RotateMode mRotateMode = RotateMode::Auto;

    uint8_t mRotateSlowTime = 0u;
    uint8_t mRotateSlowProgress = 0u;

    //-- debug and editor stuff ------------------------------//

    String debugPreviousPoseInfo;
    String debugCurrentPoseInfo;
    String debugAnimationFadeInfo;

    friend DebugGui;
    friend EditorScene;
};

//============================================================================//

} // namespace sts
