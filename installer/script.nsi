
!include "MUI2.nsh"
!include "FileFunc.nsh"  ; GetOptions
!include ..\build\version.nsi


!define PRODUCT_VERSION "${VERSION}"
!define PRODUCT_NAME "Gpick"
!define PRODUCT_NAME_SMALL "gpick"
!define PRODUCT_PUBLISHER "Albertas Vyðniauskas"
!define PRODUCT_WEB_SITE "http://code.google.com/p/gpick/"

!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

!define REGISTRY_APP_PATHS "Software\Microsoft\Windows\CurrentVersion\App Paths"


Name "Gpick"
OutFile "Gpick_${VERSION}_win32-setup.exe"
Caption "Gpick v${VERSION} Setup"
SetCompressor /SOLID lzma
SetCompressorDictSize 32
InstallDir $PROGRAMFILES\Gpick
InstallDirRegKey HKLM "Software\Gpick" ""
RequestExecutionLevel admin

; gtk installer name for embedding
!define GTK_INSTALLER_EXE "gtk2-runtime-2.24.10-2012-10-10-ash.exe"

Var MUI_TEMP
Var STARTMENU_FOLDER
var install_option_removeold  ; uninstall the old version first (if present): yes (default), no.
var gtk_mode  ; "public", "private" or "none"
var gtk_tmp  ; temporary variable
  
!define MUI_WELCOMEPAGE  
!define MUI_LICENSEPAGE
!define MUI_DIRECTORYPAGE
!define MUI_ABORTWARNING
!define MUI_UNINSTALLER
!define MUI_UNCONFIRMPAGE
!define MUI_FINISHPAGE  

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "License.txt"
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE on_components_page_leave
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_STARTMENU Application $STARTMENU_FOLDER
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_FINISHPAGE_NOREBOOTSUPPORT
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"


AllowSkipFiles off

var gtk_dll_abs_path

Section "!Program Files" SecProgramFiles
	SectionIn RO

	SetOutPath "$INSTDIR"
	
	File ..\build\source\gpick.exe
	File lua5.2.dll
		
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
	File ..\share\gpick\layouts.lua
	
	SetOutPath "$INSTDIR"
	WriteRegStr HKLM "Software\Gpick" "" $INSTDIR
	
	SetShellVarContext all  ; use all user variables as opposed to current user

	; Don't set any paths for this exe if it has a private GTK+ installation.
!ifndef NO_GTK
	StrCmp $gtk_mode "private" skip_exe_PATH
!endif
	; set a special path for this exe, as GTK may not be in a global path.
	ReadRegStr $gtk_dll_abs_path HKLM "SOFTWARE\GTK\2.0" "DllPath"
	WriteRegStr HKLM "${REGISTRY_APP_PATHS}\gpick.exe" "Path" "$gtk_dll_abs_path"
!ifndef NO_GTK
	skip_exe_PATH:
!endif

!ifndef NO_GTK
	WriteRegStr HKLM "SOFTWARE\${PRODUCT_NAME}" "GtkInstalledMode" "$gtk_mode"
!endif

	WriteRegStr HKLM "SOFTWARE\${PRODUCT_NAME}" "InstallationDirectory" "$INSTDIR"
	WriteRegStr HKLM "SOFTWARE\${PRODUCT_NAME}" "Vendor" "${PRODUCT_PUBLISHER}"

	WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
	WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\Uninstall.exe"
	WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "InstallLocation" "$INSTDIR"
	WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
	WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\gpick.ico"
	WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
	WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
	WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoModify" 1
	WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoRepair" 1

	WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

Section "GTK+ (for this program only)" SecGtkPrivate
	SectionIn 1
	SetShellVarContext all  ; use all user variables as opposed to current user
	AddSize 12200  ; ~ size of unpacked gtk
	SetOutPath "$INSTDIR"
	File "${GTK_INSTALLER_EXE}"
	; TODO: in the future, when we have translations for this program,
	; make the GTK+ translations installation dependent on their installation status.
	ExecWait '"${GTK_INSTALLER_EXE}" /sideeffects=no /dllpath=root /translations=no /compatdlls=no /S /D=$INSTDIR'
	Delete "$INSTDIR\${GTK_INSTALLER_EXE}"
SectionEnd

; disabled by default
Section /o "GTK+ (shared installation)" SecGtkPublic
	SectionIn 1
	SetShellVarContext all  ; use all user variables as opposed to current user
	AddSize 12200  ; ~ size of unpacked gtk
	SetOutPath "$INSTDIR"
	File "${GTK_INSTALLER_EXE}"
	ExecWait '"${GTK_INSTALLER_EXE}"'
	Delete "$INSTDIR\${GTK_INSTALLER_EXE}"
SectionEnd



; Executed on installation start
Function .onInit
	SetShellVarContext all  ; use all user variables as opposed to current user
	
	${GetOptions} "$CMDLINE" "/removeold=" $install_option_removeold
	
	Call PreventMultipleInstances
	Call DetectPrevInstallation
	StrCpy $gtk_mode "private" ; default
FunctionEnd

function .onselchange

!ifndef NO_GTK
	; Remember which gtk section was selected.
	; Deselect the other section.

	; If it was private, we check if public is checked and uncheck private.

	StrCmp $gtk_mode "private" check_public  ; old selection
	StrCmp $gtk_mode "public" check_private  ; old selection
	goto check_exit

	check_public:
		SectionGetFlags ${SecGtkPublic} $gtk_tmp  ; see if it's checked
		IntOp $gtk_tmp $gtk_tmp & ${SF_SELECTED}
		IntCmp $gtk_tmp ${SF_SELECTED} "" check_exit check_exit
		SectionGetFlags ${SecGtkPrivate} $gtk_tmp  ; unselect the other one
		IntOp $gtk_tmp $gtk_tmp & ${SECTION_OFF}
		SectionSetFlags ${SecGtkPrivate} $gtk_tmp
		goto check_exit

	check_private:
		SectionGetFlags ${SecGtkPrivate} $gtk_tmp  ; see if it's checked
		IntOp $gtk_tmp $gtk_tmp & ${SF_SELECTED}
		IntCmp $gtk_tmp ${SF_SELECTED} "" check_exit check_exit
		SectionGetFlags ${SecGtkPublic} $gtk_tmp  ; unselect the other one
		IntOp $gtk_tmp $gtk_tmp & ${SECTION_OFF}
		SectionSetFlags ${SecGtkPublic} $gtk_tmp

	check_exit:


	; store the current mode
	StrCpy $gtk_mode "none"

	SectionGetFlags ${SecGtkPrivate} $gtk_tmp
	IntOp $gtk_tmp $gtk_tmp & ${SF_SELECTED}
	IntCmp $gtk_tmp ${SF_SELECTED} "" mode_end_private mode_end_private
	StrCpy $gtk_mode "private"
	mode_end_private:

	SectionGetFlags ${SecGtkPublic} $gtk_tmp
	IntOp $gtk_tmp $gtk_tmp & ${SF_SELECTED}
	IntCmp $gtk_tmp ${SF_SELECTED} "" mode_end_public mode_end_public
	StrCpy $gtk_mode "public"
	mode_end_public:

	; MessageBox MB_ICONINFORMATION|MB_OK "gtk_mode: $gtk_mode" /SD IDOK
!endif  ; !NO_GTK

functionend


Function on_components_page_leave
	StrCmp $gtk_mode "none" "" noabort
		Call AskForGtk
	noabort:
FunctionEnd


; Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${SecProgramFiles} "Gpick - Advanced color picker"
	!insertmacro MUI_DESCRIPTION_TEXT ${SecGtkPrivate} "GTK+ libraries, needed by Gpick. \
		This will install a private version of GTK+, usable only by Gpick."
	!insertmacro MUI_DESCRIPTION_TEXT ${SecGtkPublic} "GTK+ libraries, needed by Gpick. \
		This will install a system-wide version of GTK+, shareable with other programs."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Section "Desktop Shortcut" SecDesktopShortcut
  SetOutPath "$INSTDIR"
  CreateShortCut "$DESKTOP\Gpick.lnk" "$INSTDIR\Gpick.exe" ""
SectionEnd

Section "-Start Menu Shortcut" SecStartMenu

  SetOutPath "$INSTDIR"
  
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
    
    CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Gpick.lnk" "$INSTDIR\Gpick.exe"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  
  !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

; ------------------ POST INSTALL


;Uninstaller Section

Section "Uninstall"
	SetShellVarContext all  ; use all user variables as opposed to current user
	SetAutoClose false

	ReadRegStr $gtk_mode HKLM "SOFTWARE\${PRODUCT_NAME}" "GtkInstalledMode"
	StrCmp $gtk_mode "private" "" skip_gtk_remove
		; remove private GTK+, specify the same custom options are during installation
		ExecWait "$INSTDIR\gtk2_runtime_uninst.exe /remove_config=yes /sideeffects=no /dllpath=root /translations=no /compatdlls=no /S"
		; _?=$INSTDIR
		; Delete "$INSTDIR\gtk2_runtime_uninst.exe"  ; If using _? flag, it won't get deleted automatically, do it manually.
skip_gtk_remove:

	DeleteRegKey HKLM "SOFTWARE\${PRODUCT_NAME}"
	DeleteRegKey HKLM "${PRODUCT_UNINST_KEY}"
	StrCmp $gtk_mode "private" skip_exe_PATH_remove
	DeleteRegKey HKLM "${REGISTRY_APP_PATHS}\gpick.exe"
skip_exe_PATH_remove:
	
	Delete "$INSTDIR\Uninstall.exe"

	Delete "$INSTDIR\gpick.exe"
	Delete "$INSTDIR\lua5.2.dll"
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
	Delete "$INSTDIR\share\gpick\layouts.lua"
	RMDir "$INSTDIR\share\gpick"
	RMDir "$INSTDIR\share\locale"
	RMDir "$INSTDIR\share"

	RMDir "$INSTDIR"
  
  !insertmacro MUI_STARTMENU_GETFOLDER Application $MUI_TEMP
  
  Delete "$SMPROGRAMS\$MUI_TEMP\Gpick.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\Uninstall.lnk"
  
  Delete "$DESKTOP\Gpick.lnk"
  
  ;Delete empty start menu parent diretories
  StrCpy $MUI_TEMP "$SMPROGRAMS\$MUI_TEMP"
 
  startMenuDeleteLoop:
	ClearErrors
    RMDir $MUI_TEMP
    GetFullPathName $MUI_TEMP "$MUI_TEMP\.."
    
    IfErrors startMenuDeleteLoopDone
  
    StrCmp $MUI_TEMP $SMPROGRAMS startMenuDeleteLoopDone startMenuDeleteLoop
  startMenuDeleteLoopDone:

  DeleteRegKey /ifempty HKLM "Software\Gpick"

SectionEnd

; Detect previous installation
Function DetectPrevInstallation
	; if /removeold=no option is given, don't check anything.
	StrCmp $install_option_removeold "no" old_detect_done

	SetShellVarContext all  ; use all user variables as opposed to current user
	push $R0

	; detect previous installation
	ReadRegStr $R0 HKLM "${PRODUCT_UNINST_KEY}" "UninstallString"
	StrCmp $R0 "" old_detect_done

	MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
		"${PRODUCT_NAME} is already installed. $\n$\nClick `OK` to remove the \
		previous version or `Cancel` to continue anyway." \
		/SD IDOK IDOK old_uninst
		; Abort
		goto old_detect_done

	; Run the old uninstaller
	old_uninst:
		ClearErrors
		IfSilent old_silent_uninst old_nosilent_uninst

		old_nosilent_uninst:
			ExecWait '$R0'
			goto old_uninst_continue

		old_silent_uninst:
			ExecWait '$R0 /S _?=$INSTDIR'

		old_uninst_continue:

		IfErrors old_no_remove_uninstaller

		; You can either use Delete /REBOOTOK in the uninstaller or add some code
		; here to remove to remove the uninstaller. Use a registry key to check
		; whether the user has chosen to uninstall. If you are using an uninstaller
		; components page, make sure all sections are uninstalled.
		old_no_remove_uninstaller:

	old_detect_done: ; old installation not found, all ok

	pop $R0
FunctionEnd

; detect GTK installation (any of available versions)
Function AskForGtk
	SetShellVarContext all  ; use all user variables as opposed to current user
	push $R0

	ReadRegStr $R0 HKLM "SOFTWARE\GTK\2.0" "DllPath"
	StrCmp $R0 "" no_gtk have_gtk

	no_gtk:
		MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
		"GTK2-Runtime is not installed. This product needs it to function properly.$\n\
		Please install GTK2-Runtime from http://gtk-win.sf.net/ first.$\n$\n\
		Click 'Cancel' to abort the installation \
		or 'OK' to continue anyway." \
		/SD IDOK IDOK have_gtk
		;Abort  ; Abort has different meaning from onpage callbacks, so use Quit
		Quit
		goto end_gtk_check

	have_gtk:
		; do nothing

	end_gtk_check:

	pop $R0
FunctionEnd



; Prevent running multiple instances of the installer
Function PreventMultipleInstances
	Push $R0
	System::Call 'kernel32::CreateMutexA(i 0, i 0, t ${PRODUCT_NAME}) ?e'
	Pop $R0
	StrCmp $R0 0 +3
		MessageBox MB_OK|MB_ICONEXCLAMATION "The installer is already running." /SD IDOK
		Abort
	Pop $R0
FunctionEnd
