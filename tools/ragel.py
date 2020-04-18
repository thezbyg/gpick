from SCons.Script import Builder
from SCons.Action import Action
def addRagelBuilder(env):
	env.Append(BUILDERS = {
		'Ragel': Builder(
			action = Action("$RAGELCOM", "$RAGELCOMSTR"),
			suffix = '.cpp',
			src_suffix = '.rl',
			single_source = True,
		),
	})
	env["RAGEL"] = env.Detect("ragel")
	env["RAGELCOM"] = "$RAGEL $RAGELFLAGS -o $TARGET $SOURCE"
	env["RAGELFLAGS"] = "-F1 -C"
