#!/usr/bin/env python
# coding: utf-8
import os, string, sys, shutil, math
from tools import *

env = GpickEnvironment(ENV = os.environ)

vars = Variables(os.path.join(env.GetLaunchDir(), 'user-config.py'))
vars.Add('DESTDIR', 'Directory to install under', '/usr/local')
vars.Add('LOCALEDIR', 'Path to locale directory', '')
vars.Add('DEBARCH', 'Debian package architecture', 'i386')
vars.Add(BoolVariable('ENABLE_NLS', 'Compile with gettext support', True))
vars.Add(BoolVariable('DEBUG', 'Compile with debug information', False))
vars.Add('BUILD_TARGET', 'Build target', '')
vars.Add('TOOLCHAIN', 'Toolchain', 'gcc')
vars.Add('MSVS_VERSION', 'Visual Studio version', '11.0')
vars.Add(BoolVariable('PREBUILD_GRAMMAR', 'Use prebuild grammar files', False))
vars.Add(BoolVariable('USE_GTK3', 'Use GTK3 instead of GTK2', True))
vars.Add(BoolVariable('DEV_BUILD', 'Use development flags', False))
vars.Add(BoolVariable('PREFER_VERSION_FILE', 'Read version information from file instead of using GIT', False))
vars.Update(env)

if env['LOCALEDIR'] == '':
	env['LOCALEDIR'] = env['DESTDIR'] + '/share/locale'

if not env['BUILD_TARGET']:
	env['BUILD_TARGET'] = sys.platform

if env['BUILD_TARGET'] == 'win32':
	if env['TOOLCHAIN'] == 'msvc':
		env['TARGET_ARCH'] = 'x86'
		env['MSVS'] = {'VERSION': env['MSVS_VERSION']}
		env['MSVC_VERSION'] = env['MSVS_VERSION']
		env["MSVC_SETUP_RUN"] = False
		Tool('msvc')(env)
	else:
		if sys.platform != 'win32':
			env.Tool('crossmingw', toolpath = ['tools'])
		else:
			env.Tool('mingw')

env.AddCustomBuilders()
env.GetVersionInfo(env['PREFER_VERSION_FILE'])

try:
	umask = os.umask(0o022)
except OSError: # ignore on systems that don't support umask
	pass

if 'CC' in os.environ:
	env['CC'] = os.environ['CC']
if 'CFLAGS' in os.environ:
	env['CCFLAGS'] += SCons.Util.CLVar(os.environ['CFLAGS'])
if 'CXX' in os.environ:
	env['CXX'] = os.environ['CXX']
if 'CXXFLAGS' in os.environ:
	env['CXXFLAGS'] += SCons.Util.CLVar(os.environ['CXXFLAGS'])
if 'LDFLAGS' in os.environ:
	env['LINKFLAGS'] += SCons.Util.CLVar(os.environ['LDFLAGS'])

if not env.GetOption('clean'):
	conf = Configure(env)

	programs = {}
	if env['ENABLE_NLS']:
		programs['GETTEXT'] = {'checks':{'msgfmt': 'GETTEXT'}}
		programs['XGETTEXT'] = {'checks':{'xgettext': 'XGETTEXT'}, 'required': False}
		programs['MSGMERGE'] = {'checks':{'msgmerge': 'MSGMERGE'}, 'required': False}
		programs['MSGCAT'] = {'checks':{'msgcat': 'MSGCAT'}, 'required': False}
	programs['RAGEL'] = {'checks':{'ragel': 'RAGEL'}}
	env.ConfirmPrograms(conf, programs)

	libs = {}

	if not env['TOOLCHAIN'] == 'msvc':
		if not env['USE_GTK3']:
			libs['GTK_PC'] = {'checks':{'gtk+-2.0': '>= 2.24.0'}}
			libs['GIO_PC'] = {'checks':{'gio-unix-2.0': '>= 2.26.0', 'gio-2.0': '>= 2.26.0'}}
		else:
			libs['GTK_PC'] = {'checks':{'gtk+-3.0': '>= 3.0.0'}}
		libs['LUA_PC'] = {'checks':{'lua5.4-c++': '>= 5.4', 'lua5.3-c++': '>= 5.3', 'lua-c++': '>= 5.2', 'lua5.2-c++': '>= 5.2'}}
	env.ConfirmLibs(conf, libs)
	env.ConfirmBoost(conf, '1.71')
	env = conf.Finish()

Decider('MD5-timestamp')

if not env['TOOLCHAIN'] == 'msvc':
	if not ('CFLAGS' in os.environ or 'CXXFLAGS' in os.environ or 'LDFLAGS' in os.environ):
		if env['DEBUG']:
			env.Append(
				CXXFLAGS = ['-Wall', '-g3', '-O0'],
				CFLAGS = ['-Wall', '-g3', '-O0'],
				LINKFLAGS = ['-Wl,-as-needed'],
				)
		else:
			env.Append(
				CPPDEFINES = ['NDEBUG'],
				CDEFINES = ['NDEBUG'],
				CXXFLAGS = ['-Wall', '-O3'],
				CFLAGS = ['-Wall', '-O3'],
				LINKFLAGS = ['-Wl,-as-needed', '-s'],
				)
		env.Append(
			CXXFLAGS = ['-std=c++17'],
		)
	else:
		stdMissing = True
		for flag in env['CXXFLAGS']:
			if flag.startswith('-std='):
				stdMissing = False
				break
		if stdMissing:
			env.Append(
				CXXFLAGS = ['-std=c++17'],
			)
	if env['DEV_BUILD']:
		env.Append(
			CXXFLAGS = ['-Werror'],
			CFLAGS = ['-Werror'],
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
				CXXFLAGS = ['/Od', '/EHsc', '/MD', '/Gy', '/Zi', '/TP', '/wd4819'],
				CPPDEFINES = ['WIN32', '_DEBUG', 'NOMINMAX'],
				LINKFLAGS = ['/MANIFEST', '/DEBUG'],
			)
	else:
		env.Append(
				CXXFLAGS = ['/O2', '/Oi', '/GL', '/EHsc', '/MD', '/Gy', '/Zi', '/TP', '/wd4819'],
				CPPDEFINES = ['WIN32', 'NDEBUG', 'NOMINMAX'],
				LINKFLAGS = ['/MANIFEST', '/LTCG'],
			)

if env['DEV_BUILD']:
	env.Append(CPPDEFINES = ['GPICK_DEV_BUILD'])

env.Append(CPPPATH = ['#source'])

def buildVersion(env):
	version_env = env.Clone()
	version_env['GPICK_BUILD_HASH'] = version_env['GPICK_BUILD_HASH'][:10]
	sources = version_env.Template(version_env.Glob('source/version/*.in'), TEMPLATE_ENV_FILTER = ['GPICK_*'])
	return version_env.StaticObject(version_env.Glob('#source/version/*.cpp') + sources)

def buildLayout(env):
	layout_env = env.Clone()
	if not env.GetOption('clean') and not env['TOOLCHAIN'] == 'msvc':
		layout_env.ParseConfig('pkg-config --cflags --libs $GTK_PC')
		layout_env.ParseConfig('pkg-config --cflags --libs $LUA_PC')
	return layout_env.StaticObject(layout_env.Glob('source/layout/*.cpp'))

def buildGtk(env):
	gtk_env = env.Clone()
	if not env.GetOption('clean') and not env['TOOLCHAIN'] == 'msvc':
		gtk_env.ParseConfig('pkg-config --cflags --libs $GTK_PC')
		gtk_env.ParseConfig('pkg-config --cflags --libs $LUA_PC')
	return gtk_env.StaticObject(gtk_env.Glob('source/gtk/*.cpp'))

def buildI18n(env):
	i18n_env = env.Clone()
	if not env.GetOption('clean'):
		if i18n_env['ENABLE_NLS']:
			i18n_env.Append(CPPDEFINES = ['ENABLE_NLS', 'LOCALEDIR=' + i18n_env['LOCALEDIR']])
	return i18n_env.StaticObject(i18n_env.Glob('source/i18n/*.cpp'))

def buildDbus(env):
	dbus_env = env.Clone()
	if not env.GetOption('clean') and not env['TOOLCHAIN'] == 'msvc':
		if not env['USE_GTK3']:
			dbus_env.ParseConfig('pkg-config --cflags $GIO_PC')
		else:
			dbus_env.ParseConfig('pkg-config --cflags $GTK_PC')
	if not env['BUILD_TARGET'] == 'win32':
		sources = dbus_env.Glob('source/dbus/*.c') + dbus_env.Glob('source/dbus/*.cpp')
	else:
		sources = dbus_env.Glob('source/dbus/*.cpp')
	return dbus_env.StaticObject(sources)

def buildTools(env):
	tools_env = env.Clone()
	if not env.GetOption('clean') and not env['TOOLCHAIN'] == 'msvc':
		tools_env.ParseConfig('pkg-config --cflags --libs $GTK_PC')
		tools_env.ParseConfig('pkg-config --cflags --libs $LUA_PC')
	if tools_env['ENABLE_NLS']:
		tools_env.Append(CPPDEFINES = ['ENABLE_NLS'])
	return tools_env.StaticObject(tools_env.Glob('source/tools/*.cpp'))

def buildLua(env):
	lua_env = env.Clone()
	if not env.GetOption('clean') and not env['TOOLCHAIN'] == 'msvc':
		lua_env.ParseConfig('pkg-config --cflags --libs $GTK_PC')
		lua_env.ParseConfig('pkg-config --cflags --libs $LUA_PC')
	return lua_env.StaticObject(lua_env.Glob('source/lua/*.cpp'))

def buildColorNames(env):
	return env.StaticObject(env.Glob('source/color_names/*.cpp'))

def buildMath(env):
	return env.StaticObject(env.Glob('source/math/*.cpp'))

def buildWindowsResources(env):
	resources_env = env.Clone()
	resources = resources_env.Template(resources_env.Glob('source/winres/*.rc.in'), TEMPLATE_ENV_FILTER = ['GPICK_*'])
	objects = resources_env.RES(resources)
	Command("source/winres/gpick-icon.ico", File("source/winres/gpick-icon.ico").srcnode(), Copy("$TARGET", "${SOURCE}"))
	if not (env['TOOLCHAIN'] == 'msvc'):
		Command("source/winres/gpick.exe.manifest", File("source/winres/gpick.exe.manifest").srcnode(), Copy("$TARGET", "${SOURCE}"))
	Depends(resources, 'source/winres/gpick-icon.ico')
	if not (env['TOOLCHAIN'] == 'msvc'):
		Depends(resources, 'source/winres/gpick.exe.manifest')
	return objects

def addDebianPackageAlias(env):
	DEBNAME = "gpick"
	DEBVERSION = str(env['GPICK_BUILD_VERSION_FULL'])
	DEBMAINT = "Albertas Vyšniauskas <albertas.vysniauskas@gpick.org>"
	DEBARCH = env['DEBARCH']
	DEBDEPENDS = "libgtk2.0-0 (>= 2.24), libc6 (>= 2.13), liblua5.2-0 (>= 5.2), libcairo2 (>=1.8), libglib2.0-0 (>=2.24)"
	DEBPRIORITY = "optional"
	DEBSECTION = "graphics"
	DEBDESC = "Advanced color picker"
	DEBDESCLONG = """ Gpick is a program used to pick colors
 from anywhere on the screen, mix them to
 get new colors, generate shades and tints
 and export palettes to common file formats
 or simply copy them to the clipboard
"""
	DEBPACKAGEFILE = '%s_%s_%s.deb' % (DEBNAME, DEBVERSION, DEBARCH)
	CONTROL_TEMPLATE = """Package: %s
Version: %s
Section: %s
Priority: %s
Architecture: %s
Depends: %s
Installed-Size: %s
Maintainer: %s
Description: %s
%s"""
	DEBCONTROLDIR = os.path.join("deb", DEBNAME, "DEBIAN")
	DEBCONTROLFILE = os.path.join(DEBCONTROLDIR, "control")
	DEBINSTALLDIR = os.path.join('deb' , DEBNAME, 'usr')
	env['DESTDIR'] = DEBINSTALLDIR #redirect install location
	def writeControlFile(target = None, source = None, env = None):
		installedSize = 0
		files = Glob(os.path.join('build', 'deb', DEBNAME, 'usr'))
		for i in files:
			installedSize += os.stat(str(i))[6]
		installedSize = int(math.ceil(installedSize/1024))
		controlInfo = CONTROL_TEMPLATE % (
			DEBNAME, DEBVERSION, DEBSECTION, DEBPRIORITY, DEBARCH,
			DEBDEPENDS, str(installedSize), DEBMAINT, DEBDESC, DEBDESCLONG)
		f = open(str(target[0]), 'w')
		f.write(controlInfo)
		f.close()
		return None
	env.Append(BUILDERS = {
		'DebianPackage': Builder(action = "fakeroot dpkg-deb -b %s %s" % ("$SOURCE", "$TARGET")),
		'DebianControl': Builder(action = writeControlFile),
	})
	env.Alias(target = "debian", source = [
		env.Install(dir = DEBCONTROLDIR, source = [env.Glob("deb/DEBIAN/*")]),
		env.DebianControl(source = env.Alias('install'), target = DEBCONTROLFILE),
		env.DebianPackage(source = env.Dir(os.path.join('deb', DEBNAME)), target = os.path.join('.', DEBPACKAGEFILE))
	])

def buildGpick(env):
	gpick_env = env.Clone()
	if not env.GetOption('clean') and not env['TOOLCHAIN'] == 'msvc':
		gpick_env.ParseConfig('pkg-config --cflags --libs $GTK_PC', None, False)
		gpick_env.ParseConfig('pkg-config --cflags --libs $LUA_PC', None, False)
	if env['ENABLE_NLS']:
		gpick_env.Append(CPPDEFINES = ['ENABLE_NLS'])
	gpick_env.Append(CPPDEFINES = ['GSEAL_ENABLE'])
	sources = gpick_env.Glob('source/*.cpp') + gpick_env.Glob('source/transformation/*.cpp')

	objects = []
	objects += buildVersion(env)
	objects += buildGtk(env)
	objects += buildLayout(env)
	objects += buildI18n(env)
	objects += buildDbus(env)
	objects += buildTools(env)
	objects += buildLua(env)
	objects += buildColorNames(env)
	objects += buildMath(env)

	if env['TOOLCHAIN'] == 'msvc':
		gpick_env.Append(LIBS = ['glib-2.0', 'gtk-win32-2.0', 'gobject-2.0', 'gdk-win32-2.0', 'cairo', 'gdk_pixbuf-2.0', 'lua5.2', 'expat2.1', 'pango-1.0', 'pangocairo-1.0', 'intl'])
		gpick_env.Append(LINKFLAGS = ['/SUBSYSTEM:WINDOWS', '/ENTRY:mainCRTStartup'], CPPDEFINES = ['XML_STATIC'])
		objects += buildWindowsResources(env)

	if not gpick_env['BUILD_TARGET'] == 'win32':
		gpick_env.Append(LIBS = ['expat'])
		if gpick_env['BUILD_TARGET'].startswith('linux') or gpick_env['BUILD_TARGET'].startswith('gnu0') or gpick_env['BUILD_TARGET'].startswith('gnukfreebsd'):
			gpick_env.Append(LIBS = ['rt'])

	text_file_parser_objects = gpick_env.StaticObject(['source/parser/TextFile.cpp', gpick_env.Ragel('source/parser/TextFileParser.rl')])
	objects += text_file_parser_objects

	dynv_objects = gpick_env.StaticObject(gpick_env.Glob('source/dynv/*.cpp'))
	objects += dynv_objects

	common_objects = gpick_env.StaticObject(gpick_env.Glob('source/common/*.cpp'))
	objects += common_objects

	gpick_objects = gpick_env.StaticObject(sources)
	objects += gpick_objects

	object_map = {}
	for obj in objects:
		if str(obj.dir) == '.':
			object_map[os.path.splitext(obj.name)[0]] = obj
		else:
			object_map[str(obj.dir) + '/' + os.path.splitext(obj.name)[0]] = obj

	executable = gpick_env.Program('gpick', source = [objects])

	test_env = gpick_env.Clone()
	test_env.Append(LIBS = ['boost_unit_test_framework'], CPPDEFINES = ['BOOST_TEST_DYN_LINK'])

	tests = test_env.Program('tests', source = test_env.Glob('source/test/*.cpp') + [object_map['source/' + name] for name in ['Color', 'EventBus', 'lua/Script', 'lua/Ref', 'lua/Color', 'lua/ColorObject', 'ColorList', 'ColorObject', 'FileFormat', 'ErrorCode', 'Converter', 'Converters', 'InternalConverters', 'version/Version']] + dynv_objects + text_file_parser_objects + common_objects)

	return executable, tests

executable, tests = buildGpick(env)

env.Alias(target = "build", source = [executable, env.Install('source', executable)])
env.Alias(target = "test", source = [tests, env.Install('source', tests)])

if 'debian' in COMMAND_LINE_TARGETS:
	addDebianPackageAlias(env)

if env['ENABLE_NLS']:
	translations = env.Glob('share/locale/*/LC_MESSAGES/gpick.po')
	locales = env.Gettext(translations)
	Depends(executable, locales)
	stripped_locales = []
	for translation in translations:
		stripped_locales.append(env.Msgcat(translation, File(translation).srcnode(), MSGCAT_FLAGS = ['--no-location', '--sort-output', '--no-wrap', '--to-code=utf-8']))
	env.Alias(target = "strip_locales", source = stripped_locales)
	env.Alias(target = "locales", source = locales)
	template_c = env.Xgettext("template_c.pot", env.Glob('source/*.cpp') + env.Glob('source/tools/*.cpp') + env.Glob('source/transformation/*.cpp'), XGETTEXT_FLAGS = ['--keyword=N_', '--from-code=UTF-8', '--package-version="$GPICK_BUILD_VERSION_FULL"'])
	template_lua = env.Xgettext("template_lua.pot", env.Glob('share/gpick/*.lua'), XGETTEXT_FLAGS = ['--language=C++', '--keyword=N_', '--from-code=UTF-8', '--package-version="$GPICK_BUILD_VERSION_FULL"'])
	template = env.Msgcat("template.pot", [template_c, template_lua], MSGCAT_FLAGS = ['--use-first'])
	env.Alias(target = "template", source = [
		template
	])

env.Alias(target = "install", source = [
	env.InstallProgram(dir = env['DESTDIR'] +'/bin', source = [executable]),
	env.InstallData(dir = env['DESTDIR'] +'/share/metainfo', source = ['share/metainfo/org.gpick.gpick.metainfo.xml']),
	env.InstallData(dir = env['DESTDIR'] +'/share/applications', source = ['share/applications/org.gpick.gpick.desktop']),
	env.InstallData(dir = env['DESTDIR'] +'/share/mime/packages', source = ['share/mime/packages/org.gpick.gpick.xml']),
	env.InstallData(dir = env['DESTDIR'] +'/share/doc/gpick', source = ['share/doc/gpick/copyright']),
	env.InstallData(dir = env['DESTDIR'] +'/share/gpick', source = [env.Glob('share/gpick/*.png'), env.Glob('share/gpick/*.lua'), env.Glob('share/gpick/*.txt'), env.Glob('share/gpick/.gpick-data-directory')]),
	env.InstallData(dir = env['DESTDIR'] +'/share/man/man1', source = ['share/man/man1/gpick.1']),
	env.InstallData(dir = env['DESTDIR'] +'/share/icons/hicolor/48x48/apps/', source = [env.Glob('share/icons/hicolor/48x48/apps/*.png')]),
	env.InstallData(dir = env['DESTDIR'] +'/share/icons/hicolor/scalable/apps/', source = [env.Glob('share/icons/hicolor/scalable/apps/*.svg')]),
	env.InstallDataAutoDir(dir = env['DESTDIR'] + '/share/locale/', relative_dir = 'share/locale/', source = [env.Glob('share/locale/*/LC_MESSAGES/gpick.mo')]),
])

env.Alias(target = "version", source = [
	env.AlwaysBuild(env.Template(target = ".version", source = None, TEMPLATE_ENV_FILTER = ['GPICK_*'], TEMPLATE_SOURCE = '@GPICK_BUILD_VERSION@\n@GPICK_BUILD_REVISION@\n@GPICK_BUILD_HASH@\n@GPICK_BUILD_DATE@\n'))
])

def phony(env, target, action):
	alias = env.Alias(target, [], action)
	env.AlwaysBuild(alias)
	return alias

env.Alias("archive", ['version'], [
	'git archive --format=tar.gz --prefix="gpick-${GPICK_BUILD_VERSION_FULL}/" --add-file="build/.version" --output="build/gpick-${GPICK_BUILD_VERSION_FULL}.tar.gz" HEAD',
	'git archive --format=zip --prefix="gpick-${GPICK_BUILD_VERSION_FULL}/" --add-file="build/.version" --output="build/gpick-${GPICK_BUILD_VERSION_FULL}.zip" HEAD',
])

env.Default(executable)

