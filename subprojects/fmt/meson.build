project('fmt', 'cpp',
    default_options : [
        'cpp_std=c++20'
    ]
)

args = [
    '-DFMT_EXCEPTIONS=0',
    '-D_HAS_EXCEPTIONS=0'
]

src = [
    'fmt/src/format.cc',
    'fmt/src/os.cc'
]

libfmt = library('fmt', src,
    cpp_args : args,
    include_directories : 'fmt/include'
)

fmt = declare_dependency(
    link_with : libfmt,
    compile_args : args,
    include_directories : 'fmt/include'
)
