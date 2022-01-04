#include "scene.h"

#include "gltf/tinygltf.h"

namespace engine::render {
    namespace gltf = tinygltf;
    
    struct Attribute {
        DXGI_FORMAT format;
        D3D12_VERTEX_BUFFER_VIEW view;
    };

    struct Primitive {

    };

    struct GltfScene : Scene3D {
        using Super = Scene3D;

        GltfScene(Context* context, std::string_view path);
        virtual ~GltfScene() = default;

        virtual void create() override;
        virtual void destroy() override;

        virtual ID3D12CommandList* populate() override; 

    private:
        void createBuffers();
        void destroyBuffers();

        void createImages();
        void destroyImages();
        
        void drawNode(size_t index);

        virtual UINT requiredCbvSrvSize() const override;
        gltf::Model model;

        std::vector<Resource> buffers;
        std::vector<Resource> images;

        std::vector<Attribute> attributes;

        DescriptorHeap cbvSrvHeap;
    };
}
