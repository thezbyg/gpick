
INCLUDES = -I.
CC = gcc
CFLAGS = $(INCLUDES) -O3 -Wall -c -fmessage-length=0 `pkg-config --cflags gtk+-2.0`
CPP = g++
CPPFLAGS = $(CFLAGS)
LD = g++
LDFLAGS = -s -mwindows -fno-rtti -fno-exceptions 
LDOBJECTS = `pkg-config --libs gtk+-2.0`
RES = windres

SOURCES=Color.cpp ColorNames.cpp main.cpp MathUtil.cpp Sampler.cpp \
uiColorComponent.cpp uiDialogShades.cpp uiListPalette.cpp \
uiSwatch.cpp uiUtilities.cpp uiZoomed.cpp

RESOURCES = 

OBJECTS = $(SOURCES:.cpp=.o) $(RESOURCES:.rc=.o)
EXECUTABLE = bin/gpick

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) $(LDOBJECTS) -o $@

.cpp.o:
	$(CPP) $(CPPFLAGS) -o $@ $< 
	
.c.o:
	$(CC) $(CFLAGS) -o $@ $< 

resources.o: res/resources.rc
	$(RES) $< $@


.PHONY: clean
clean:
	\rm *.o
     
