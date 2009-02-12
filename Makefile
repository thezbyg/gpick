
prefix = /usr/local
bindir = $(prefix)/bin
datadir = $(prefix)/share
INSTALL = install -c
INSTALLDATA = install -c -m 644

INCLUDES = -I.
CC = gcc
CFLAGS = $(INCLUDES) -MD -MP -MG -MMD -O3 -Wall -c -fmessage-length=0 `pkg-config --cflags gtk+-2.0`
CPP = g++
CPPFLAGS = $(CFLAGS)
LD = g++
LDFLAGS = -s -mwindows -fno-rtti -fno-exceptions 
LDOBJECTS = `pkg-config --libs gtk+-2.0`
OBJDIR = obj/$(*F)

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
endif

PHONY: all
all: $(EXECUTABLE)

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
	$(INSTALL) bin/gpick $(bindir)/gpick
	$(INSTALLDATA) res/falloff-cubic.png -D $(datadir)/gpick/falloff-cubic.png
	$(INSTALLDATA) res/falloff-linear.png $(datadir)/gpick/falloff-linear.png
	$(INSTALLDATA) res/falloff-none.png $(datadir)/gpick/falloff-none.png
	$(INSTALLDATA) res/falloff-quadratic.png $(datadir)/gpick/falloff-quadratic.png
	$(INSTALLDATA) res/falloff-exponential.png $(datadir)/gpick/falloff-exponential.png
	$(INSTALLDATA) res/colors.txt $(datadir)/gpick/colors.txt
	$(INSTALLDATA) res/colors0.txt $(datadir)/gpick/colors0.txt
	$(INSTALLDATA) res/gpick.png $(datadir)/icons/hicolor/48x48/apps/gpick.png

.PHONY: clean
clean:
	\rm -f $(OBJECTS)
	\rm -f $(RESOURCES)
	\rm -f $(OBJECTS:%.o=%.d)
