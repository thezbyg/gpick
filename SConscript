#!/usr/bin/env python

import os
import string
import sys

import tools.gpick

env = Environment(ENV=os.environ, BUILDERS = {'WriteNsisVersion' : Builder(action = tools.gpick.WriteNsisVersion, suffix = ".nsi")})

vars = Variables(os.path.join(env.GetLaunchDir(), 'user-config.py'))
vars.Add('DESTDIR', 'Directory to install under', '/usr/local')
vars.Add('DEBARCH', 'Debian package architecture', 'i386')
vars.Add('WITH_UNIQUE', 'Use libunique instead of pure DBus', False)
vars.Add('WITH_DBUSGLIB', 'Compile with DBus support', True)
vars.Add('DEBUG', 'Compile with debug information', False)
vars.Update(env)

v = Variables(os.path.join(env.GetLaunchDir(), 'version.py'))
v.Add('GPICK_BUILD_VERSION', '', '0.0')
v.Update(env)

tools.gpick.GetVersionInfo(env)

try:
	umask = os.umask(022)
except OSError:     # ignore on systems that don't support umask
	pass

def CheckPKG(context, name):
	context.Message( 'Checking for %s... ' % name )
	ret = context.TryAction('pkg-config --exists "%s"' % name)[0]
	context.Result( ret )
	return ret

if not env.GetOption('clean'):
	conf = Configure(env, custom_tests = { 'CheckPKG' : CheckPKG })
	
	libs = {
		'GTK_PC': 			{'checks':{'gtk+-2.0':'>= 2.12.0'}},
		'LUA_PC': 			{'checks':{'lua':'>= 5.1', 'lua5.1':'>= 5.1'}},
	}
	
	if env['WITH_UNIQUE']==True:
		libs['UNIQUE_PC'] = {'checks':{'unique-1.0':'>= 1.0.8'}}
	elif env['WITH_DBUSGLIB']==True:
		libs['DBUSGLIB_PC'] = {'checks':{'dbus-glib-1':'>= 0.76'}}

	tools.gpick.ConfirmLibs(conf, env, libs)
	
	env = conf.Finish()


Decider('MD5-timestamp')

env.Replace(
	SHCCCOMSTR = "Compiling ==> $TARGET",
	SHCXXCOMSTR = "Compiling ==> $TARGET",
	CCCOMSTR = "Compiling ==> $TARGET",
	CXXCOMSTR = "Compiling ==> $TARGET",
	SHLINKCOMSTR = "Linking shared ==> $TARGET",
	LINKCOMSTR = "Linking ==> $TARGET",
	LDMODULECOMSTR = "Linking module ==> $TARGET",
	ARCOMSTR = "Linking static ==> $TARGET",
	TARCOMSTR = "Archiving ==> $TARGET"
	)
	
if env['DEBUG']==False:
	env.Append(LINKFLAGS=['-s'])

executable = SConscript(['source/SConscript'], exports='env')

env.Alias(target="build", source=[
	executable
])

if 'debian' in COMMAND_LINE_TARGETS:
	SConscript("deb/SConscript", exports='env')

env.Alias(target="install", source=[
	env.InstallProgram(dir=env['DESTDIR'] +'/bin', source=[executable]),
	env.InstallData(dir=env['DESTDIR'] +'/share/applications', source=['share/applications/gpick.desktop']),
	env.InstallData(dir=env['DESTDIR'] +'/share/doc/gpick', source=['share/doc/gpick/copyright']),
	env.InstallData(dir=env['DESTDIR'] +'/share/gpick', source=[env.Glob('share/gpick/*.png'), env.Glob('share/gpick/*.lua'), env.Glob('share/gpick/*.txt')]),
	env.InstallData(dir=env['DESTDIR'] +'/share/man/man1', source=['share/man/man1/gpick.1']),
	env.InstallData(dir=env['DESTDIR'] +'/share/icons/hicolor/48x48/apps/', source=[env.Glob('share/icons/hicolor/48x48/apps/*.png')]),
	env.InstallData(dir=env['DESTDIR'] +'/share/icons/hicolor/scalable/apps/', source=[env.Glob('share/icons/hicolor/scalable/apps/*.svg')]),
])

env.Alias(target="nsis", source=[
	env.WriteNsisVersion("version.py")
])

env.Alias(target="tar", source=[
	env.Append(TARFLAGS = ['-z']),
	env.Prepend(TARFLAGS = ['--transform', '"s,^,gpick-'+str(env['GPICK_BUILD_VERSION'])+'.'+str(env['GPICK_BUILD_REVISION'])+'/,"']),
	env.Tar('gpick_'+str(env['GPICK_BUILD_VERSION'])+'.'+str(env['GPICK_BUILD_REVISION'])+'.tar.gz', tools.gpick.GetSourceFiles(env, "("+tools.gpick.RegexEscape(os.sep)+r"\.)|("+tools.gpick.RegexEscape(os.sep)+r"\.svn$)|(^"+tools.gpick.RegexEscape(os.sep)+r"build$)", r"(^\.)|(\.pyc$)|(~$)|(\.log$)|(^gpick-.*\.tar\.gz$)|(^user-config\.py$)"))
])



env.Default(executable)

