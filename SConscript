#!/usr/bin/env python

import os
import string
import sys

from tools.gpick import *

env = GpickEnvironment(ENV=os.environ, BUILDERS = {'WriteNsisVersion' : Builder(action = WriteNsisVersion, suffix = ".nsi")})

vars = Variables(os.path.join(env.GetLaunchDir(), 'user-config.py'))
vars.Add('DESTDIR', 'Directory to install under', '/usr/local')
vars.Add('LOCALEDIR', 'Path to locale directory', '')
vars.Add('DEBARCH', 'Debian package architecture', 'i386')
vars.Add(BoolVariable('ENABLE_NLS', 'Compile with gettext support', True))
vars.Add(BoolVariable('DEBUG', 'Compile with debug information', False))
vars.Add('BUILD_TARGET', 'Build target', '')
vars.Add('TOOLCHAIN', 'Toolchain', 'gcc')
vars.Add(BoolVariable('EXPERIMENTAL_CSS_PARSER', 'Compile with experimental CSS parser', False))
vars.Add('MSVS_VERSION', 'Visual Studio version', '11.0')
vars.Add(BoolVariable('PREBUILD_GRAMMAR', 'Use prebuild grammar files', False))
vars.Update(env)

if env['LOCALEDIR'] == '':
	env['LOCALEDIR'] = env['DESTDIR'] + '/share/locale'

v = Variables(os.path.join(env.GetLaunchDir(), 'version.py'))
v.Add('GPICK_BUILD_VERSION', '', '0.0')
v.Update(env)

if not env['BUILD_TARGET']:
	env['BUILD_TARGET'] = sys.platform

if env['BUILD_TARGET'] == 'win32':
	if env['TOOLCHAIN'] == 'msvc':
		env['TARGET_ARCH'] = 'x86'
		env['MSVS'] = {'VERSION': env['MSVS_VERSION']} 
		env['MSVC_VERSION'] = env['MSVS_VERSION']
		Tool('msvc')(env)
	else:
		if sys.platform != 'win32':
			env.Tool('crossmingw', toolpath = ['tools'])
		else:
			env.Tool('mingw')

env.AddCustomBuilders()
env.GetVersionInfo()

try:
	umask = os.umask(022)
except OSError:     # ignore on systems that don't support umask
	pass


if not env.GetOption('clean'):
	conf = Configure(env)

	programs = {}
	if env['ENABLE_NLS']:
		programs['GETTEXT'] = {'checks':{'msgfmt':'GETTEXT'}}
		programs['XGETTEXT'] = {'checks':{'xgettext':'XGETTEXT'}, 'required':False}
		programs['MSGMERGE'] = {'checks':{'msgmerge':'MSGMERGE'}, 'required':False}
		programs['MSGCAT'] = {'checks':{'msgcat':'MSGCAT'}, 'required':False}
	if env['EXPERIMENTAL_CSS_PARSER'] and not env['PREBUILD_GRAMMAR']:
		programs['LEMON'] = {'checks':{'lemon':'LEMON'}}
		programs['FLEX'] = {'checks':{'flex':'FLEX'}}
	env.ConfirmPrograms(conf, programs)

	libs = {}

	if not env['TOOLCHAIN'] == 'msvc':
		libs['GTK_PC'] = {'checks':{'gtk+-2.0':'>= 2.24.0'}}
		libs['GIO_PC'] = {'checks':{'gio-unix-2.0':'>= 2.26.0', 'gio-2.0':'>= 2.26.0'}}
		libs['LUA_PC'] = {'checks':{'lua':'>= 5.2', 'lua5.2':'>= 5.2'}}

	env.ConfirmLibs(conf, libs)

	env = conf.Finish()

if os.environ.has_key('CC'):
	env['CC'] = os.environ['CC']
if os.environ.has_key('CFLAGS'):
	env['CCFLAGS'] += SCons.Util.CLVar(os.environ['CFLAGS'])
if os.environ.has_key('CXX'):
	env['CXX'] = os.environ['CXX']
if os.environ.has_key('CXXFLAGS'):
	env['CXXFLAGS'] += SCons.Util.CLVar(os.environ['CXXFLAGS'])
if os.environ.has_key('LDFLAGS'):
	env['LINKFLAGS'] += SCons.Util.CLVar(os.environ['LDFLAGS'])
	
Decider('MD5-timestamp')

if not env['TOOLCHAIN'] == 'msvc':
	if not (os.environ.has_key('CFLAGS') or os.environ.has_key('CXXFLAGS') or os.environ.has_key('LDFLAGS')):
		if env['DEBUG']:
			env.Append(
				CPPFLAGS = ['-Wall', '-g3', '-O0'],
				CFLAGS = ['-Wall', '-g3', '-O0'],
				LINKFLAGS = ['-Wl,-as-needed'],
				)
		else:
			env.Append(
				CPPDEFINES = ['NDEBUG'],
				CDEFINES = ['NDEBUG'],
				CPPFLAGS = ['-Wall', '-O3'],
				CFLAGS = ['-Wall', '-O3'],
				LINKFLAGS = ['-Wl,-as-needed', '-s'],
				)

	if env['BUILD_TARGET'] == 'win32':
		env.Append(	
				LINKFLAGS = ['-Wl,--enable-auto-import', '-static-libgcc', '-static-libstdc++'],
				CPPDEFINES = ['_WIN32_WINNT=0x0501'],
				)
else:
	env['LINKCOM'] = [env['LINKCOM'], 'mt.exe -nologo -manifest ${TARGET}.manifest -outputresource:$TARGET;1']
	if env['DEBUG']:
		env.Append(
				CPPFLAGS = ['/EHsc', '/O2', '/GL', '/MD', '/Zi', '/wd4819'],
				CPPDEFINES = {'_BIND_TO_CURRENT_VCLIBS_VERSION': '1'},
				LINKFLAGS = ['/LTCG', '/MANIFEST', '/DEBUG'],
			)
	else:
		env.Append(
				CPPFLAGS = ['/EHsc', '/O2', '/GL', '/MD', '/Zi', '/wd4819'],
				CPPDEFINES = {'_BIND_TO_CURRENT_VCLIBS_VERSION': '1'},
				LINKFLAGS = ['/LTCG', '/MANIFEST', '/DEBUG'],
			)
			
extern_libs = SConscript(['extern/SConscript'], exports='env')
executable, parser_files = SConscript(['source/SConscript'], exports='env')

env.Alias(target="build", source=[
	executable
])

if 'debian' in COMMAND_LINE_TARGETS:
	SConscript("deb/SConscript", exports='env')


if env['ENABLE_NLS']:
	locales = env.Gettext(env.Glob('share/locale/*/LC_MESSAGES/gpick.po'))
	Depends(executable, locales)
	
	env.Alias(target="locales", source=[
		locales
	])

	template_c = env.Xgettext("template_c.pot", env.Glob('source/*.cpp') + env.Glob('source/tools/*.cpp') + env.Glob('source/transformation/*.cpp'), XGETTEXT_FLAGS = ['--keyword=N_', '--from-code=UTF-8', '--package-version="$GPICK_BUILD_VERSION"'])
	template_lua = env.Xgettext("template_lua.pot", env.Glob('share/gpick/*.lua'), XGETTEXT_FLAGS = ['--language=C++', '--keyword=N_', '--from-code=UTF-8', '--package-version="$GPICK_BUILD_VERSION"'])

	template = env.Msgcat("template.pot", [template_c, template_lua], MSGCAT_FLAGS = ['--use-first'])

	env.Alias(target="template", source=[
		template
	])

env.Alias(target="install", source=[
	env.InstallProgram(dir=env['DESTDIR'] +'/bin', source=[executable]),
	env.InstallData(dir=env['DESTDIR'] +'/share/applications', source=['share/applications/gpick.desktop']),
	env.InstallData(dir=env['DESTDIR'] +'/share/doc/gpick', source=['share/doc/gpick/copyright']),
	env.InstallData(dir=env['DESTDIR'] +'/share/gpick', source=[env.Glob('share/gpick/*.png'), env.Glob('share/gpick/*.lua'), env.Glob('share/gpick/*.txt')]),
	env.InstallData(dir=env['DESTDIR'] +'/share/man/man1', source=['share/man/man1/gpick.1']),
	env.InstallData(dir=env['DESTDIR'] +'/share/icons/hicolor/48x48/apps/', source=[env.Glob('share/icons/hicolor/48x48/apps/*.png')]),
	env.InstallData(dir=env['DESTDIR'] +'/share/icons/hicolor/scalable/apps/', source=[env.Glob('share/icons/hicolor/scalable/apps/*.svg')]),
	env.InstallDataAutoDir(dir=env['DESTDIR'] + '/share/locale/', relative_dir='share/locale/', source=[env.Glob('share/locale/*/LC_MESSAGES/gpick.mo')]),
])

env.Alias(target="nsis", source=[
	env.WriteNsisVersion("version.py")
])

tarFiles = env.GetSourceFiles( "("+RegexEscape(os.sep)+r"\.)|("+RegexEscape(os.sep)+r"\.svn$)|(^"+RegexEscape(os.sep)+r"build$)", r"(^\.)|(\.pyc$)|(\.orig$)|(~$)|(\.log$)|(\.diff)|(\.mo$)|(^gpick-.*\.tar\.gz$)|(^user-config\.py$)")

for item in parser_files:
	tarFiles.append(str(item))

if 'TAR' in env:
	env.Alias(target="tar", source=[
		env.Append(TARFLAGS = ['-z']),
		env.Prepend(TARFLAGS = ['--transform', '"s,(^(build/)?),gpick_'+str(env['GPICK_BUILD_VERSION'])+'/,x"']),
		env.Tar('gpick_'+str(env['GPICK_BUILD_VERSION'])+'.tar.gz', tarFiles)
	])

env.Default(executable)

