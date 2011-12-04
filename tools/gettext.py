from SCons.Script import *

def addGettextBuilder(env):

	GettextAction = SCons.Action.Action("$GETTEXTCOM", "$GETTEXTCOMSTR")
	
	env["GETTEXT"]    = env.Detect("msgfmt")
	env["GETTEXTCOM"] = "$GETTEXT --check-format --check-domain -f -o $TARGET $SOURCE"
	
	builder = Builder(
		action = GettextAction,
		suffix = '.mo',
		src_suffix = '.po',
		single_source = True)

	env.Append(BUILDERS = {'Gettext': builder})

