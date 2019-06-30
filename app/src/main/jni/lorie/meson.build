project(
	'xwayland_proxy',
	'c',
	version: '0.0.1',
	license: 'MIT',
	meson_version: '>=0.47.0',
	default_options: [
		'c_std=c11',
		'warning_level=1',
		'b_lundef=false',
		'prefix=/usr/local'
	],
)

pkg = import('pkgconfig')
compiler = meson.get_compiler('c')
wayland_server = dependency('wayland-server', version: '>=1.16')
wayland_client = dependency('wayland-client')
egl = compiler.find_library('EGL')
glesv2 = compiler.find_library('GLESv2')
libx11 = dependency('x11')
libxkbcommon = dependency('xkbcommon')
libxkbcommon_x11 = dependency('xkbcommon-x11')
libx11_xcb = dependency('x11-xcb')

incdirs = include_directories('.')
proxy_srcs = [
	'backend-x11.c', 
	'main.c',
	'renderer.c'
]

add_global_arguments('-finstrument-functions', language : 'c')

proxy = executable(
	'proxy',
	sources: proxy_srcs,
	include_directories: incdirs,
	dependencies: [egl, glesv2, libx11, libx11_xcb, wayland_server, libxkbcommon, libxkbcommon_x11],
	link_args: ['-O0', '-lm', '-ldl', '-g', '-rdynamic', '-finstrument-functions']
)
