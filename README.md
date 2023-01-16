# simcoe engine
functional if not pretty

# layout
* common - shared code that both the game and editor use
    * include/engine
        * base - core stuff that everything else depends on
        * ui - TODO: put this somewhere better
        * memory - memory allocators
        * container - general use containers
        * math - linear algebra and utilities
        * window - windowing
        * locale - translation
        * input - keyboard and gamepad input handling
        * rhi - d3d12 abstraction layer
        * render - core renderer

    * vendor - third party libraries
        * imgui - dear imgui used for debug gui

* editor - editor specific code
    * include/editor/shaders - shader compilation
    * include/editor/widgets - dear imgui widgets

* game - gameplay specific code

* resources - data
    * agility - d3d12 agility sdk
    * locale - translations
    * shaders - d3d12 shaders
