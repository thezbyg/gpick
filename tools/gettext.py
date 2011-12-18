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

	XgettextAction = SCons.Action.Action("$XGETTEXTCOM", "$XGETTEXTCOMSTR")
	
	env["XGETTEXT"]    = env.Detect("xgettext")
	env["XGETTEXTCOM"] = "$XGETTEXT --keyword=_ --from-code utf8 --package-name=gpick $XGETTEXT_FLAGS --output=$TARGET $SOURCES"
	
	builder = Builder(
		action = XgettextAction,
		suffix = '.pot',
		src_suffix = '.cpp',
		single_source = False)

	env.Append(BUILDERS = {'Xgettext': builder})
