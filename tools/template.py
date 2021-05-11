import re, os, fnmatch
from SCons.Script import Builder
from SCons.Action import Action

def addTemplateBuilder(env):
	def filterKeys(keys, env):
		if not ('TEMPLATE_ENV_FILTER' in env):
			return keys
		filters = env['TEMPLATE_ENV_FILTER']
		matched = []
		for filter in filters:
			for name in keys:
				if fnmatch.fnmatch(name, filter):
					matched.append(name)
		return list(dict.fromkeys(matched))
	def buildEmitter(target, source, env):
		dict = env.Dictionary()
		keys = filterKeys(dict.keys(), env)
		for key in keys:
			if isinstance(dict[key], str):
				env.Depends(target, env.Value(dict[key]))
		return target, source
	def buildFile(target, source, env):
		source_dest = str(target[0])
		wfile = open(source_dest, 'w', encoding = 'utf-8')
		dict = env.Dictionary()
		keys = filterKeys(dict.keys(), env)
		if len(source) == 0:
			data = env['TEMPLATE_SOURCE']
		else:
			data = open(source[0].srcnode().get_path(), 'r').read()
		for key in keys:
			if isinstance(dict[key], str):
				data = re.sub('@' + key + '@', dict[key], data)
		wfile.write(data)
		wfile.close()
		return 0
	def buildString(target, source, env):
		return 'Preparing file %s' % os.path.basename(str(target[0]))
	env.Append(BUILDERS = {
		'Template': Builder(
			action = Action(buildFile, buildString),
			emitter = buildEmitter,
		),
	})
