#pragma once

#include "simcoe/render/context.h"

#include <unordered_map>

struct GraphBuilder;

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
        virtual void setResource(ID3D12Resource* pIn);
        
        virtual D3D12_RESOURCE_STATES getState() const = 0;

        Pass *getPass() const { return pPass; }

    private:
        Pass *pPass;
    };

    struct InEdge : Edge {
        InEdge(const GraphObject& self, Pass *pPass, D3D12_RESOURCE_STATES state)
            : Edge(self, pPass)
            , state(state)
        { }

        ID3D12Resource *getResource() override;
        void setResource(ID3D12Resource *pIn) override;

        D3D12_RESOURCE_STATES getState() const override { return state; }

    private:
        D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
        ID3D12Resource *pResource = nullptr;
    };

    struct OutEdge : Edge {
        using Edge::Edge;
    };

    struct Pass : GraphObject {
        friend Graph;
        friend GraphBuilder;

        Pass(const GraphObject& other) : GraphObject(other) {}

        virtual void start() = 0;
        virtual void stop() = 0;
        virtual void execute() = 0;

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
        
        std::vector<std::unique_ptr<InEdge>> inputs;
        std::vector<std::unique_ptr<OutEdge>> outputs;
    };

    struct RelayEdge : OutEdge {
        RelayEdge(const GraphObject& self, Pass *pPass, InEdge *pOther)
            : OutEdge(self, pPass)
            , pOther(pOther)
        { }

        ID3D12Resource *getResource() override;
        D3D12_RESOURCE_STATES getState() const override;

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
    };

    struct Graph {
        friend GraphBuilder;
        Graph(Context& context);

        void start();
        void stop();

        Context& getContext();

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

        std::unordered_map<const char*, std::unique_ptr<Pass>> passes;

        // dst <- src
        std::unordered_map<InEdge*, OutEdge*> edges;
    };
}
