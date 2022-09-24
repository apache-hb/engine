#include "objects/pipeline.h"

using namespace engine;

DxPipelineState::DxPipelineState(ID3D12RootSignature *signature, ID3D12PipelineState *pipeline)
    : signature(signature)
    , pipeline(pipeline) 
{ }

ID3D12RootSignature *DxPipelineState::getSignature() { 
    return signature.get(); 
}

ID3D12PipelineState *DxPipelineState::getPipelineState() { 
    return pipeline.get(); 
}
