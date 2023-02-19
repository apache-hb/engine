#include "game/camera.h"

#include "simcoe/math/consts.h"

using namespace game;

namespace {
    constexpr float3 kUpVector = float3::from(0.f, 0.f, 1.f); // z-up
    constexpr float kPiDiv180 = (kPi<float> / 180.f);
    constexpr float kPitchLimit = kPiDiv4<float>;
}

float4x4 ICamera::getProjection(float aspectRatio) const {
    return float4x4::perspectiveRH(fov * kPiDiv180, aspectRatio, 0.1f, 1000.f).transpose();
}

FirstPerson::FirstPerson(float3 position, float3 direction, float fov)
    : ICamera(position, fov)
    , direction(direction)
    , pitch(0.f)
    , yaw(0.f)
{ 
    rotate(0.f, 0.f);
}

void FirstPerson::move(float3 offset) {
    float x = position.x - (std::cos(yaw) * offset.x) + (std::sin(yaw) * offset.y);
    float y = position.y - (std::sin(yaw) * offset.x) - (std::cos(yaw) * offset.y);

    position = float3::from(x, y, offset.z + position.z);
}

void FirstPerson::rotate(float yawUpdate, float pitchUpdate) {
    float newPitch = std::clamp(pitch + pitchUpdate, -kPitchLimit, kPitchLimit);
    float newYaw = yaw - yawUpdate;

    float newRotation = std::cos(newPitch);

    direction = float3::from(newRotation * std::sin(newYaw), newRotation * -std::cos(newYaw), std::sin(newPitch));
    pitch = newPitch;
    yaw = newYaw;
}

float4x4 FirstPerson::mvp(const float4x4 &model, float aspectRatio) const {
    auto view = float4x4::lookToRH(position, direction, kUpVector).transpose();
    auto projection = getProjection(aspectRatio);
    return projection * view * model;
}

ThirdPerson::ThirdPerson(float3 position, float3 focus, float fov) 
    : ICamera(position, fov)
    , focus(focus)
{ }

void ThirdPerson::move(float3 to) {
    position = to;
}

void ThirdPerson::refocus(float3 to) {
    focus = to;
}

float4x4 ThirdPerson::mvp(const float4x4 &model, float aspectRatio) const {
    auto view = float4x4::lookAtRH(position, focus, kUpVector).transpose();
    auto projection = getProjection(aspectRatio);
    return projection * view * model;
}
