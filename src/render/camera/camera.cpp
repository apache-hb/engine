#include "camera.h"

#include <algorithm>

#include "imgui/imgui.h"

#include "math/consts.h"

namespace engine::render {
    namespace m = engine::math;

    constexpr auto kPitchLimit = m::kPiDiv4<float>;

    void Camera::move(float x, float z, float y) {
        auto inX = x;
        auto inY = y;
        auto inZ = -z;

        auto pos = position.load();

        auto move = m::float3::from(inX, inY, inZ);

        if (fabs(inX) > 0.1f && fabs(inZ) > 0.1f) {
            const auto [moveX, moveY, _] = move.normal();
            move.x = moveX;
            move.y = moveY;
        }

        float newX = (move.x * -cosf(yaw) - move.z * sinf(yaw)) + pos.x;
        float newY = move.y + pos.y;
        float newZ = (move.x * sinf(yaw) - move.z * cosf(yaw)) + pos.z;

        position = float3::from(newX, newY, newZ);
    }

    void Camera::rotate(float yawChange, float pitchChange) {
        // clamp pitch to prevent camera flipping
        float newPitch = std::clamp(pitch + pitchChange, -kPitchLimit, kPitchLimit);
        float newYaw = yaw - yawChange;

        float rot = cosf(newPitch);

        direction = float3::from(rot * sinf(newYaw), sinf(newPitch), rot * cosf(newYaw));
        pitch = newPitch;
        yaw = newYaw;
    }

    void Camera::store(float4x4* view, float4x4* projection, float aspect) const {
        auto where = position.load();
        auto look = direction.load();

        auto up = float4::from(0.f, 1.f, 0.f, 1.f);

        *view = float4x4::lookToRH(where.vec4(), look.vec4(), up).transpose();
        *projection = float4x4::perspectiveRH(fov * (m::kPi<float> / 180.f), aspect, 0.1f, 1000.f).transpose();
    }

    void Camera::imgui() {
        const auto pos = position.load();
        const auto dir = direction.load();
        ImGui::Begin("Camera info");
            ImGui::Text("Position: %f, %f, %f", pos.x, pos.y, pos.z);
            ImGui::Text("Direction: %f, %f, %f", dir.x, dir.y, dir.z);
            ImGui::Text("Fov: %f", fov);
        ImGui::End();
    }
}
