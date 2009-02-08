
!include "MUI2.nsh"



!define VERSION "0.0.17"
Name "GPick"
OutFile "gpick-win32-${VERSION}-setup.exe"
Caption "GPick v${VERSION} Setup"
SetCompressor /SOLID lzma
SetCompressorDictSize 32
InstallDir $PROGRAMFILES\GPick
InstallDirRegKey HKLM "Software\GPick" ""
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

	SetOutPath "$INSTDIR\bin"
	
	File ..\bin\gpick.exe
	
	SetOutPath "$INSTDIR\res"
	
	File ..\res\colors0.txt
	File ..\res\colors.txt
	File ..\res\falloff-none.png
	File ..\res\falloff-linear.png
	File ..\res\falloff-quadratic.png
	File ..\res\falloff-cubic.png
	File ..\res\falloff-exponential.png
	
	SetOutPath "$INSTDIR"
	WriteRegStr HKLM "Software\GPick" "" $INSTDIR
	WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Desktop Shortcut" SecDesktopShortcut
  SetOutPath "$INSTDIR"
  CreateShortCut "$DESKTOP\GPick.lnk" "$INSTDIR\bin\gpick.exe" ""
SectionEnd

Section "-Start Menu Shortcut" SecStartMenu

  SetOutPath "$INSTDIR\bin"
  
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
    
    CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\GPick.lnk" "$INSTDIR\bin\gpick.exe"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  
  !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd



;Uninstaller Section

Section "Uninstall"


Delete "$INSTDIR\Uninstall.exe"

Delete "$INSTDIR\bin\gpick.exe"
RMDir "$INSTDIR\bin"

Delete "$INSTDIR\res\colors0.txt"
Delete "$INSTDIR\res\colors.txt"
Delete "$INSTDIR\res\falloff-none.png"
Delete "$INSTDIR\res\falloff-linear.png"
Delete "$INSTDIR\res\falloff-quadratic.png"
Delete "$INSTDIR\res\falloff-cubic.png"
Delete "$INSTDIR\res\falloff-exponential.png"
RMDir "$INSTDIR\res"

RMDir "$INSTDIR"
  

  !insertmacro MUI_STARTMENU_GETFOLDER Application $MUI_TEMP
  
  Delete "$SMPROGRAMS\$MUI_TEMP\GPick.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\Uninstall.lnk"
  
  Delete "$DESKTOP\GPick.lnk"
  
  ;Delete empty start menu parent diretories
  StrCpy $MUI_TEMP "$SMPROGRAMS\$MUI_TEMP"
 
  startMenuDeleteLoop:
	ClearErrors
    RMDir $MUI_TEMP
    GetFullPathName $MUI_TEMP "$MUI_TEMP\.."
    
    IfErrors startMenuDeleteLoopDone
  
    StrCmp $MUI_TEMP $SMPROGRAMS startMenuDeleteLoopDone startMenuDeleteLoop
  startMenuDeleteLoopDone:

  DeleteRegKey /ifempty HKLM "Software\GPick"

SectionEnd