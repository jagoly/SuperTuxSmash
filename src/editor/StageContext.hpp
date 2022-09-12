#pragma once

#include "setup.hpp"

#include "editor/BaseContext.hpp"

namespace sq { class Texture; }

namespace sts {

//============================================================================//

struct EditorScene::StageContext : BaseContext
{
    StageContext(EditorScene& editor, String ctxKey);

    ~StageContext() override;

    //--------------------------------------------------------//

    void apply_working_changes() override;

    void do_undo_redo(bool redo) override;

    void save_changes() override;

    void show_menu_items() override;

    void show_widgets() override;

    //--------------------------------------------------------//

    void show_widget_stage();

    void show_widget_cubemaps();

    ShrunkCubeMap shrink_cube_map_skybox(vk::ImageLayout layout, uint outputSize);

    void generate_cube_map_irradiance();
    void generate_cube_map_radiance();

    void update_cube_map_texture(sq::ImageStuff source, uint size, uint levels, sq::Texture& texture);

    //--------------------------------------------------------//

    CubeMapView skybox;
    CubeMapView irradiance;
    std::array<CubeMapView, RADIANCE_LEVELS> radiance;

    bool irradianceModified = false;
    bool radianceModified = false;
};

//============================================================================//

} // namespace sts
