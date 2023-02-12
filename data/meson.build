python3 = import('python').find_installation()

script = meson.current_source_dir()/'tools/hlsl.py'

hlsl = generator(python3, 
    output : [ '@BASENAME@.ps', '@BASENAME@.vs' ],
    arguments : [ script, '@INPUT@', '@BUILD_DIR@/@BASENAME@', 'vs,ps', '6_0' ]
)
