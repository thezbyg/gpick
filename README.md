# Gpick - advanced color picker and palette editor.

<img src="../../wiki/images/logo.png" align="right" alt="Gpick logo" width="150" height="150" />

[![Build Status](https://dev.azure.com/thezbyg/Gpick/_apis/build/status/thezbyg.gpick?branchName=master)](https://dev.azure.com/thezbyg/Gpick/_build/latest?definitionId=1&branchName=master) [![Nightly packages](../../wiki/images/nightly.svg)](https://dev.azure.com/thezbyg/Gpick/_build/latest?definitionId=4&branchName=master)

Gpick is an application that allows you to sample any color from anywhere on the desktop, and use it to create palettes (i.e. collections of colors) for use in graphic design applications. Gpick also has other features that help in the creation of color palettes, such as:

* The ability to create a palette from an imported image
* Automatic naming of colors
* Color scheme generator
* Import and export from various file formats

<p align="center">
<img src="../../wiki/images/readme-screenshot.png" alt="Gpick screenshot" width="500" />
</p>

## Building from source



### Compiler

C++17 support is required. Compilation is currently only tested on following compilers:

 * GCC 9.3 and 10.2.
 * Clang 11.0.

### Build dependencies

CMake 3.12 or newer: build process management application ([https://cmake.org/](https://cmake.org/)).

SCons 3.0 or newer: a software construction tool ([http://www.scons.org](http://www.scons.org)).

Either CMake or SCons can be used. Package maintainers should use CMake, because SCons support is deprecated and will be removed at some point in the future.

Ragel 6.9 or newer: state machine compiler ([http://www.colm.net/open-source/ragel](http://www.colm.net/open-source/ragel)).

### Dependencies

GTK+ 3.0 ([http://www.gtk.org](http://www.gtk.org)).

GTK+ 2.24 ([http://www.gtk.org](http://www.gtk.org)).

Either GTK+ 3.x or GTK+ 2.x can be used.

Lua 5.4, 5.3 or 5.2 ([http://www.lua.org](http://www.lua.org)).

Expat ([http://expat.sourceforge.net](http://expat.sourceforge.net)).

Boost 1.71 or newer ([http://www.boost.org](http://www.boost.org)).
Used libraries:

 * Interprocess.
 * Test (only when building/running tests).

### Optional dependencies

gettext ([http://www.gnu.org/s/gettext](http://www.gnu.org/s/gettext)). Required if ENABLE\_NLS is enabled. Required by default.

### Building

#### Using CMake:

`mkdir build && cd build` to create out-of-source build directory.

`cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local` to prepare build files for installation to `/usr/local`.

`make` to compile all files.

`make install` to install executable and resources to `DESTDIR`. Default `DESTDIR` value is set by `CMAKE_INSTALL_PREFIX` variable.

#### Using SCons:

`scons` to compile all files and place executable file in `build/`.

`scons install` to install executable and resources to `DESTDIR`. By default `DESTDIR` is `/usr/local`.

### Build options

ENABLE\_NLS - compile with gettext support. Enabled by default.

USE\_GTK3 - use GTK3 instead of GTK2. Enabled by default.

PREFER\_VERSION\_FILE - read version information from file instead of using GIT. Disabled by default. This option enables unconditional `.version` file usage. `.version` file is included in release source archives and is a simple text file containing the following information in four lines: major/minor version, revision, commit hash and commit date.
