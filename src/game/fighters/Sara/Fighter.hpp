#pragma once

#include <game/Fighter.hpp>

namespace sts { namespace fighters {

//============================================================================//

class Sara_Fighter final : public Fighter
{
public:

    //========================================================//

    Sara_Fighter();
    ~Sara_Fighter();

    void setup() override;
    void tick() override;
    void integrate() override;
    void render_depth() override;
    void render_main() override;

    //========================================================//

private:

    sq::Armature ARMA_Sara;
    sq::Mesh MESH_Sara;

    sq::UniformBuffer UBO_Sara;

    sq::Armature::Pose POSE_Rest;
    sq::Armature::Pose POSE_Stand;
    sq::Armature::Pose POSE_Jump;

    sq::Armature::Animation ANIM_Walk;

    sq::Texture2D TX_Main_diff { sq::Texture::Format::RGB8_UN };
    sq::Texture2D TX_Main_spec { sq::Texture::Format::R8_UN };

    sq::Texture2D TX_Hair_diff { sq::Texture::Format::RGB8_UN };
    sq::Texture2D TX_Hair_norm { sq::Texture::Format::RGB8_SN };
    sq::Texture2D TX_Hair_mask { sq::Texture::Format::R8_UN };

    sq::Shader VS_Sara { sq::Shader::Stage::Vertex };
    sq::Shader FS_Main { sq::Shader::Stage::Fragment };
    sq::Shader FS_Hair { sq::Shader::Stage::Fragment };

    //========================================================//

    sq::Armature::Pose mPosePrevious;
    sq::Armature::Pose mPoseCurrent;

    float mAnimationProgress = 0.f;

    Mat4F mFinalMatrix;
    Mat3F mNormalMatrix;
};

//============================================================================//

}} // namespace sts::fighters
