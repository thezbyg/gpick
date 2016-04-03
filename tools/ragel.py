#!/usr/bin/env python
from SCons.Script import *
from SCons.Script.SConscript import SConsEnvironment
import SCons.Script.SConscript

def addRagelBuilder(env):
	RagelAction = SCons.Action.Action("$RAGELCOM", "$RAGELCOMSTR")
	env["RAGEL"] = env.Detect("ragel")
	env["RAGELCOM"] = "$RAGEL $RAGELFLAGS -o $TARGET $SOURCE"
	env["RAGELFLAGS"] = "-F1 -C"
	builder = Builder(
		action = RagelAction,
		suffix = '.cpp',
		src_suffix = '.rl',
		single_source = True)
	env.Append(BUILDERS = {'Ragel': builder})
