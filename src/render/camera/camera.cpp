#include "camera.h"

#include <algorithm>

namespace engine::input {
    constexpr auto kPitchLimit = XM_PIDIV4;

    void Camera::move(float x, float z, float y) {
        auto inX = x;
        auto inY = y;
        auto inZ = -z;

        auto pos = position.load();

        auto move = XMFLOAT3(inX, inY, inZ);

        if (fabs(inX) > 0.1f && fabs(inZ) > 0.1f) {
            auto vec = XMVector3Normalize(XMLoadFloat3(&move));
            move.x = XMVectorGetX(vec);
            move.y = XMVectorGetY(vec);
        }

        float newX = (move.x * -cosf(yaw) - move.z * sinf(yaw)) + pos.x;
        float newY = move.y + pos.y;
        float newZ = (move.x * sinf(yaw) - move.z * cosf(yaw)) + pos.z;

        position = XMFLOAT3(newX, newY, newZ);
    }

    void Camera::rotate(float yawChange, float pitchChange) {
        // clamp pitch to prevent camera flipping
        float newPitch = std::clamp(pitch + pitchChange, -kPitchLimit, kPitchLimit);
        float newYaw = yaw - yawChange;

        float rot = cosf(newPitch);

        direction = XMFLOAT3(rot * sinf(newYaw), sinf(newPitch), rot * cosf(newYaw));
        pitch = newPitch;
        yaw = newYaw;
    }

    void Camera::store(XMFLOAT4X4 *view, XMFLOAT4X4 *projection, float aspect) const {
        auto where = position.load();
        auto look = direction.load();

        auto up = XMVectorSet(0.f, 1.f, 0.f, 1.f);

        auto viewMatrix = XMMatrixLookToRH(XMLoadFloat3(&where), XMLoadFloat3(&look), up);
        auto projectionMatrix = XMMatrixPerspectiveFovRH(fov * (XM_PI / 180.f), aspect, 0.1f, 100000.f);

        XMStoreFloat4x4(view, XMMatrixTranspose(viewMatrix));
        XMStoreFloat4x4(projection, XMMatrixTranspose(projectionMatrix));
    }
}
