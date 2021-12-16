#include "camera.h"

namespace engine::render {
    Camera::Camera(XMFLOAT3 pos, XMFLOAT3 look) 
        : position(pos)
        , target(target)
        , up({ 0, 1, 0 }) /// +y is up
    { }

    void Camera::write(Camera::ConstBuffer *buffer, float fov, float aspect) const {
        Camera::ConstBuffer self;
        
        auto world = XMMATRIX(g_XMIdentityR0, g_XMIdentityR1, g_XMIdentityR2, g_XMIdentityR3);
        auto view = XMMatrixLookAtLH(XMLoadFloat3(&position), XMLoadFloat3(&target), XMLoadFloat3(&up));
        auto proj = XMMatrixPerspectiveFovLH(fov, aspect, 0.1f, 1000.f);
        
        XMStoreFloat4x4(&self.world, XMMatrixTranspose(world));
        XMStoreFloat4x4(&self.view, XMMatrixTranspose(world * view));
        XMStoreFloat4x4(&self.projection, XMMatrixTranspose(world * view * proj));

        memcpy(buffer, &self, sizeof(Camera::ConstBuffer));
    }
}
