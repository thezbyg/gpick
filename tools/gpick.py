#!/usr/bin/env python

import os
import time
import SCons
import re
import string
import sys
import glob
import subprocess

from SCons.Script import *
from SCons.Script.SConscript import SConsEnvironment

import SCons.Script.SConscript
import SCons.SConf
import SCons.Conftest

def ConfirmLibs(conf, env, libs):
	for evar, args in libs.iteritems():
		found = False
		for name, version in args['checks'].iteritems():
			if conf.CheckPKG(name + ' ' + version):
				env[evar]=name
				found = True;
				break
		if not found:
			if 'required' in args:
				if not args['required']==False:
					env.Exit(1)
			else:
				env.Exit(1)


def InstallPerm(env, dir, source, perm):
	obj = env.Install(dir, source)
	for i in obj:
		env.AddPostAction(i, Chmod(i, perm))
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
	try:
		svn_revision = subprocess.Popen(['svnversion', '-n',  env.GetLaunchDir()], shell=False, stdout=subprocess.PIPE).communicate()[0]
		svn_revision = str(svn_revision)
		if svn_revision=="exported":
			svn_revision="0"
		svn_revision=svn_revision.replace(':','.')
		svn_revision=svn_revision.rstrip('PSM')
		revision=svn_revision;
	except OSError, e:
		revision = '0'

	env.Replace(GPICK_BUILD_REVISION = revision,
		GPICK_BUILD_DATE =  time.strftime ("%Y-%m-%d"),
		GPICK_BUILD_TIME =  time.strftime ("%H:%M:%S"));	

def RegexEscape(str):
	return str.replace('\\', '\\\\')

def WriteNsisVersion(target, source, env):
	for t in target:
		for s in source:
			file = open(str(t),"w")
			file.writelines('!define VERSION "' + str(env['GPICK_BUILD_VERSION'])+'.'+str(env['GPICK_BUILD_REVISION']) + '"')
			file.close()
	return 0

def Glob(path):
	files = []
	for f in glob.glob(os.path.join(path, '*')):
		if os.path.isdir(str(f)):
			files.extend(Glob(str(f)));
		else:
			files.append(str(f));
	return files

