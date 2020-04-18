import re
from SCons.Script import Builder
from SCons.Action import Action
from SCons.Node.FS import File
def addTemplateBuilder(env):
	def buildEmitter(target, source, env):
		data = open(source[0].srcnode().get_path()).read()
		dict = env.Dictionary()
		keys = dict.keys()
		for key in keys:
			if isinstance(dict[key], str):
				env.Depends(target, env.Value(dict[key]))
		return target, source
	def buildFile(target, source, env):
		source_dest = str(target[0])
		wfile = open(source_dest, "w")
		data = open(source[0].srcnode().get_path()).read()
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
	env.Append(BUILDERS = {
		'Template': Builder(
			action = Action(buildFile, buildString),
			emitter = buildEmitter,
		),
	})
