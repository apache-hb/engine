libbase = library('base', 'src/base/io.cpp',
    cpp_args : args,
    include_directories : 'include'
)

base = declare_dependency(
    link_with : libbase,
    include_directories : 'include'
)



libplatform = library('platform', 'src/platform'/ target_machine.system() + '.cpp',
    cpp_args : args,
    include_directories : 'include'
)

platform = declare_dependency(
    link_with : libplatform,
    include_directories : 'include'
)



liblogging = library('logging', 'src/logging/logging.cpp',
    cpp_args : args,
    dependencies : [ fmt, base ],
    include_directories : 'include'
)

logging = declare_dependency(
    link_with : liblogging,
    dependencies : [ fmt, base ],
    include_directories : 'include'
)
