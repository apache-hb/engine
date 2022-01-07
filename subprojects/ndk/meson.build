project('ndk', 'cpp')

here = meson.current_source_dir()
links = [ ]

if buildtype.startswith('debug')
    message('using debug dlss sdk')
    links += [ here/'lib/x86_64/nvsdk_ngx_d.lib' ]
else
    message('using release dlss sdk')
    links += [ here/'lib/x86_64/nvsdk_ngx_s.lib' ]
endif

ndk = declare_dependency(
    include_directories : '.',
    link_args : links
)
