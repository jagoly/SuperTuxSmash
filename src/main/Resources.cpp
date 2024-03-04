#include "main/Resources.hpp"

#include "game/VisualEffect.hpp"

#include <sqee/misc/Json.hpp>

#include <sqee/objects/Mesh.hpp>
#include <sqee/objects/Pipeline.hpp>
#include <sqee/objects/Sound.hpp>
#include <sqee/objects/Texture.hpp>

#include <sqee/vk/VulkanContext.hpp>

using namespace sts;

//============================================================================//

ResourceCaches::ResourceCaches(sq::AudioContext& audio)
    : mAudioContext(audio)
{
    const auto& ctx = sq::VulkanContext::get();

    // The current texture limit of 60 is because some gpus, including the intel passthrough I use
    // in my windows vm, has maxPerStageDescriptorSamplers = 64, leaving 4 slots for other textures.
    //
    // Right now, STS can get by with this small limit, but it will almost certainly become an
    // issue in the future. One solution would be to seperate samplers from images, as the limit for
    // sampledImages is 200 (and is generally higher for most hardware).
    //
    // Really, I probably jumped the gun switching everything to bindless, but I really don't have
    // any interest at this point in maintaining a seperate texture slot system.

    bindlessTextureSetLayout = ctx.create_descriptor_set_layout (
        vk::DescriptorSetLayoutBinding(0u, vk::DescriptorType::eCombinedImageSampler, 60u, vk::ShaderStageFlagBits::eFragment),
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

    cubeTextures.assign_factory([](const String& key)
    {
        auto result = sq::Texture();
        result.load_from_file_cube("assets/" + key);
        return result;
    });

    pipelines.assign_factory([this](const String& key)
    {
        auto result = sq::Pipeline();
        result.load_from_minified_json(key, passConfigMap);
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
    // todo: remove from bindless set and reuse indices
    //textures.free_unreachable();
    cubeTextures.free_unreachable();
    meshes.free_unreachable();

    pipelines.reload_resources();
}
