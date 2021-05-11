import time, re, subprocess

def getVersionInfo():
	result = subprocess.Popen(['git', 'describe', '--match=gpick-*', '--match=v*', '--always', '--long'], shell = False, stdout = subprocess.PIPE, stderr = subprocess.STDOUT).communicate()[0]
	match = re.search(r'^(?:gpick-|v)(\d+\.[\w]+(\.[\w]+)?)(?:-([\d]+))?', str(result, 'utf-8'))
	version = match.group(1)
	revision = match.group(3)
	result = subprocess.Popen(['git', 'show', '--no-patch', '--format="%H %ct"'], shell = False, stdout = subprocess.PIPE, stderr=subprocess.STDOUT).communicate()[0]
	match = re.search('([\d\w]+) (\d+)', str(result, 'utf-8'))
	hash = match.group(1)
	date = time.strftime('%Y-%m-%d', time.gmtime(int(match.group(2))))
	return (version, revision, hash, date)

def writeVersionFile():
	version = getVersionInfo()
	with open('.version', 'w', encoding = 'utf-8') as file:
		file.write('\n'.join(version))
