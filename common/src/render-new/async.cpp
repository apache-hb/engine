#include "engine/render-new/async.h"
#include "engine/render-new/queue.h"
#include "engine/render-new/render.h"

using namespace simcoe;
using namespace simcoe::render;

struct AsyncBufferCopy final : IAsync {
    AsyncBufferCopy(rhi::Buffer upload, rhi::Buffer result, size_t size)
        : upload(std::move(upload))
        , result(std::move(result))
        , size(size)
    { }

    void apply(rhi::CommandList& list) override {
        list.copyBuffer(result, upload, size);
    }

    rhi::Buffer getBuffer() override {
        return std::move(result);
    }

private:
    rhi::Buffer upload;
    rhi::Buffer result;
    size_t size;
};

struct AsyncTextureCopy final : IAsync {
    AsyncTextureCopy(rhi::Buffer upload, rhi::Buffer result)
        : upload(std::move(upload))
        , result(std::move(result))
    { }

    void apply(rhi::CommandList& list) override {
        list.copyTexture(result, upload);
    }

    rhi::Buffer getBuffer() override {
        return std::move(result);
    }

private:
    rhi::Buffer upload;
    rhi::Buffer result;
};

struct AsyncBatch final : IAsync {
    AsyncBatch(std::vector<AsyncAction>&& actions)
        : actions(std::move(actions))
    { }

    void apply(rhi::CommandList& list) override {
        for (auto& action : actions) {
            action->apply(list);
        }
    }

    rhi::Buffer getBuffer() override {
        ASSERTF(false, "AsyncBatch::getBuffer() is not supported");
    }

private:
    std::vector<AsyncAction> actions;
};

AsyncAction CopyQueue::beginDataUpload(const void* data, size_t size) {
    auto& device = ctx.getDevice();

    rhi::Buffer buffer = device.newBuffer(size, rhi::DescriptorSet::Visibility::eHostVisible, rhi::Buffer::State::eUpload);
    rhi::Buffer upload = device.newBuffer(size, rhi::DescriptorSet::Visibility::eDeviceOnly, rhi::Buffer::State::eCommon);
    buffer.write(data, size);

    buffer.rename("upload buffer");
    upload.rename("target buffer");

    return std::make_shared<AsyncBufferCopy>(std::move(buffer), std::move(upload), size);
}

AsyncAction CopyQueue::beginTextureUpload(const void* data, UNUSED size_t size, rhi::TextureSize resolution) {
    auto& device = ctx.getDevice();

    // TODO: busted
    auto uploadDesc = rhi::newTextureDesc(resolution, rhi::Buffer::State::eCommon);
    auto targetDesc = rhi::newTextureDesc(resolution, rhi::Buffer::State::eCopyDst);

    rhi::Buffer buffer = device.newTexture(uploadDesc, rhi::DescriptorSet::Visibility::eHostVisible);
    rhi::Buffer upload = device.newTexture(targetDesc, rhi::DescriptorSet::Visibility::eDeviceOnly);
    buffer.writeTexture(data, resolution);

    buffer.rename("upload texture");
    upload.rename("target texture");

    return std::make_shared<AsyncTextureCopy>(std::move(buffer), std::move(upload));
}   

AsyncAction CopyQueue::beginBatchUpload(std::vector<AsyncAction> &&actions) {
    return std::make_shared<AsyncBatch>(std::move(actions));
}
