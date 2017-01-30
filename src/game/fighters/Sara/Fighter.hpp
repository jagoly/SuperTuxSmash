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
    void render() override;

    //========================================================//

private:

    sq::Armature ARMA_Sara;
    sq::Mesh MESH_Sara;

    sq::UniformBuffer UBO_Sara;

    sq::Armature::Pose POSE_Rest;
    sq::Armature::Pose POSE_Stand;
    sq::Armature::Pose POSE_Jump;

    std::array<sq::Armature::Pose, 8> ANIM_Walk;

    sq::Texture2D TX_Main_diff { sq::Texture::Format::RGB8_UN };
    sq::Texture2D TX_Main_spec { sq::Texture::Format::R8_UN };

    sq::Texture2D TX_Hair_diff { sq::Texture::Format::RGB8_UN };
    sq::Texture2D TX_Hair_norm { sq::Texture::Format::RGB8_SN };
    sq::Texture2D TX_Hair_mask { sq::Texture::Format::R8_UN };

    sq::Shader VS_Sara { sq::Shader::Stage::Vertex };
    sq::Shader FS_Main { sq::Shader::Stage::Fragment };
    sq::Shader FS_Hair { sq::Shader::Stage::Fragment };

    sq::Armature::Pose mPosePrevious;
    sq::Armature::Pose mPoseCurrent;

    bool mAnimationSwitch = false;
    float mAnimationProgress = 0.f;
    uint mAnimationIndex = 0u;
};

//============================================================================//

}} // namespace sts::fighters
