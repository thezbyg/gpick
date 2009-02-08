
INCLUDES = -I.
CC = gcc
CFLAGS = $(INCLUDES) -O3 -Wall -c -fmessage-length=0 `pkg-config --cflags gtk+-2.0`
CPP = g++
CPPFLAGS = $(CFLAGS)
LD = g++
LDFLAGS = -s -mwindows -fno-rtti -fno-exceptions 
LDOBJECTS = `pkg-config --libs gtk+-2.0`

SOURCES=Color.cpp ColorNames.cpp main.cpp MathUtil.cpp Sampler.cpp \
uiColorComponent.cpp uiDialogVariations.cpp uiListPalette.cpp \
uiSwatch.cpp uiUtilities.cpp uiZoomed.cpp uiDialogMix.cpp \
uiExport.cpp

EXECUTABLE = bin/gpick
OBJECTS = $(SOURCES:.cpp=.o)
RESOURCES =

ifeq ($(strip $(OS)),Windows_NT)
	RES = windres
	RESOURCE_FILES = res/resources.rc
	RESOURCES = $(RESOURCE_FILES:.rc=.o)
endif


all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) $(RESOURCES)
	$(LD) $(LDFLAGS) $(OBJECTS) $(RESOURCES) $(LDOBJECTS) -o $@

%.o: %.rc
	$(RES) $< $@

.cpp.o:
	$(CPP) $(CPPFLAGS) -o $@ $< 
	
.c.o:
	$(CC) $(CFLAGS) -o $@ $< 

clean:
	\rm -f $(OBJECTS)
	\rm -f $(RESOURCES)
     
.PHONY: all clean
