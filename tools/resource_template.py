from SCons.Script import *
from SCons.Tool.install import copyFunc

import re

def addResourceTemplateBuilder(env):
	
	def buildResourceFile(target, source, env):
		source_dest = SCons.Util.splitext(str(target[0]))[0] + ".rc"
		wfile = open(source_dest,"w")
		data = open(str(File(source[0]).srcnode())).read()
		for var in env['RESOURCE_TEMPLATE_VARS']:
			m = re.search('([^=]+)=(.+)', var)
			data = re.sub("%" + m.group(1) + "%", m.group(2), data)
		wfile.write(data)
		wfile.close()
		return 0

	def buildResourceFileString(target, source, env):
		return "Preparing resource file %s" % os.path.basename(str(target[0]))

	builder = Builder(
		action = SCons.Action.Action(buildResourceFile, buildResourceFileString),
		suffix = '.rc',
		src_suffix = '.rct',
		)
		
	env.Append(BUILDERS = {'ResourceTemplate': builder})

