project('stb', 'c')

args = [ '-D_CRT_SECURE_NO_WARNINGS=1' ]

libstb_image = library('stb-image', 'stb_image.c',
    c_args : args,
    include_directories : '.'
)

stb_image = declare_dependency(
    link_with : libstb_image,
    include_directories : '.'
)
