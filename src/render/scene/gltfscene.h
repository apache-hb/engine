#include "scene.h"

#include "gltf/tinygltf.h"

namespace engine::render {
    namespace gltf = tinygltf;
    
    struct Attribute {
        DXGI_FORMAT format;
        D3D12_VERTEX_BUFFER_VIEW view;
    };

    struct Primitive {
        std::vector<Attribute> attributes;
        UINT vertexCount;
        D3D12_PRIMITIVE_TOPOLOGY topology;
        D3D12_INDEX_BUFFER_VIEW indexView;
        UINT indexCount;
        UINT textureIndex;
    };

    struct Mesh {
        std::vector<Primitive> primitives;
    };

    struct GltfScene : Scene3D {
        using Super = Scene3D;

        GltfScene(Context* context, Camera* camera, std::string_view path);
        virtual ~GltfScene() = default;

        virtual void create() override;
        virtual void destroy() override;

        virtual ID3D12CommandList* populate() override; 

    private:
        void createBuffers();
        void destroyBuffers();

        void createImages();
        void destroyImages();
        
        void createMeshes();
        void destroyMeshes();

        void drawNode(size_t index);

        virtual UINT requiredCbvSrvSize() const override;
        gltf::Model model;

        std::vector<Resource> buffers;
        std::vector<Resource> images;
        std::vector<Mesh> meshes;

        DescriptorHeap cbvSrvHeap;
    };
}
