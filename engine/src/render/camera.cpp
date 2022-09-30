#include "engine/render/camera.h"

#include "engine/base/macros.h"

#include "engine/math/consts.h"
#include <algorithm>

#include <DirectXMath.h>

using namespace engine;
using namespace engine::render;

namespace {
    constexpr auto kUpVector = float3::from(0.f, 0.f, 1.f); // z-up
    constexpr auto kPiDiv180 = (kPi<float> / 180.f);
    constexpr auto kPitchLimit = kPiDiv4<float>;
}

Perspective::Perspective(float3 position, float3 direction, float fov)
    : position(position)
    , direction(direction)
    , pitch(0.f)
    , yaw(0.f)
    , fov(fov)
{ }

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

float4x4 Perspective::mvp(const float4x4 &model, float aspectRatio) const {
    auto view = float4x4::lookToRH(position, direction, kUpVector).transpose();
    auto projection = float4x4::perspectiveRH(fov * kPiDiv180, aspectRatio, 0.1f, 1000.f).transpose();
    return projection * view * model;
}

LookAt::LookAt(float3 position, float3 focus, float fov) 
    : position(position)
    , focus(focus)
    , fov(fov)
{ }

float4x4 LookAt::mvp(const float4x4 &model, float aspectRatio) const {
    auto view = float4x4::lookAtRH(position, focus, kUpVector).transpose();
    auto projection = float4x4::perspectiveRH(fov * kPiDiv180, aspectRatio, 0.1f, 1000.f).transpose();
    return projection * view * model;
}

void LookAt::setPosition(float3 update) {
    position = update;
}

void LookAt::setFocus(float3 update) {
    focus = update;
}

CameraBuffer::CameraBuffer(Camera &camera)
    : camera(camera)
{ }

void CameraBuffer::attach(rhi::Device& device, rhi::CpuHandle handle) {
    buffer = device.newBuffer(sizeof(CameraData), rhi::DescriptorSet::Visibility::eHostVisible, rhi::Buffer::State::eUpload);
    device.createConstBufferView(buffer, sizeof(CameraData), handle);

    ptr = buffer.map();
}

void CameraBuffer::detach() {

}

void CameraBuffer::update(float aspectRatio) {
    data.mvp = camera.mvp(float4x4::identity(), aspectRatio);
    memcpy(ptr, &data, sizeof(CameraData));
}
