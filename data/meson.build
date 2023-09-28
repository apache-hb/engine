python3 = import('python').find_installation()

here = meson.current_source_dir()
script = here/'tools/hlsl.py'

# TODO: cant install until https://github.com/mesonbuild/meson/issues/3206 is resolved

hlsl = generator(python3,
    output : [ '@BASENAME@.ps.cso', '@BASENAME@.vs.cso' ],
    arguments : [ script, '@INPUT@', '@BUILD_DIR@/@BASENAME@', 'vs,ps', '6_0' ]
)

# install agility dlls
install_data(
    [ here/'agility/D3D12Core.dll', here/'agility/D3D12SdkLayers.dll' ],
    install_dir : get_option('prefix') / 'D3D12'
)
