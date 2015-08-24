# What is Gpick?

[Gpick](http://www.gpick.org) is an advanced color picker 
and palette editing tool. You can use it to pick colors 
from anywhere on the screen, mix colors, create variations 
of hue and lightness, build harmonic color palettes etc.

# Documentation

You can find a reference to Gpick's features on the 
[project's website](http://www.gpick.org/help/).

# Building

To build Gpick on Linux, run:

	$ scons
	$ sudo scons install

To remove an installed copy of Gpick, run:

	$ sudo scons -c install

To rebuild and install Gpick from Git, run:

	$ sudo scons -c install
	$ scons -c
	$ git pull --rebase
	$ scons
	$ sudo scons install

