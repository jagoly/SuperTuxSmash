//#include <sqee/maths/Functions.hpp>
//#include <sqee/gl/UniformBuffer.hpp>

//#include "render/Camera.hpp"

//namespace maths = sq::maths;
//using namespace sts;

////============================================================================//

////Camera::Camera() = default;

//void Camera::update_view_bounds(Vec2F min, Vec2F max)
//{
//    Bounds averageBounds = { min, max };

//    for (uint i = 0u; i < 15u; ++i)
//    {
//        averageBounds.min += mViewHistory[i].min;
//        averageBounds.max += mViewHistory[i].max;
//        mViewHistory[i] = mViewHistory[i + 1];
//    }

//    mViewHistory.back() = { min, max };

//    mPreviousView = mCurrentView;
//    mCurrentView.min = averageBounds.min / 16.f;
//    mCurrentView.max = averageBounds.max / 16.f;
//}

////============================================================================//

//void Camera::update_camera(float blend, float aspect, sq::UniformBuffer& ubo)
//{
//    const Vec3F cameraDirection = Vec3F(0.f, 0.f, +1.f);

//    mProjMatrix = maths::perspective_LH(1.f, aspect, 0.2f, 200.f);

//    const Vec2F minView = maths::mix(mPreviousView.min, mCurrentView.min, blend);
//    const Vec2F maxView = maths::mix(mPreviousView.max, mCurrentView.max, blend);

//    const float cameraDistance = [&]() {
//        float distanceX = (maxView.x - minView.x + 4.f) * 0.5f / std::tan(0.5f) / aspect;
//        float distanceY = (maxView.y - minView.y + 4.f) * 0.5f / std::tan(0.5f);
//        return maths::max(distanceX, distanceY); }();

//    Vec3F cameraTarget = Vec3F((minView + maxView) * 0.5f, 0.f);
//    Vec3F cameraPosition = cameraTarget - cameraDirection * cameraDistance;

//    mViewMatrix = maths::look_at_LH(cameraPosition, cameraTarget, Vec3F(0.f, 1.f, 0.f));

//    const float screenLeft = [&]() {
//        Vec4F worldPos = Vec4F(mOuterBounds.min.x, cameraTarget.y, 0.f, 1.f);
//        Vec4F screenPos = mProjMatrix * (mViewMatrix * worldPos);
//        return (screenPos.x / screenPos.w + 1.f) * screenPos.w; }();

//    const float screenRight = [&]() {
//        Vec4F worldPos = Vec4F(mOuterBounds.max.x, cameraTarget.y, 0.f, 1.f);
//        Vec4F screenPos = mProjMatrix * (mViewMatrix * worldPos);
//        return (screenPos.x / screenPos.w - 1.f) * screenPos.w; }();

//    if (screenLeft > 0.f && screenRight < 0.f) {}
//    else if (screenLeft > 0.f) { cameraTarget.x += screenLeft; }
//    else if (screenRight < 0.f) { cameraTarget.x += screenRight; }

//    cameraPosition = cameraTarget - cameraDirection * cameraDistance;

//    mViewMatrix = maths::look_at_LH(cameraPosition, cameraTarget, Vec3F(0.f, 1.f, 0.f));

//    ubo.update_complete ( mViewMatrix, mProjMatrix,
//                          maths::inverse(mViewMatrix), maths::inverse(mProjMatrix),
//                          cameraPosition, 0, cameraDirection, 0 );
//}

////============================================================================//

//void Renderer::render_particles(float accum, float blend)
//{
//    mParticleRenderer->swap_sets();

//    for (const auto& object : mRenderObjects)
//        for (const auto& set : object->get_particle_sets())
//            mParticleRenderer->integrate_set(set);

//    mParticleRenderer->render_particles();
//}

////============================================================================//

//void Renderer::finish_rendering()
//{
//    //-- Resolve the Multi Sample Texture --------------------//

//    fbos.Main.blit(fbos.Resolve, options.Window_Size, gl::COLOR_BUFFER_BIT);

//    //-- Composite to the Default Framebuffer ----------------//

//    context.bind_FrameBuffer_default();

//    context.set_state(Context::Blend_Mode::Disable);
//    context.set_state(Context::Cull_Face::Disable);
//    context.set_state(Context::Depth_Test::Disable);

//    context.bind_Texture(textures.Resolve, 0u);
//    context.bind_Program(shaders.Composite);

//    sq::draw_screen_quad();
//}

////============================================================================//

//void Renderer::render_blobs(const Vector<HitBlob*>& blobs)
//{
//    context.bind_FrameBuffer(fbos.Main);

//    context.set_state(Context::Blend_Mode::Alpha);
//    context.set_state(Context::Cull_Face::Disable);
//    context.set_state(Context::Depth_Test::Disable);

//    context.bind_Program(shaders.Debug_HitBlob);

//    context.bind_VertexArray(meshes.Sphere.get_vao());

//    const Mat4F projViewMat = camera.projMatrix * camera.viewMatrix;

//    for (const HitBlob* blob : blobs)
//    {
//        const maths::Sphere& s = blob->sphere;

//        const Mat4F matrix = maths::transform(s.origin, Vec3F(s.radius));

//        shaders.Debug_HitBlob.update(0, projViewMat * matrix);
//        shaders.Debug_HitBlob.update(1, blob->get_debug_colour());

//        meshes.Sphere.draw_complete();
//    }
//}

////============================================================================//

//void Renderer::render_blobs(const Vector<HurtBlob*>& blobs)
//{
//    context.bind_FrameBuffer(fbos.Main);

//    context.set_state(Context::Blend_Mode::Alpha);
//    context.set_state(Context::Cull_Face::Disable);
//    context.set_state(Context::Depth_Test::Disable);

//    context.bind_Program(shaders.Debug_HitBlob);

//    context.bind_VertexArray(meshes.Capsule.get_vao());

//    const Mat4F projViewMat = camera.projMatrix * camera.viewMatrix;

//    for (const HurtBlob* blob : blobs)
//    {
//        const maths::Capsule& c = blob->capsule;

//        const Mat3F rotation = maths::basis_from_y(maths::normalize(c.originB - c.originA));

//        const Vec3F originM = (c.originA + c.originB) * 0.5f;
//        const float lengthM = maths::distance(c.originA, c.originB);

//        const Mat4F matrixA = maths::transform(c.originA, rotation, c.radius);
//        const Mat4F matrixB = maths::transform(c.originB, rotation, c.radius);

//        const Mat4F matrixM = maths::transform(originM, rotation, Vec3F(c.radius, lengthM, c.radius));

//        shaders.Debug_HitBlob.update(1, blob->get_debug_colour());

//        shaders.Debug_HitBlob.update(0, projViewMat * matrixA);
//        meshes.Capsule.draw_partial(0u);

//        shaders.Debug_HitBlob.update(0, projViewMat * matrixB);
//        meshes.Capsule.draw_partial(1u);

//        shaders.Debug_HitBlob.update(0, projViewMat * matrixM);
//        meshes.Capsule.draw_partial(2u);
//    }
//}
