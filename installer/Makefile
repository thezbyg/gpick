
VERSION_FULL=$(shell python -c 'import getversion; getversion.getversion("full")')
VERSION=$(shell python -c 'import getversion; getversion.getversion("ms")')
MSI=gpick_$(VERSION_FULL)

HEAT_SRCS=gpick_files.xstl gtk_files.xstl
SRCS=gpick.wxs $(HEAT_SRCS:.xstl=.wxs)
OBJS=$(SRCS:.wxs=.wixobj)
CANDLE=candle.exe
LIGHT=light.exe
HEAT=heat.exe

HEAT_FLAGS=-ag -dr INSTALLDIR -suid -srd -var var.FilePath
FLAGS=-dFilePath=files -dVersionFull=$(VERSION_FULL) -dVersion=$(VERSION)
LIGHT_FLAGS=-ext WixUtilExtension -ext WixUIExtension $(FLAGS)
CANDLE_FLAGS=$(FLAGS)

DEPFILES= files/Gpick.exe

.SUFFIXES: .wxs .wixobj .xstl

all: $(MSI).msi
  
$(MSI).msi: $(OBJS) $(DEPFILES) $(HEAT_SRCS)
	$(LIGHT) -nologo $(LIGHT_FLAGS) -out $@ $(OBJS)

.xstl.wxs:
	$(HEAT) dir "files" -nologo $(HEAT_FLAGS) -cg C.Files.$(<:.xstl=) -t $< -out $@

.wxs.wixobj:
	$(CANDLE) -nologo $(CANDLE_FLAGS) -out $@ $^

clean:
	$(RM) $(MSI).msi $(OBJS)

install: $(MSI).msi
	$(RM) install.log
	msiexec -i $(MSI).msi -l*v install.log

uninstall:
	msiexec -x $(MSI).msi

