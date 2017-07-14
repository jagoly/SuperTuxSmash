#pragma once

#include "game/Renderer.hpp"
#include "game/Fighter.hpp"

namespace sts::fighters {

//============================================================================//

class Sara_Fighter final : public Fighter
{
public: //====================================================//

    Sara_Fighter(Game& game, Controller& controller);
    ~Sara_Fighter();

    //--------------------------------------------------------//

    void setup() override;

    void tick() override;

    void integrate(float blend) override;

    void render_depth() override;

    void render_main() override;

private: //===================================================//

    sq::Armature ARMA_Sara;
    sq::Mesh MESH_Sara;

    sq::Armature::Pose POSE_Rest;
    sq::Armature::Pose POSE_Stand;
    sq::Armature::Pose POSE_Jump;

    sq::Armature::Pose POSE_Act_Neutral;
    sq::Armature::Pose POSE_Act_TiltDown;
    sq::Armature::Pose POSE_Act_TiltForward;
    sq::Armature::Pose POSE_Act_TiltUp;

    sq::Armature::Animation ANIM_Walk;

    sq::Texture2D TX_Main_diff { sq::Texture::Format::RGB8_UN };
    sq::Texture2D TX_Main_spec { sq::Texture::Format::R8_UN };

    sq::Texture2D TX_Hair_diff { sq::Texture::Format::RGB8_UN };
    sq::Texture2D TX_Hair_norm { sq::Texture::Format::RGB8_SN };
    sq::Texture2D TX_Hair_mask { sq::Texture::Format::R8_UN };

    sq::Program PROG_Main;
    sq::Program PROG_Hair;

    //--------------------------------------------------------//

    sq::UniformBuffer mUbo;

    sq::Armature::Pose mPosePrevious;
    sq::Armature::Pose mPoseCurrent;

    float mAnimationProgress = 0.f;

    Mat4F mFinalMatrix;
    Mat3F mNormalMatrix;
};

//============================================================================//

} // namespace sts::fighters
