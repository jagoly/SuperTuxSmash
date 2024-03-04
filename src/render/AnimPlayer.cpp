#include "render/AnimPlayer.hpp"

#include "render/Camera.hpp"
#include "render/Renderer.hpp"

// notes on bone vs. matrix indices
//  - bone index:
//     - first bone is index 0, -1 means none
//     - defs always use bone indices
//  - matrix index:
//     - first bone is index 1, 0 means the entity itself
//     - shaders always use matrix indices
//     - sq::Armature has no concept of matrix indices

using namespace sts;

//============================================================================//

AnimPlayer::AnimPlayer(const sq::Armature& armature) : armature(armature)
{
    previousSample.resize(armature.get_rest_sample().size());
    currentSample.resize(armature.get_rest_sample().size());
    blendSample.resize(armature.get_rest_sample().size());

    debugEnableBlend.resize(armature.get_bone_count(), char(true));
}

//============================================================================//

void AnimPlayer::integrate(Renderer& renderer, const Mat4F& modelMatrix, float bbScaleX, float blend)
{
    armature.blend_samples(previousSample, currentSample, blend, blendSample);

    for (size_t bone = 0u; bone < armature.get_bone_count(); ++bone)
    {
        if (bool(debugEnableBlend[bone]) == false)
        {
            sq::Armature::Bone* blendSampleBones = reinterpret_cast<sq::Armature::Bone*>(blendSample.data());
            sq::Armature::Bone* currentSampleBones = reinterpret_cast<sq::Armature::Bone*>(currentSample.data());
            blendSampleBones[bone] = currentSampleBones[bone];
        }
    }

    const CameraBlock& camera = renderer.get_camera().get_block();

    Mat34F* modelMats = renderer.reserve_matrices(1u + uint(armature.get_bone_count()), modelMatsIndex);

    modelMats[0] = Mat34F(maths::transpose(modelMatrix));

    armature.compute_model_matrices (
        blendSample, camera.viewMat, camera.invViewMat, modelMatrix, Vec2F(bbScaleX, 1.f),
        modelMats + 1, armature.get_bone_count()
    );

    // todo: if animation has uniform scale, just set normalMatsIndex to modelMatsIndex
    Mat34F* normalMats = renderer.reserve_matrices(1u + uint(armature.get_bone_count()), normalMatsIndex);

    for (size_t i = 0u; i <= armature.get_bone_count(); ++i)
        normalMats[i] = Mat34F(maths::inverse(maths::transpose(Mat3F(modelMats[i]))));
}
