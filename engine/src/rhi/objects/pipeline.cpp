#include "engine/rhi/rhi.h"
#include "objects/common.h"

using namespace engine;
using namespace engine::rhi;

PipelineState::PipelineState(ID3D12RootSignature *signature, ID3D12PipelineState *pipeline)
    : signature(signature)
    , pipeline(pipeline) 
{ }
