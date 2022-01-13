project('imgui-widgets', 'cpp')

imgui = subproject('imgui').get_variable('imgui')

libwidgets = library('widgets', [ 'imgui-file-dialog.cpp' ],
    cpp_args : [ '-D_CRT_SECURE_NO_WARNINGS=1' ],
    dependencies : imgui
)

widgets = declare_dependency(
    link_with : libwidgets,
    include_directories : '.',
    dependencies : imgui
)
