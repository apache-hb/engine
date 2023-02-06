#pragma once

#include "simcoe/render/context.h"

#include <unordered_map>

struct GraphBuilder;

namespace simcoe::render {
    struct Graph;
    struct GraphObject;
    struct Pass;
    struct Resource;
    struct Wire;

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

    struct Pass : GraphObject {
        friend Graph;
        friend GraphBuilder;

        Pass(const GraphObject& other) : GraphObject(other) {}

        virtual void start() = 0;
        virtual void stop() = 0;
        virtual void execute() = 0;

    protected:
        template<typename T> requires (std::is_base_of_v<Wire, T>)
        Wire *in(const char* pzName, auto&&... args) {
            return addInput(new T(GraphObject(pzName, getGraph()), this, args...));
        }

        template<typename T> requires (std::is_base_of_v<Wire, T>)
        Wire *out(const char* pzName, auto&&... args) {
            return addOutput(new T(GraphObject(pzName, getGraph()), this, args...));
        }

    private:
        Wire *addInput(Wire *pWire);
        Wire *addOutput(Wire *pWire);
        
        std::vector<std::unique_ptr<Wire>> inputs;
        std::vector<std::unique_ptr<Wire>> outputs;
    };

    struct Resource : GraphObject {
        friend GraphBuilder;

        virtual void create() = 0;
        virtual void destroy() = 0;

        Resource(const GraphObject& other, ID3D12Resource *pResource) 
            : GraphObject(other) 
            , pResource(pResource)
        { }

    private:
        ID3D12Resource *pResource;
        D3D12_RESOURCE_STATES current;
    };

    struct Wire : GraphObject {
        friend GraphBuilder;

        Wire(const GraphObject& self, Pass *pPass, D3D12_RESOURCE_STATES state)
            : GraphObject(self)
            , pPass(pPass)
            , state(state)
        { }

    private:
        Pass *pPass;
        D3D12_RESOURCE_STATES state;
    };

    struct Graph {
        friend GraphBuilder;
        Graph(Context& context);

        void start();
        void stop();

        Context& getContext();

    protected:
        void connect(Wire *pSource, Wire *pTarget);
        void connect(Wire *pWire, Resource *pResource);

        void execute(Pass *pRoot);

        template<typename T> requires (std::is_base_of_v<Pass, T>)
        T *addPass(const char *pzName, auto&&... args) {
            T *pPass = new T(GraphObject(pzName, *this), args...);
            passes[pzName] = std::unique_ptr<Pass>(pPass);
            return pPass;
        }

        template<typename T> requires (std::is_base_of_v<Resource, T>)
        T *addResource(const char *pzName, auto&&... args) {
            T *pResource = new T(GraphObject(pzName, *this), args...);
            resources[pzName] = std::unique_ptr<Resource>(pResource);
            return pResource;
        }

    private:
        Context& context;

        std::unordered_map<const char*, std::unique_ptr<Pass>> passes;
        std::unordered_map<const char *, std::unique_ptr<Resource>> resources;

        // dst <- src
        std::unordered_map<Wire*, Wire*> wires;
        std::unordered_map<Wire*, Resource*> wireResources;
    };
}
