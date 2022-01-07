project('discord', 'c')

here = meson.current_source_dir()
links = [ here/'lib/discord_game_sdk.dll.lib' ]

gamesdk = declare_dependency(
    include_directories : '.',
    link_args : links
)
