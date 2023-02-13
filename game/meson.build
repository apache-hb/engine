shaders = hlsl.process('../data/shaders/post.hlsl')

executable('game', [ 'main.cpp' ] + shaders,
    dependencies : [ core, input, locale, imgui, imnodes, render, gdk, audio ],
    cpp_args : args,
    win_subsystem : 'console',
    install : true,
    install_dir : get_option('prefix')
)
