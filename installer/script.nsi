
!include "MUI2.nsh"
!include ..\build\version.nsi

Name "gpick"
OutFile "gpick_${VERSION}_win32-setup.exe"
Caption "gpick v${VERSION} Setup"
SetCompressor /SOLID lzma
SetCompressorDictSize 32
InstallDir $PROGRAMFILES\gpick
InstallDirRegKey HKLM "Software\gpick" ""
RequestExecutionLevel admin

Var MUI_TEMP
Var STARTMENU_FOLDER
  
!define MUI_WELCOMEPAGE  
!define MUI_LICENSEPAGE
!define MUI_DIRECTORYPAGE
!define MUI_ABORTWARNING
!define MUI_UNINSTALLER
!define MUI_UNCONFIRMPAGE
!define MUI_FINISHPAGE  

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "License.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_STARTMENU Application $STARTMENU_FOLDER
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"





AllowSkipFiles off

Section "!Program Files" SecProgramFiles
	SectionIn RO

	SetOutPath "$INSTDIR"
	
	File ..\build\source\gpick.exe
	
	SetOutPath "$INSTDIR\share\gpick"
	
	File ..\share\gpick\colors0.txt
	File ..\share\gpick\colors.txt
	File ..\share\gpick\gpick-falloff-none.png
	File ..\share\gpick\gpick-falloff-linear.png
	File ..\share\gpick\gpick-falloff-quadratic.png
	File ..\share\gpick\gpick-falloff-cubic.png
	File ..\share\gpick\gpick-falloff-exponential.png
	File ..\share\icons\hicolor\48x48\apps\gpick.png
	File ..\share\gpick\init.lua
	File ..\share\gpick\helpers.lua
	
	SetOutPath "$INSTDIR"
	WriteRegStr HKLM "Software\gpick" "" $INSTDIR
	WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Desktop Shortcut" SecDesktopShortcut
  SetOutPath "$INSTDIR"
  CreateShortCut "$DESKTOP\gpick.lnk" "$INSTDIR\gpick.exe" ""
SectionEnd

Section "-Start Menu Shortcut" SecStartMenu

  SetOutPath "$INSTDIR"
  
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
    
    CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\gpick.lnk" "$INSTDIR\gpick.exe"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  
  !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd



;Uninstaller Section

Section "Uninstall"


Delete "$INSTDIR\Uninstall.exe"

Delete "$INSTDIR\gpick.exe"

Delete "$INSTDIR\share\gpick\colors0.txt"
Delete "$INSTDIR\share\gpick\colors.txt"
Delete "$INSTDIR\share\gpick\gpick-falloff-none.png"
Delete "$INSTDIR\share\gpick\gpick-falloff-linear.png"
Delete "$INSTDIR\share\gpick\gpick-falloff-quadratic.png"
Delete "$INSTDIR\share\gpick\gpick-falloff-cubic.png"
Delete "$INSTDIR\share\gpick\gpick-falloff-exponential.png"
Delete "$INSTDIR\share\gpick\gpick.png"
Delete "$INSTDIR\share\gpick\init.lua"
Delete "$INSTDIR\share\gpick\helpers.lua"
RMDir "$INSTDIR\share\gpick"
RMDir "$INSTDIR\share"

RMDir "$INSTDIR"
  

  !insertmacro MUI_STARTMENU_GETFOLDER Application $MUI_TEMP
  
  Delete "$SMPROGRAMS\$MUI_TEMP\gpick.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\Uninstall.lnk"
  
  Delete "$DESKTOP\gpick.lnk"
  
  ;Delete empty start menu parent diretories
  StrCpy $MUI_TEMP "$SMPROGRAMS\$MUI_TEMP"
 
  startMenuDeleteLoop:
	ClearErrors
    RMDir $MUI_TEMP
    GetFullPathName $MUI_TEMP "$MUI_TEMP\.."
    
    IfErrors startMenuDeleteLoopDone
  
    StrCmp $MUI_TEMP $SMPROGRAMS startMenuDeleteLoopDone startMenuDeleteLoop
  startMenuDeleteLoopDone:

  DeleteRegKey /ifempty HKLM "Software\gpick"

SectionEnd