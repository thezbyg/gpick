from SCons.Script import *
from SCons.Tool.install import copyFunc

def addLemonBuilder(env):
	
	LemonAction = SCons.Action.Action("$LEMONCOM", "$LEMONCOMSTR")
	
	env["LEMON"]      = env.Detect("lemon")
	env["LEMONCOM"] = "cd build/; $LEMON $SOURCE"
	
	def headerEmitter(target, source, env): 
		bs = SCons.Util.splitext(str(source[0].name))[0] 
		target.append(bs + '.h')
		target.append(bs + '.c')
		bs = SCons.Util.splitext(str(target[0]))[0] + ".y"
		source.append(bs) 	
		return (target, source)
		
	def buildAction(target, source, env):
		source_dest = SCons.Util.splitext(str(target[0]))[0] + ".y"
		Execute(Copy('build/lempar.c', 'extern/lempar.c'))
		Execute(Copy(source_dest, File(source[0]).srcnode()))
		LemonAction.execute([target], ['../' + source_dest], env);
		Execute(Delete(source_dest))
		return 0
	
	builder = Builder(
		action = buildAction,
		suffix = '.c',
		src_suffix = '.y',
		emitter = headerEmitter)
		
	env.Append(BUILDERS = {'Lemon': builder})
