import re
from SCons.Script import Builder
from SCons.Action import Action
def addResourceTemplateBuilder(env):
	def buildResourceFile(target, source, env):
		source_dest = SCons.Util.splitext(str(target[0]))[0] + ".rc"
		wfile = open(source_dest,"w")
		data = open(str(File(source[0]).srcnode())).read()
		for key, var in env['RESOURCE_TEMPLATE_VARS'].iteritems():
			data = re.sub("%" + key + "%", var, data)
		wfile.write(data)
		wfile.close()
		return 0
	def buildResourceFileString(target, source, env):
		return "Preparing resource file %s" % os.path.basename(str(target[0]))
	env.Append(BUILDERS = {
		'ResourceTemplate': Builder(
			action = Action(buildResourceFile, buildResourceFileString),
			suffix = '.rc',
			src_suffix = '.rct',
		),
	})
