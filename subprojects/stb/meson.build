project('stb', 'c')

libstb = library('stb', [ 'stb_image.c', 'stb_image_write.c' ],
    c_args : [ '-D_CRT_SECURE_NO_WARNINGS=1' ]
)

stb = declare_dependency(
    link_with : libstb,
    include_directories : '.'
)
