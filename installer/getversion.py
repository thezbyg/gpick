import os, re
parentdir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def getversion(style):
	with open(os.path.join(parentdir, ".version"), "r") as file:
		lines = file.read().splitlines()
		version = lines[0]
		revision = lines[1]
	if style == "full":
		print(version + '-' + revision)
	else:
		print(re.sub(r'(\d+)[^\.]*', '\\1', version) + '.' + revision)
