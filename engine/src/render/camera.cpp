#include "engine/render/camera.h"

#include "engine/base/macros.h"

#include "engine/math/consts.h"
#include <algorithm>

using namespace engine;
using namespace engine::render;

namespace {
    constexpr auto kUpVector = float4::from(0.f, 0.f, 1.f, 1.f);
    constexpr auto kPiDiv180 = (kPi<float> / 180.f);
    constexpr auto kPitchLimit = kPiDiv4<float>;
}

Camera::Camera(float3 position, float3 direction, float fov)
    : position(position)
    , direction(direction)
    , pitch(0.f)
    , yaw(0.f)
    , fov(fov)
{ }

void Camera::move(float3 offset) {
    float x = position.x - (std::cos(yaw) * offset.x) + (std::sin(yaw) * offset.y);
    float y = position.y - (std::sin(yaw) * offset.x) - (std::cos(yaw) * offset.y);

    position = float3::from(x, y, offset.z + position.z);
}

void Camera::rotate(float yawUpdate, float pitchUpdate) {
    float newPitch = std::clamp(pitch + pitchUpdate, -kPitchLimit, kPitchLimit);
    float newYaw = yaw - yawUpdate;

    float newRotation = std::cos(newPitch);

    direction = float3::from(newRotation * std::sin(newYaw), newRotation * -std::cos(newYaw), std::sin(newPitch));
    pitch = newPitch;
    yaw = newYaw;
}

float4x4 Camera::mvp(const float4x4 &model, float aspectRatio) const {
    auto where = position.vec4(0.f);
    auto look = direction.vec4(0.f);
    auto view = float4x4::lookToRH(where, look, kUpVector).transpose();
    auto projection = float4x4::perspectiveRH(fov * kPiDiv180, aspectRatio, 0.1f, 1000.f).transpose();
    return projection * view * model;
}
