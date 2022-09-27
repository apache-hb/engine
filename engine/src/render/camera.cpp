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

void Camera::move(UNUSED float3 offset) {

}

void Camera::rotate(float2 update) {
    float newPitch = std::clamp(pitch + update.y, -kPitchLimit, kPitchLimit);
    float newYaw = yaw - update.x;

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
