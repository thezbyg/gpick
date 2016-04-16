from SCons.Script import *
import re
def addTemplateBuilder(env):
	def buildFile(target, source, env):
		source_dest = str(target[0])
		wfile = open(source_dest,"w")
		data = open(str(File(source[0]).srcnode())).read()
		dict = env.Dictionary()
		keys = dict.keys()
		for key in keys:
			if isinstance(dict[key], str):
				data = re.sub("%" + key + "%", dict[key], data)
		wfile.write(data)
		wfile.close()
		return 0
	def buildString(target, source, env):
		return "Preparing file %s" % os.path.basename(str(target[0]))
	builder = Builder(
		action = SCons.Action.Action(buildFile, buildString),
	)
	env.Append(BUILDERS = {'Template': builder})
