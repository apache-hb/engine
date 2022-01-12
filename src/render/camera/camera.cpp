#include "camera.h"

#include <algorithm>

#include "imgui/imgui.h"
#include "logging/debug.h"
#include "math/consts.h"

using namespace engine;

struct CameraDebugObject : debug::DebugObject {
    using Super = debug::DebugObject;
    CameraDebugObject(render::Camera* camera): Super("Camera"), camera(camera) { }

    virtual void info() override {
        const auto pos = camera->getPosition();
        const auto dir = camera->getDirection();
        ImGui::Text("Position: %f, %f, %f", pos.x, pos.y, pos.z);
        ImGui::Text("Direction: %f, %f, %f", dir.x, dir.y, dir.z);
        ImGui::SliderFloat("Fov", &camera->fov, 30.f, 120.f);
        ImGui::SliderFloat("Pitch", &camera->pitch, -90.f, 90.f);
        ImGui::SliderFloat("Yaw", &camera->yaw, -180.f, 180.f);
    }

private:
    render::Camera* camera;
};

static CameraDebugObject* cameraDebugObject = nullptr;

namespace engine::render {
    constexpr auto kPitchLimit = kPiDiv4<float>;

    Camera::Camera(float3 pos, float3 dir, float fov)
        : position(pos)
        , direction(dir)
        , pitch(dir.x)
        , yaw(dir.y)
        , fov(fov)
    {
        cameraDebugObject = new CameraDebugObject(this);
    }

    void Camera::move(float x, float y, float z) {
        auto pos = getPosition();

        auto move = float3::from(x, y, z);

        if (fabs(x) > 0.1f && fabs(y) > 0.1f) {
            auto vec = move.normal();
            move.x = vec.x;
            move.y = vec.y;
        }

        float newX = pos.x - (cos(yaw) * x) + (sin(yaw) * y);
        float newY = pos.y - (sin(yaw) * x) - (cos(yaw) * y);
        float newZ = z + pos.z;

        position = float3::from(newX, newY, newZ);
    }

    void Camera::rotate(float yawChange, float pitchChange) {
        // clamp pitch to prevent camera flipping
        float newPitch = std::clamp(pitch + pitchChange, -kPitchLimit, kPitchLimit);
        float newYaw = yaw - yawChange;

        float rot = cosf(newPitch);

        direction = float3::from(rot * sinf(newYaw), rot * -cosf(newYaw), sinf(newPitch));
        pitch = newPitch;
        yaw = newYaw;
    }

    void Camera::store(float4x4* view, float4x4* projection, float aspect) const {
        auto where = getPosition().vec4(0.f);
        auto look = getDirection().vec4(0.f);

        auto up = float4::from(0.f, 0.f, 1.f, 1.f);
        
        *view = float4x4::lookToRH(where, look, up).transpose();
        *projection = float4x4::perspectiveRH(fov * (kPi<float> / 180.f), aspect, 0.1f, 1000.f).transpose();
    }

    float4x4 Camera::getMvp(const float4x4& model, float aspect) const {
        auto where = getPosition().vec4(0.f);
        auto look = getDirection().vec4(0.f);
        auto up = float4::from(0.f, 0.f, 1.f, 1.f);
        auto view = float4x4::lookToRH(where, look, up).transpose();
        auto projection = float4x4::perspectiveRH(fov * (kPi<float> / 180.f), aspect, 0.1f, 1000.f).transpose();
        return projection * view * model;
    }
}
