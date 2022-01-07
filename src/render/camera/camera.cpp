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
        const auto fov = camera->getFov();
        const auto pitch = camera->getPitch();
        const auto yaw = camera->getYaw();
        ImGui::Text("Position: %f, %f, %f", pos.x, pos.y, pos.z);
        ImGui::Text("Direction: %f, %f, %f", dir.x, dir.y, dir.z);
        ImGui::Text("Fov: %f", fov);
        ImGui::Text("Pitch: %f", pitch);
        ImGui::Text("Yaw: %f", yaw);
    }

private:
    render::Camera* camera;
};

static CameraDebugObject* cameraDebugObject = nullptr;

namespace engine::render {
    constexpr auto kPitchLimit = XM_PIDIV4;

    Camera::Camera(XMFLOAT3 pos, XMFLOAT3 dir, float fov)
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

        auto move = XMFLOAT3(x, y, z);

        if (fabs(x) > 0.1f && fabs(y) > 0.1f) {
            auto vec = XMVector3Normalize(XMLoadFloat3(&move));
            move.x = XMVectorGetX(vec);
            move.y = XMVectorGetY(vec);
        }

        float newX = pos.x - (cos(yaw) * x) + (sin(yaw) * y);
        float newY = pos.y - (sin(yaw) * x) - (cos(yaw) * y);
        float newZ = z + pos.z;

        position = XMFLOAT3(newX, newY, newZ);
    }

    void Camera::rotate(float yawChange, float pitchChange) {
        // clamp pitch to prevent camera flipping
        float newPitch = std::clamp(pitch + pitchChange, -kPitchLimit, kPitchLimit);
        float newYaw = yaw - yawChange;

        float rot = cosf(newPitch);

        direction = XMFLOAT3(rot * sinf(newYaw), rot * -cosf(newYaw), sinf(newPitch));
        pitch = newPitch;
        yaw = newYaw;
    }

    void Camera::store(XMFLOAT4X4* view, XMFLOAT4X4* projection, float aspect) const {
        auto where = getPosition();
        auto look = getDirection();

        auto up = XMVectorSet(0.f, 0.f, 1.f, 1.f);

        auto viewMatrix = XMMatrixLookToRH(XMLoadFloat3(&where), XMLoadFloat3(&look), up);
        auto projectionMatrix = XMMatrixPerspectiveFovRH(fov * (XM_PI / 180.f), aspect, 0.1f, 100000.f);

        XMStoreFloat4x4(view, XMMatrixTranspose(viewMatrix));
        XMStoreFloat4x4(projection, XMMatrixTranspose(projectionMatrix));
    }
}
