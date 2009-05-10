
ifndef prefix
	prefix = /usr/local
endif
bindir = $(prefix)/bin
datadir = $(prefix)/share
INSTALL = install -c -m 755
INSTALLDATA = install -c -m 644

ifndef LUAPC
LUAPC = lua5.1
endif

INCLUDES = -I.
CC = gcc
CFLAGS = $(INCLUDES) -MD -MP -MG -MMD -O3 -Wall -c -fmessage-length=0 `pkg-config --cflags gtk+-2.0 --cflags lua5.1` \
-DG_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGLIB_PIXBUF_DISABLE_DEPRECATED

CPP = g++
CPPFLAGS = $(CFLAGS)
LD = g++
LDFLAGS = $(USR_LDFLAGS) -s -fno-rtti -fno-exceptions 
LDOBJECTS = `pkg-config --libs gtk+-2.0 --libs $(LUAPC)`
OBJDIR = obj/$(*F)

MKDIR = mkdir
MAKEDIRS = bin obj obj/res obj/dynv obj/gtk

SOURCES=Color.cpp \
ColorNames.cpp \
main.cpp \
MathUtil.cpp \
Sampler.cpp \
uiColorComponent.cpp \
uiDialogVariations.cpp \
uiDialogGenerate.cpp \
uiDialogMix.cpp \
uiExport.cpp \
uiListPalette.cpp \
uiUtilities.cpp \
Random.cpp \
LuaSystem.cpp \
LuaExt.cpp \
ColorObject.cpp \
FileFormat.cpp \
uiConverter.cpp

-include dynv/subdir.mk
-include gtk/subdir.mk

EXECUTABLE = bin/gpick
OBJECTS = $(SOURCES:%.cpp=$(OBJDIR)%.o)
RESOURCES =

ifeq ($(strip $(OS)),Windows_NT)
	RES = windres
	RESOURCE_FILES = res/resources.rc
	RESOURCES = $(RESOURCE_FILES:%.rc=$(OBJDIR)%.o)
	LDFLAGS += -mwindows
endif

PHONY: all
all: $(MAKEDIRS) $(EXECUTABLE)

$(MAKEDIRS):
	$(MKDIR) $@

$(EXECUTABLE): $(OBJECTS) $(RESOURCES)
	$(LD) $(LDFLAGS) $(OBJECTS) $(RESOURCES) $(LDOBJECTS) -o $@

$(OBJDIR)%.o: %.rc
	$(RES) $< $@
	
-include $(OBJECTS:.o=.d)

$(OBJDIR)%.o: %.cpp
	$(CPP) $(CPPFLAGS) -o $@ $< 

$(OBJDIR)%.o: %.c
	$(CC) $(CFLAGS) -o $@ $< 
     
.PHONY: install
install: all
	$(INSTALL) bin/gpick -D $(DESTDIR)$(bindir)/gpick
	$(INSTALLDATA) share/applications/gpick.desktop -D $(DESTDIR)$(datadir)/applications/gpick.desktop
	$(INSTALLDATA) share/doc/gpick/copyright -D $(DESTDIR)$(datadir)/doc/gpick/copyright
	$(INSTALLDATA) share/gpick/init.lua -D $(DESTDIR)$(datadir)/gpick/init.lua
	$(INSTALLDATA) share/gpick/colors.txt -D $(DESTDIR)$(datadir)/gpick/colors.txt
	$(INSTALLDATA) share/gpick/colors0.txt -D $(DESTDIR)$(datadir)/gpick/colors0.txt
	$(INSTALLDATA) share/gpick/falloff-cubic.png -D $(DESTDIR)$(datadir)/gpick/falloff-cubic.png
	$(INSTALLDATA) share/gpick/falloff-linear.png -D $(DESTDIR)$(datadir)/gpick/falloff-linear.png
	$(INSTALLDATA) share/gpick/falloff-none.png -D $(DESTDIR)$(datadir)/gpick/falloff-none.png
	$(INSTALLDATA) share/gpick/falloff-quadratic.png -D $(DESTDIR)$(datadir)/gpick/falloff-quadratic.png
	$(INSTALLDATA) share/gpick/falloff-exponential.png -D $(DESTDIR)$(datadir)/gpick/falloff-exponential.png
	$(INSTALLDATA) share/icons/hicolor/48x48/apps/gpick.png -D $(DESTDIR)$(datadir)/icons/hicolor/48x48/apps/gpick.png
	$(INSTALLDATA) share/icons/hicolor/scalable/apps/gpick.svg -D $(DESTDIR)$(datadir)/icons/hicolor/scalable/apps/gpick.svg
	$(INSTALLDATA) share/man/man1/gpick.1 -D $(DESTDIR)$(datadir)/man/man1/gpick.1

.PHONY: clean
clean:
	\rm -f $(OBJECTS)
	\rm -f $(RESOURCES)
	\rm -f $(OBJECTS:%.o=%.d)
