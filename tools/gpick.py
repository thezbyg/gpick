#!/usr/bin/env python

import os
import time
import SCons
import re
import string
import sys
import glob
import subprocess

from lemon import *
from flex import *
from gettext import *
from resource_template import *

from SCons.Script import *
from SCons.Util import *
from SCons.Script.SConscript import SConsEnvironment

import SCons.Script.SConscript
import SCons.SConf
import SCons.Conftest

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

def CheckPKG(context, name):
	context.Message('Checking for library %s... ' % name)
	ret = context.TryAction('pkg-config --exists "%s"' % name)[0]
	context.Result(ret)
	return ret

def CheckProgram(context, env, name, member_name):
	context.Message('Checking for program %s... ' % name)
	if env[member_name]:
		context.Result(True)
		return True
	else:
		context.Result(False)
		return False

class GpickLibrary(NodeList):
	include_dirs = []

class GpickEnvironment(SConsEnvironment):
	
	extern_libs = {}
	
	def AddCustomBuilders(self):
		addLemonBuilder(self)
		addFlexBuilder(self)
		addGettextBuilder(self)
		addResourceTemplateBuilder(self)
		
	def DefineLibrary(self, library_name, library):
		self.extern_libs[library_name] = library
		
	def UseLibrary(self, library_name):
		lib = self.extern_libs[library_name]
		
		for i in lib:
			lib_include_path = os.path.split(i.path)[0]
			self.PrependUnique(LIBS = [library_name], LIBPATH = ['#' + lib_include_path])
			
		self.PrependUnique(CPPPATH = lib.include_dirs)
		
		return lib

	def ConfirmPrograms(self, conf, programs):
		conf.AddTests({'CheckProgram': CheckProgram})
		
		for evar, args in programs.iteritems():
			found = False
			for name, member_name in args['checks'].iteritems():
				if conf.CheckProgram(self, name, member_name):
					found = True;
					break
			if not found:
				if 'required' in args:
					if not args['required']==False:
						self.Exit(1)
				else:
					self.Exit(1)

	def ConfirmLibs(self, conf, libs):
		conf.AddTests({'CheckPKG': CheckPKG})
		
		for evar, args in libs.iteritems():
			found = False
			for name, version in args['checks'].iteritems():
				if conf.CheckPKG(name + ' ' + version):
					self[evar]=name
					found = True;
					break
			if not found:
				if 'required' in args:
					if not args['required']==False:
						self.Exit(1)
				else:
					self.Exit(1)

	def InstallPerm(self, dir, source, perm):
		obj = self.Install(dir, source)
		for i in obj:
			self.AddPostAction(i, Chmod(i, perm))
		return dir

	def InstallPermAutoDir(self, dir, relative_dir, source, perm):
		for f in Flatten(source):
			path = dir
			if str(f.get_dir()).startswith(relative_dir):
				path = os.path.join(path, str(f.get_dir())[len(relative_dir):])
			else:
				path = os.path.join(path, str(f.get_dir()))
			obj = self.Install(path, f)
			for i in obj:
				self.AddPostAction(i, Chmod(i, perm))
		return dir

	InstallProgram = lambda self, dir, source: GpickEnvironment.InstallPerm(self, dir, source, 0755)
	InstallData = lambda self, dir, source: GpickEnvironment.InstallPerm(self, dir, source, 0644)
	InstallDataAutoDir = lambda self, dir, relative_dir, source: GpickEnvironment.InstallPermAutoDir(self, dir, relative_dir, source, 0644)

	def GetSourceFiles(self, dir_exclude_pattern, file_exclude_pattern):
		dir_exclude_prog = re.compile(dir_exclude_pattern)
		file_exclude_prog = re.compile(file_exclude_pattern)
		files = []
		MatchFiles(files, self.GetLaunchDir(), os.sep, dir_exclude_prog, file_exclude_prog)
		return files

	def GetVersionInfo(self):
		try:
			revision = subprocess.Popen(['hg', 'log', '--template', '"{rev}:{node}\\n"', '-r', 'tip',  self.GetLaunchDir()], shell=False, stdout=subprocess.PIPE, stderr=subprocess.STDOUT).communicate()[0]
			match = re.search('([\d]+):([\d\w]+)', str(revision))
			revision = match.group(2)
		except:
			revision = 'not under version control system'

		self.Replace(GPICK_BUILD_REVISION = revision,
			GPICK_BUILD_DATE =  time.strftime ("%Y-%m-%d"),
			GPICK_BUILD_TIME =  time.strftime ("%H:%M:%S"));	

def RegexEscape(str):
	return str.replace('\\', '\\\\')

def WriteNsisVersion(target, source, env):
	for t in target:
		for s in source:
			file = open(str(t),"w")
			file.writelines('!define VERSION "' + str(env['GPICK_BUILD_VERSION']) + '"')
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


