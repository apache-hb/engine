project('engine', [ 'cpp', 'c' ],
    license : 'GPLv3',
    version : '0.1.0',
    default_options : [
        'cpp_std=c++20',
        'cpp_rtti=false',
        'cpp_eh=none',
        'b_ndebug=if-release',
        'warning_level=3',
        'werror=true',
        'b_vscrt=static_from_buildtype',
        'default_library=static',
        'b_lundef=true',
        'auto_features=enabled'
    ]
)

d3d12 = subproject('d3d12').get_variable('d3d12')
gdk = subproject('gdk').get_variable('gdk')

args = [ 
    '-D_HAS_EXCEPTIONS=0', # msvc stl requires this
    '/wd4324', # warnings about padding
    '/wd4127' # warnings about if constexpr
]

subdir('simcoe')
subdir('data')

subdir('game')

#gltf = subproject('tinygltf').get_variable('gltf')
#queue = subproject('atomic-queue').get_variable('queue')

# compile libraries that are shared between game and editor
# subdir('common')

# compile game specific libraries
# subdir('game')

# compile editor specific libraries
# subdir('editor')
