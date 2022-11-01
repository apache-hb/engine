#include "engine/render-new/render.h"
#include "engine/rhi/rhi.h"

using namespace simcoe;
using namespace simcoe::render;

bool ThreadHandle::valid() const {
    return index != SIZE_MAX && parent != nullptr;
}

rhi::CommandList& ThreadHandle::cmd() {
    ASSERT(valid());
    return parent->lists[index];
}

CopyQueue::CopyQueue(rhi::Device& device, const ContextInfo& info) 
    : threads(info.threads) 
    , queue(device.newQueue(rhi::CommandList::Type::eCopy))
    , alloc(info.threads)
{
    lists = new rhi::CommandList[threads];
    allocators = new rhi::Allocator[threads];

    for (size_t i = 0; i < threads; ++i) {
        allocators[i] = device.newAllocator(rhi::CommandList::Type::eCopy);
        lists[i] = device.newCommandList(allocators[i], rhi::CommandList::Type::eCopy);
    }
}

ThreadHandle CopyQueue::getThread() {
    std::lock_guard lock(mutex);

    size_t index = alloc.alloc(eRecording);
    if (index == SIZE_MAX) { return { SIZE_MAX, nullptr }; }
    
    lists[index].beginRecording(allocators[index]);
    return { index, this };
}

void CopyQueue::submit(ThreadHandle thread) {
    std::lock_guard lock(mutex);

    lists[thread.index].endRecording();
    alloc.update(thread.index, eSubmitting);
}

void CopyQueue::wait(UNUSED Context& ctx) {
    std::lock_guard lock(mutex);

    std::vector<ID3D12CommandList*> lists;
    for (size_t i = 0; i < alloc.getSize(); i++) {
        if (alloc.checkBit(i, eSubmitting)) {
            alloc.release(i);
            lists.push_back(this->lists[i].get());
        }
    }

    if (lists.empty()) { return; }
    queue.execute(lists);
}
