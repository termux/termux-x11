project(
	'lorie',
	'c', 'cpp',
	version: '0.0.1',
	license: 'MIT',
	meson_version: '>=0.47.0',
	default_options: [
		'c_std=c11',
		'warning_level=1',
		'b_lundef=false',
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

incdirs = include_directories('.', 'include', 'scanner')
proxy_srcs = [
	'client.cpp',
	'clipboard.cpp',
	'compositor.cpp',
	'egl-helper.cpp',
	'message-queue.cpp',
	'renderer.cpp',
	'seat.cpp',
	'surface.cpp',
	'output.cpp',
	'utils/log.cpp',
	'utils/utils.cpp',
	'scanner/wayland.cpp',
	
	'backend/x11/x11.cpp',
	'backend/x11/x11-window.cpp',
	'backend/x11/stacktrace.cpp'
]

add_global_arguments('-finstrument-functions', language : 'c')
add_global_arguments('-finstrument-functions', language : 'cpp')

lorie = executable(
	'lorie',
	sources: proxy_srcs,
	include_directories: incdirs,
	dependencies: [egl, glesv2, libx11, libx11_xcb, wayland_server, libxkbcommon, libxkbcommon_x11],
	link_args: ['-O0', '-lm', '-ldl', '-lXcursor', '-lpthread', '-g', '-rdynamic', '-finstrument-functions']
)
