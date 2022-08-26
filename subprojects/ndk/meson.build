project('ndk', 'cpp')

here = meson.current_source_dir()
links = [ ]

if buildtype.startswith('debug')
    links += [ here/'lib/x86_64/nvsdk_ngx_d.lib' ]
else
    links += [ here/'lib/x86_64/nvsdk_ngx_s.lib' ]
endif

message('dlss library ', links)

ndk = declare_dependency(
    include_directories : '.',
    link_args : links
)
