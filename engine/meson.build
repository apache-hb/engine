libplatform = library('platform', 'src/platform'/ target_machine.system() + '.cpp',
    cpp_args : args,
    include_directories : 'include'
)

platform = declare_dependency(
    link_with : libplatform
)



liblogging = library('logging', 'src/logging/logging.cpp',
    cpp_args : args,
    dependencies : [ fmt ],
    include_directories : 'include'
)

logging = declare_dependency(
    link_with : liblogging,
    include_directories : 'include'
)
