#pragma once

#include "setup.hpp" // IWYU pragma: export

#include "main/Resources.hpp"

namespace sts {

//============================================================================//

class RenderObject : sq::NonCopyable
{
public: //====================================================//

    RenderObject(Renderer& renderer) : renderer(renderer) {}

    virtual ~RenderObject() = default;

    //--------------------------------------------------------//

    virtual void integrate(float blend) = 0;

    virtual void render_opaque() const = 0;

    virtual void render_transparent() const = 0;

protected: //=================================================//

    Renderer& renderer;

    //--------------------------------------------------------//

    struct OpaqueObject
    {
        const bool* condition = nullptr;
        MaterialHandle material = nullptr;
        MeshHandle mesh = nullptr;

        bool invertCondition = false;
        int8_t subMesh = -1;
    };

    std::vector<OpaqueObject> mOpaqueObjects;

    //--------------------------------------------------------//

    void base_load_from_json(const String& path, std::map<TinyString, const bool*> conditions);

    void base_render_opaque() const;
};

//============================================================================//

} // namespace sts
