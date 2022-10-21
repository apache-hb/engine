# TODO
* reorg input manager
    * seperate thread for polling events
    * push to central queue
    * multiple listeners on single queue
* get tinygltf working
* convert to our own format
* object culling
    * mipmaps
    * lod system
    * asset streaming
* basic scene system

D3D12 ERROR: CGraphicsCommandList::SetGraphicsRootDescriptorTable: Specified GPU Descriptor Handle (ptr = 0x68acf140000a0 at 7 offsetInDescriptorsFromDescriptorHeapStart), for Root Signature (0x000001A58DA80BC0:'Unnamed ID3D12RootSignature Object')'s Descriptor Table (at Parameter Index [4])'s Descriptor Range (at Range Index [0] of type D3D12_DESCRIPTOR_RANGE_TYPE_SRV) has not been initialized. All descriptors of descriptor ranges declared STATIC (not-DESCRIPTORS_VOLATILE) in a root signature must be initialized prior to being set on the command list. Only bothering to report this error once for an unbounded size descriptor range, so there may be other errors since an unbounded range declared as STATIC means the rest of the heap is STATIC.  [ EXECUTION ERROR #646: INVALID_DESCRIPTOR_HANDLE]

D3D12 ERROR: ID3D12CommandList::DrawIndexedInstanced: Static Descriptor SRV resource dimensions (D3D12_SRV_DIMENSION_UNKNOWN) differs from that expected by shader (D3D12_SRV_DIMENSION_TEXTURE2D) at register 2 space 0. Static Descriptor (ptr = 0x00068acf140000e0) is from Descriptor Table Handle (ptr = 0x00068acf140000a0) at 2 descriptor offset. Also Root Signature (0x000001A58DA80BC0:'Unnamed ID3D12RootSignature Object')'s Descriptor Table (at Parameter Index [4])'s Descriptor Range (at Range Index [0])'s Descriptor (at descriptorIndex in range [2]). Only reporting this error once for an unbounded descriptor range, so there may be other errors since an unbounded range declared as STATIC means the rest of the heap is STATIC.  [ EXECUTION ERROR #1023: COMMAND_LIST_STATIC_DESCRIPTOR_RESOURCE_DIMENSION_MISMATCH]

D3D12 ERROR: ID3D12CommandList::DrawIndexedInstanced: Static Descriptor SRV resource dimensions (D3D12_SRV_DIMENSION_UNKNOWN) differs from that expected by shader (D3D12_SRV_DIMENSION_TEXTURE2D) at register 2 space 0. Static Descriptor (ptr = 0x00068acf140000e0) is from Descriptor Table Handle (ptr = 0x00068acf140000a0) at 2 descriptor offset. Also Root Signature (0x000001A58DA80BC0:'Unnamed ID3D12RootSignature Object')'s Descriptor Table (at Parameter Index [4])'s Descriptor Range (at Range Index [0])'s Descriptor (at descriptorIndex in range [2]). Only reporting this error once for an unbounded descriptor range, so there may be other errors since an unbounded range declared as STATIC means the rest of the heap is STATIC.  [ EXECUTION ERROR #1023: COMMAND_LIST_STATIC_DESCRIPTOR_RESOURCE_DIMENSION_MISMATCH]
D3