from SCons.Script import Builder
from SCons.Action import Action
def addGettextBuilder(env):
	env.Append(BUILDERS = {
		'Gettext': Builder(
			action = Action("$GETTEXTCOM", "$GETTEXTCOMSTR"),
			suffix = '.mo',
			src_suffix = '.po',
			single_source = True,
		),
		'Xgettext': Builder(
			action = Action("$XGETTEXTCOM", "$XGETTEXTCOMSTR"),
			suffix = '.pot',
			src_suffix = '.cpp',
			single_source = False,
		),
		'Msgmerge': Builder(
			action = Action("$MSGMERGECOM", "$MSGMERGECOMSTR"),
			suffix = '.pot',
			src_suffix = '.pot',
			single_source = False,
		),
		'Msgcat': Builder(
			action = Action("$MSGCATCOM", "$MSGCATCOMSTR"),
			suffix = '.pot',
			src_suffix = '.pot',
			single_source = False,
		),
	})
	env["GETTEXT"] = env.Detect("msgfmt")
	env["GETTEXTCOM"] = "$GETTEXT --check-format --check-domain -f -o $TARGET $SOURCE"
	env["XGETTEXT"] = env.Detect("xgettext")
	env["XGETTEXTCOM"] = "$XGETTEXT --keyword=_ --from-code utf-8 --package-name=gpick $XGETTEXT_FLAGS --output=$TARGET $SOURCES"
	env["MSGMERGE"] = env.Detect("msgmerge")
	env["MSGMERGECOM"] = "$MSGMERGE $MSGMERGE_FLAGS --output-file=$TARGET $SOURCES"
	env["MSGCAT"] = env.Detect("msgcat")
	env["MSGCATCOM"] = "$MSGCAT $MSGCAT_FLAGS --output-file=$TARGET $SOURCES"

