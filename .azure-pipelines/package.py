import jinja2, subprocess, os, shlex, argparse

parser = argparse.ArgumentParser(description = "")
parser.add_argument('actions', type = str, nargs = "*")
parser.set_defaults(actions = ["deb"])
arguments = parser.parse_args()

control = """Package: gpick
Version: {{ version }}
Section: graphics
Priority: optional
Architecture: {{ architecture }}
Maintainer: Albertas Vyšniauskas <albertas.vysniauskas@gpick.org>
Depends: {{ depends }}
Installed-Size: {{ size }}
Homepage: http://www.gpick.org
Description: advanced GTK+ color picker
 Gpick is an advanced color picker used to pick colors from anywhere
 on the screen, mix them to  get new colors, generate shades and tints,
 and export palettes to common file formats or simply copy them
 to the clipboard.
"""

def run(command):
	return subprocess.run(shlex.split(command), capture_output = True, text = True, check = True).stdout.strip()

version = None
revision = None
with open(".version", "r", encoding = "utf-8") as file:
	version = file.readline().strip()
	revision = file.readline().strip()
fullVersion = version + "." + revision + "-1"

if 'version' in arguments.actions:
	print(fullVersion, end = "")

if 'deb' in arguments.actions:
	architecture = run("dpkg-architecture --query DEB_HOST_ARCH")
	name = "gpick_" + fullVersion + "_" + architecture

	os.makedirs(name + "/DEBIAN", exist_ok = True)
	run("make install DESTDIR=" + name)
	run("strip --strip-all " + name + "/usr/bin/gpick")

	os.makedirs("debian", exist_ok = True)
	with open("debian/control", "w") as file:
		pass

	env = jinja2.Environment(loader = jinja2.BaseLoader, keep_trailing_newline = True)
	template = env.from_string(control)
	with open(name + "/DEBIAN/control", "w", encoding = "utf-8") as file:
		file.write(template.render({
			"architecture": architecture,
			"depends": run("dpkg-shlibdeps -O " + name + "/usr/bin/gpick")[len("shlibs:Depends="):],
			"version": fullVersion,
			"size": run("du --summarize " + name + "/usr").split("\t")[0],
		}))

	run("dpkg-deb --build --root-owner-group " + name)
