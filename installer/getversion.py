import os,sys,re
parentdir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0,parentdir) 
import version

def getversion(style):
	if style == "full":
		print version.GPICK_BUILD_VERSION
	else:
		result = re.match(r"^(\d+\.\d+\.\d+)(.*)$", version.GPICK_BUILD_VERSION)
		if result:
			print result.group(1) 
		else:
			print "invalid_version"
