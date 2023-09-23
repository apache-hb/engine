# simcoe engine
functional if not pretty

# layout
* simcoe - common code
    * include/imgui - dear imgui
    * include/imnodes - dear imgui node editor widget
    * include/simcoe - engine code
        * async - c++20 couroutine helper library
        * core - core platform abstractions
        * input - user input handling
        * math - linear algebra library
        * memory - allocators and other memory management utils
        * render - d3d12 render impl

* game - game + editor
    * game/gdk - microsoft gdk abstraction

* data - resource and config files
    * agility - d3d12 agility sdk
    * shaders - hlsl shaders
    * tools - tools required as parts of the build pipeline

## todos
* stop conflating resources and edges
* make our own asset pipeline, meson is lacking sadly


## render graph
* graph contains directed graph of passes connected via edges
* passes are possibly statefull objects that have a list of input and output edges
* edges are stateless and describe required states of resources that must be met to begin a pass
* resources are statefull objects that track api resources and their relevant state
* passes are joined along edges to form acyclic graph