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

	MsgmergeAction = SCons.Action.Action("$MSGMERGECOM", "$MSGMERGECOMSTR")
	
	env["MSGMERGE"]    = env.Detect("msgmerge")
	env["MSGMERGECOM"] = "$MSGMERGE $MSGMERGE_FLAGS --output-file=$TARGET $SOURCES"
	
	builder = Builder(
		action = MsgmergeAction,
		suffix = '.pot',
		src_suffix = '.pot',
		single_source = False)

	env.Append(BUILDERS = {'Msgmerge': builder})

	MsgcatAction = SCons.Action.Action("$MSGCATCOM", "$MSGCATCOMSTR")
	
	env["MSGCAT"]    = env.Detect("msgcat")
	env["MSGCATCOM"] = "$MSGCAT $MSGCAT_FLAGS --output-file=$TARGET $SOURCES"
	
	builder = Builder(
		action = MsgcatAction,
		suffix = '.pot',
		src_suffix = '.pot',
		single_source = False)

	env.Append(BUILDERS = {'Msgcat': builder})

