gltf = subproject('fastgltf').get_variable('gltf')

shaders = hlsl.process('../data/shaders/post.hlsl')

src = [
    'main.cpp',
    'game/gdk.cpp'
]

executable('game', src + shaders,
    dependencies : [ core, input, locale, imgui, imnodes, render, gdk, gltf ],
    cpp_args : args,
    win_subsystem : 'console',
    install : true,
    install_dir : get_option('prefix')
)
