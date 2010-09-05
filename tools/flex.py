from SCons.Script import *

def addFlexBuilder(env):
	
	def headerEmitter(target, source, env): 
		bs = SCons.Util.splitext(str(source[0].name))[0] 
		target.append(bs + '.h') 
		return (target, source) 
	
	builder = Builder(action = 'flex --header-file=${TARGET.base}.h -o $TARGET $SOURCE',
		suffix = '.cpp',
		src_suffix = '.l',
		emitter = headerEmitter)

	env.Append(BUILDERS = {'Flex': builder})
