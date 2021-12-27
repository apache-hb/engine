# the d3d12 rendering system
design points
* reduce complexity inside context/context.cpp and context/lifetime.cpp
* wrapper functions are good even if only used once or twice
* try and keep calls to `check` only in wrapper functios
