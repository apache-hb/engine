project('gdk', 'cpp')

# TODO: is vendoring this ourselves the best way to do this?

here = meson.current_source_dir()

lib = here/'GameKit/Lib/amd64/'

gdk = declare_dependency(
    include_directories : here/'GameKit/Include',
    link_args : [ lib/'GameInput.lib', lib/'xgameruntime.lib' ]
)
