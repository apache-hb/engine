# simcoe engine
functional if not pretty

# layout
* simcoe - common code
    * include/imgui - dear imgui
    * include/imnodes - dear imgui node editor widget
    * include/simcoe - engine code
        * async - c++20 couroutine helper library
        * audio - xaudio utils
        * core - core platform abstractions
        * input - user input handling
        * locale - internationalization
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
