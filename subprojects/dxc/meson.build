project('dxc', 'cpp')

here = meson.current_source_dir()
links = [ here/'lib/dxcompiler.lib' ]

dxc = declare_dependency(
    include_directories : '.',
    link_args : links
)
