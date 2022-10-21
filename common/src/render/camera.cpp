#include "engine/render/camera.h"

#include "engine/base/macros.h"

#include "engine/math/consts.h"
#include <algorithm>

#include <DirectXMath.h>

using namespace simcoe;
using namespace simcoe::render;

namespace {
    constexpr auto kUpVector = float3::from(0.f, 0.f, 1.f); // z-up
    constexpr auto kPiDiv180 = (kPi<float> / 180.f);
    constexpr auto kPitchLimit = kPiDiv4<float>;
}

float4x4 Camera::getProjection(float aspectRatio) const {
    return float4x4::perspectiveRH(fov * kPiDiv180, aspectRatio, 0.1f, 1000.f).transpose();
}

Perspective::Perspective(float3 position, float3 direction, float fov)
    : Camera(position, fov)
    , direction(direction)
    , pitch(0.f)
    , yaw(0.f)
{ 
    rotate(0.f, 0.f);
}

void Perspective::move(float3 offset) {
    float x = position.x - (std::cos(yaw) * offset.x) + (std::sin(yaw) * offset.y);
    float y = position.y - (std::sin(yaw) * offset.x) - (std::cos(yaw) * offset.y);

    position = float3::from(x, y, offset.z + position.z);
}

void Perspective::rotate(float yawUpdate, float pitchUpdate) {
    float newPitch = std::clamp(pitch + pitchUpdate, -kPitchLimit, kPitchLimit);
    float newYaw = yaw - yawUpdate;

    float newRotation = std::cos(newPitch);

    direction = float3::from(newRotation * std::sin(newYaw), newRotation * -std::cos(newYaw), std::sin(newPitch));
    pitch = newPitch;
    yaw = newYaw;
}

float4x4 Perspective::getView() const {
    return float4x4::lookToRH(position, direction, kUpVector).transpose();
}

float4x4 Perspective::mvp(const float4x4 &model, float aspectRatio) const {
    auto view = getView();
    auto projection = getProjection(aspectRatio);
    return projection * view * model;
}

LookAt::LookAt(float3 position, float3 focus, float fov) 
    : Camera(position, fov)
    , focus(focus)
{ }

float4x4 LookAt::getView() const {
    return float4x4::lookAtRH(position, focus, kUpVector).transpose();
}

float4x4 LookAt::mvp(const float4x4 &model, float aspectRatio) const {
    auto view = getView();
    auto projection = getProjection(aspectRatio);
    return projection * view * model;
}

void LookAt::setPosition(float3 update) {
    position = update;
}

void LookAt::setFocus(float3 update) {
    focus = update;
}
