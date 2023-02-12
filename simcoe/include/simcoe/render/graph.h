#pragma once

#include "simcoe/render/context.h"

#include <unordered_map>

namespace simcoe::render {
    struct Graph;
    struct GraphObject;
    struct Pass;
    struct Edge;

    struct GraphObject {
        GraphObject(const char *pzName, Graph& graph);
        GraphObject(const GraphObject& other);

        virtual ~GraphObject() = default;

        const char *getName() const;
        Graph& getGraph();
        Context& getContext();

    private:
        const char *pzName;
        struct Graph& graph;
    };

    struct Edge : GraphObject {
        Edge(const GraphObject& self, Pass *pPass)
            : GraphObject(self)
            , pPass(pPass)
        { }

        
        virtual ID3D12Resource *getResource() = 0;
        
        virtual D3D12_RESOURCE_STATES getState() const = 0;
        virtual D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle() { return { }; }
        virtual D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle() { return { }; }

        Pass *getPass() const { return pPass; }

    private:
        Pass *pPass;
    };

    struct OutEdge : Edge {
        using Edge::Edge;
    };

    struct InEdge : Edge {
        InEdge(const GraphObject& self, Pass *pPass, D3D12_RESOURCE_STATES state)
            : Edge(self, pPass)
            , state(state)
        { }

        ID3D12Resource *getResource() override;
        void updateEdge(OutEdge *pIn);

        D3D12_RESOURCE_STATES getState() const override { return state; }
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle() override { return cpu; }
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle() override { return gpu; }

    private:
        D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
        ID3D12Resource *pResource = nullptr;
        D3D12_CPU_DESCRIPTOR_HANDLE cpu;
        D3D12_GPU_DESCRIPTOR_HANDLE gpu;
    };

    struct RelayEdge : OutEdge {
        RelayEdge(const GraphObject& self, Pass *pPass, InEdge *pOther)
            : OutEdge(self, pPass)
            , pOther(pOther)
        { }

        ID3D12Resource *getResource() override;
        D3D12_RESOURCE_STATES getState() const override;
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle() override;
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle() override;

    private:
        InEdge *pOther;
    };

    struct RenderEdge : OutEdge {
        RenderEdge(const GraphObject& self, Pass *pPass)
            : OutEdge(self, pPass)
        { }

        ID3D12Resource *getResource() override {
            return getContext().getRenderTargetResource();
        }

        D3D12_RESOURCE_STATES getState() const override {
            return D3D12_RESOURCE_STATE_PRESENT;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle() override {
            return getContext().getCpuTargetHandle();
        }

        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle() override {
            return getContext().getGpuTargetHandle();
        }
    };

    struct Pass : GraphObject {
        using InEdgeVec = std::vector<std::unique_ptr<InEdge>>;
        using OutEdgeVec = std::vector<std::unique_ptr<OutEdge>>;

        Pass(const GraphObject& other) : GraphObject(other) {}

        virtual void start() { }
        virtual void stop() { }
        virtual void execute() = 0;

        const InEdgeVec& getInputs() const { return inputs; }
        const OutEdgeVec& getOutputs() const { return outputs; }

    protected:
        template<typename T> requires (std::is_base_of_v<InEdge, T>)
        T *in(const char* pzName, auto&&... args) {
            return static_cast<T*>(addInput(new T(GraphObject(pzName, getGraph()), this, args...)));
        }

        template<typename T> requires (std::is_base_of_v<OutEdge, T>)
        T *out(const char* pzName, auto&&... args) {
            return static_cast<T*>(addOutput(new T(GraphObject(pzName, getGraph()), this, args...)));
        }

    private:
        InEdge *addInput(InEdge *pEdge);
        OutEdge *addOutput(OutEdge *pEdge);
        
        InEdgeVec inputs;
        OutEdgeVec outputs;
    };

    struct Graph {
        using PassMap = std::unordered_map<const char*, std::unique_ptr<Pass>>;
        using EdgeMap = std::unordered_map<InEdge*, OutEdge*>;

        Graph(Context& context);

        void start();
        void stop();

        Context& getContext();
        
        const PassMap& getPasses() const { return passes; }
        const EdgeMap& getEdges() const { return edges; }

    protected:
        void connect(OutEdge *pSource, InEdge *pTarget);

        void execute(Pass *pRoot);

        template<typename T> requires (std::is_base_of_v<Pass, T>)
        T *addPass(const char *pzName, auto&&... args) {
            T *pPass = new T(GraphObject(pzName, *this), args...);
            passes[pzName] = std::unique_ptr<Pass>(pPass);
            return pPass;
        }

    private:
        Context& context;

        PassMap passes;
        EdgeMap edges;
    };
}
