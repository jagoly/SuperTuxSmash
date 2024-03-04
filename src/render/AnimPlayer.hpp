#pragma once

#include "setup.hpp"

#include <sqee/objects/Armature.hpp>

namespace sts {

//============================================================================//

/// One animation with related metadata.
struct Animation final
{
    sq::Animation anim;

    bool manual{};   ///< Animation time will be updated manually by scripts.
    bool loop{};     ///< Repeat the animation when reaching the end.
    bool motion{};   ///< Extract offset from bone0 and use it to try to move.
    bool turn{};     ///< Extract rotation from bone2 and apply it to the model matrix.
    bool attach{};   ///< Extract offset from bone0 and move relative to attachPoint.
    bool fallback{}; ///< The animation failed to load and will T-Pose instead.

    const SmallString& get_key() const
    {
        return *std::prev(reinterpret_cast<const SmallString*>(this));
    }
};

//============================================================================//

/// Data required for basic entity animation playback.
struct AnimPlayer final
{
    AnimPlayer(const sq::Armature& armature);

    const sq::Armature& armature;

    const Animation* animation = nullptr;
    float animTime = 0.f;

    sq::AnimSample previousSample;
    sq::AnimSample currentSample;
    sq::AnimSample blendSample;

    // set by integrate, used when computing push constant blocks
    uint modelMatsIndex = 0u;
    uint normalMatsIndex = 0u;

    std::vector<char> debugEnableBlend; // chars because std::vector<bool> is cursed

    void integrate(Renderer& renderer, const Mat4F& modelMatrix, float bbScaleX, float blend);
};

//============================================================================//

} // namespace sts
