#include "engine/rhi/rhi.h"
#include "objects/common.h"

using namespace simcoe;
using namespace simcoe::rhi;

PipelineState::PipelineState(ID3D12RootSignature *signature, ID3D12PipelineState *pipeline)
    : signature(signature)
    , pipeline(pipeline) 
{ }
