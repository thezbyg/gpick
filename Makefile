
ifndef prefix
	prefix = /usr/local
endif
bindir = $(prefix)/bin
datadir = $(prefix)/share
INSTALL = install -c -m 755
INSTALLDATA = install -c -m 644

INCLUDES = -I.
CC = gcc
CFLAGS = $(INCLUDES) -MD -MP -MG -MMD -O3 -Wall -c -fmessage-length=0 `pkg-config --cflags gtk+-2.0`
CPP = g++
CPPFLAGS = $(CFLAGS)
LD = g++
LDFLAGS = $(USR_LDFLAGS) -s -fno-rtti -fno-exceptions 
LDOBJECTS = `pkg-config --libs gtk+-2.0`
OBJDIR = obj/$(*F)

MKDIR = mkdir
MAKEDIRS = bin obj

SOURCES=Color.cpp ColorNames.cpp main.cpp MathUtil.cpp Sampler.cpp \
uiColorComponent.cpp uiDialogVariations.cpp uiListPalette.cpp \
uiSwatch.cpp uiUtilities.cpp uiZoomed.cpp uiDialogMix.cpp \
uiExport.cpp uiDialogGenerate.cpp Random.cpp

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
	$(INSTALL) bin/gpick $(DESTDIR)$(bindir)/gpick
	$(INSTALLDATA) res/falloff-cubic.png -D $(DESTDIR)$(datadir)/gpick/falloff-cubic.png
	$(INSTALLDATA) res/falloff-linear.png $(DESTDIR)$(datadir)/gpick/falloff-linear.png
	$(INSTALLDATA) res/falloff-none.png $(DESTDIR)$(datadir)/gpick/falloff-none.png
	$(INSTALLDATA) res/falloff-quadratic.png $(DESTDIR)$(datadir)/gpick/falloff-quadratic.png
	$(INSTALLDATA) res/falloff-exponential.png $(DESTDIR)$(datadir)/gpick/falloff-exponential.png
	$(INSTALLDATA) res/colors.txt $(DESTDIR)$(datadir)/gpick/colors.txt
	$(INSTALLDATA) res/colors0.txt $(DESTDIR)$(datadir)/gpick/colors0.txt
	$(INSTALLDATA) res/gpick.png $(DESTDIR)$(datadir)/icons/hicolor/48x48/apps/gpick.png

.PHONY: clean
clean:
	\rm -f $(OBJECTS)
	\rm -f $(RESOURCES)
	\rm -f $(OBJECTS:%.o=%.d)
