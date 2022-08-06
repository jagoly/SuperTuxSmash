#include "main/Resources.hpp"

#include "game/VisualEffect.hpp"

#include <sqee/app/AudioContext.hpp>

#include <sqee/objects/Mesh.hpp>
#include <sqee/objects/Pipeline.hpp>
#include <sqee/objects/Sound.hpp>
#include <sqee/objects/Texture.hpp>

#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>

#include <sqee/vk/VulkanContext.hpp>

using namespace sts;

//============================================================================//

ResourceCaches::ResourceCaches(sq::AudioContext& audio)
    : mAudioContext(audio)
{
    const auto& ctx = sq::VulkanContext::get();

    bindlessTextureSetLayout = ctx.create_descriptor_set_layout (
        vk::DescriptorSetLayoutBinding(0u, vk::DescriptorType::eCombinedImageSampler, 256u, vk::ShaderStageFlagBits::eFragment),
        vk::Flags(vk::DescriptorBindingFlagBits::ePartiallyBound)// | vk::DescriptorBindingFlagBits::eVariableDescriptorCount
    );

    bindlessTextureSet = ctx.allocate_descriptor_set(ctx.descriptorPool, bindlessTextureSetLayout);

    //-- Assign Factory Functions ----------------------------//

    meshes.assign_factory([](const String& key)
    {
        auto result = sq::Mesh();
        result.load_from_file("assets/" + key);
        return result;
    });

    textures.assign_factory([this](const String& key)
    {
        auto result = sq::Texture();
        result.load_from_file_2D("assets/" + key);
        result.add_to_bindless_descriptor_set(bindlessTextureSet, mNumBindlessTextures);
        ++mNumBindlessTextures;
        return result;
    });

    pipelines.assign_factory([this](const JsonValue& key)
    {
        auto result = sq::Pipeline();
        result.load_from_json(key, passConfigMap);
        return result;
    });

    sounds.assign_factory([this](const String& key)
    {
        auto result = sq::Sound(mAudioContext);
        result.load_from_file("assets/" + key);
        return result;
    });

    effects.assign_factory([this](const String& key)
    {
        auto result = EffectAsset();
        result.load_from_directory("assets/" + key, *this);
        return result;
    });
}

ResourceCaches::~ResourceCaches()
{
    const auto& ctx = sq::VulkanContext::get();

    ctx.device.destroy(bindlessTextureSetLayout);
    ctx.device.free(ctx.descriptorPool, bindlessTextureSet);
}

//============================================================================//

void ResourceCaches::refresh_options()
{
    effects.free_unreachable();
    sounds.free_unreachable();
    pipelines.free_unreachable();
    textures.free_unreachable();
    meshes.free_unreachable();

    pipelines.reload_resources();
}
