#!/usr/bin/env python

import os
import time
import SCons
import re
import string
from SCons.Script.SConscript import SConsEnvironment

SConsEnvironment.Chmod = SCons.Action.ActionFactory(os.chmod, lambda dest, mode: 'Chmod("%s", 0%o)' % (dest, mode))

def InstallPerm(env, dir, source, perm):
	obj = env.Install(dir, source)
	for i in obj:
		env.AddPostAction(i, env.Chmod(str(i), perm))
	return dir

SConsEnvironment.InstallPerm = InstallPerm

SConsEnvironment.InstallProgram = lambda env, dir, source: InstallPerm(env, dir, source, 0755)
SConsEnvironment.InstallData = lambda env, dir, source: InstallPerm(env, dir, source, 0644)


def MatchFiles (files, path, repath, dir_exclude_pattern,  file_exclude_pattern):

	for filename in os.listdir(path):
		fullname = os.path.join (path, filename)
		repath_file =  os.path.join (repath, filename);
		if os.path.isdir (fullname):
			if not dir_exclude_pattern.search(repath_file):
				MatchFiles (files, fullname, repath_file, dir_exclude_pattern, file_exclude_pattern)
		else:
			if not file_exclude_pattern.search(filename):
				files.append (fullname)

	
def GetSourceFiles(env, dir_exclude_pattern, file_exclude_pattern):
	dir_exclude_prog = re.compile(dir_exclude_pattern)
	file_exclude_prog = re.compile(file_exclude_pattern)
	files = []
	MatchFiles(files, env.GetLaunchDir(), os.sep, dir_exclude_prog, file_exclude_prog)
	return files
	
def GetVersionInfo(env):
	revision = os.popen('svnversion -n %s' % env.GetLaunchDir() ).read()
	revision=revision.replace(':','.')
	env.Replace(GPICK_BUILD_REVISION = revision,
		GPICK_BUILD_DATE =  time.strftime ("%Y-%m-%d"),
		GPICK_BUILD_TIME =  time.strftime ("%H:%M:%S"));		

def RegexEscape(str):
	return str.replace('\\', '\\\\')
