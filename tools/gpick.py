import os, time, re, string, glob, subprocess
from .gettext import *
from .ragel import *
from .template import *
from .version import *
from SCons.Script import Chmod, Flatten
from SCons.Util import NodeList
from SCons.Script.SConscript import SConsEnvironment

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

def CompareVersions(a, b):
	for i in range(0, min(len(a), len(b))):
		if a[i] < b[i]:
			return 1
		if a[i] > b[i]:
			return -1
	return 0

def CheckBoost(context, version):
	context.Message('Checking for library boost >= %s... ' % (version))
	result, boost_version = context.TryRun("""
#include <boost/version.hpp>
#include <iostream>
int main(){
std::cout << BOOST_LIB_VERSION << std::endl;
return 0;
}
""", '.cpp')
	if result:
		found_version = boost_version.strip('\r\n\t ').split('_')
		required_version = version.strip('\r\n\t ').split('.')
		result = CompareVersions(required_version, found_version) >= 0
	context.Result(result)
	return result

class GpickLibrary(NodeList):
	include_dirs = []

class GpickEnvironment(SConsEnvironment):
	extern_libs = {}
	def AddCustomBuilders(self):
		addGettextBuilder(self)
		addTemplateBuilder(self)
		addRagelBuilder(self)
		
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
		
		for evar, args in programs.items():
			found = False
			for name, member_name in args['checks'].items():
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
		
		for evar, args in libs.items():
			found = False
			for name, version in args['checks'].items():
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

	def ConfirmBoost(self, conf, version):
		conf.AddTests({'CheckBoost': CheckBoost})
		if conf.CheckBoost(version):
			return
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

	InstallProgram = lambda self, dir, source: GpickEnvironment.InstallPerm(self, dir, source, 0o755)
	InstallData = lambda self, dir, source: GpickEnvironment.InstallPerm(self, dir, source, 0o644)
	InstallDataAutoDir = lambda self, dir, relative_dir, source: GpickEnvironment.InstallPermAutoDir(self, dir, relative_dir, source, 0o644)

	def GetSourceFiles(self, dir_exclude_pattern, file_exclude_pattern):
		dir_exclude_prog = re.compile(dir_exclude_pattern)
		file_exclude_prog = re.compile(file_exclude_pattern)
		files = []
		MatchFiles(files, self.GetLaunchDir(), os.sep, dir_exclude_prog, file_exclude_prog)
		return files

	def GetVersionInfo(self):
		try:
			(version, revision, hash, date) = getVersionInfo()
		except:
			try:
				with open("../.version", "r", encoding = 'utf-8') as version_file:
					(version, revision, hash, date) = version_file.read().splitlines()
			except:
				print("Version file \".version\" is required when GIT can not be used to get version information.")
				self.Exit(1)
		self.Replace(
			GPICK_BUILD_VERSION = version,
			GPICK_BUILD_REVISION = revision,
			GPICK_BUILD_HASH = hash,
			GPICK_BUILD_DATE = date,
			GPICK_BUILD_VERSION_FULL = version + '-' + revision,
			GPICK_BUILD_VERSION_FULL_COMMA = re.sub(r'(\d+)[^\.]*', '\\1', version).replace('.', ',') + ',' + revision,
		);

def RegexEscape(str):
	return str.replace('\\', '\\\\')

def Glob(path):
	files = []
	for f in glob.glob(os.path.join(path, '*')):
		if os.path.isdir(str(f)):
			files.extend(Glob(str(f)));
		else:
			files.append(str(f));
	return files

