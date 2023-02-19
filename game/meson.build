gltf = subproject('fastgltf').get_variable('gltf')

shaders = [
    hlsl.process('../data/shaders/post.hlsl'),
    hlsl.process('../data/shaders/scene.hlsl')
]

src = [
    'main.cpp',
    'src/game.cpp',
    'src/gdk.cpp',
    'src/camera.cpp',
    'src/window.cpp',
    'src/render.cpp',
    'src/render/imgui.cpp',
    'src/render/scene.cpp',
    'src/render/blit.cpp'
]

executable('game', src + shaders,
    dependencies : [ core, input, locale, imgui, imnodes, render, gdk, gltf ],
    cpp_args : args,
    include_directories : 'include',
    win_subsystem : 'console',
    install : true,
    install_dir : get_option('prefix')
)
