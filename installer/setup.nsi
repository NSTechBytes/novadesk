;--------------------------------
; Novadesk Installer Script
;--------------------------------

; Define global version variable (Update this as needed)
!define VERSION "0.9.11.0"

; The name of the installer
Name "Novadesk"

; The file to write
OutFile "dist_output\Novadesk_Setup_v${VERSION}_Beta.exe"
SetCompressor /SOLID lzma
ReserveFile "plugins\x86-unicode\UAC.dll"

; The default installation directory
InstallDir "$PROGRAMFILES64\Novadesk"

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\Novadesk" "Install_Dir"

; Start as the desktop user. Standard installs elevate only after the user presses
; Install, so Windows shows the UAC shield on that button instead of on the
; installer executable's icon.
RequestExecutionLevel user

; Use 64-bit registry view
!include "x64.nsh"
!include "LogicLib.nsh"
!include "nsDialogs.nsh"
!include "Sections.nsh"
!include "StrFunc.nsh"
!addplugindir "plugins\\x86-unicode"
!include "nsis\\UAC.nsh"
${StrStr}
${StrRep}

;--------------------------------
; Interface Settings
;--------------------------------
!include "MUI2.nsh"

; Remove NSIS's default "Nullsoft Install System" footer.
BrandingText " "

;--------------------------------
; Windows Message Constants
;--------------------------------
!ifndef WM_WININICHANGE
!define WM_WININICHANGE 0x001A
!endif
!ifndef HWND_BROADCAST
!define HWND_BROADCAST 0xFFFF
!endif
!ifndef BCM_SETSHIELD
!define BCM_SETSHIELD 0x160C
!endif

;--------------------------------
; Icon & Images
;--------------------------------
!define MUI_ICON "assets\installer.ico"
!define MUI_UNICON "assets\uninstaller.ico"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "assets\header.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP "assets\banner.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "assets\banner.bmp"

;--------------------------------
; Pages
;--------------------------------
!define MUI_PAGE_CUSTOMFUNCTION_PRE SkipPageIfInnerInstance
!insertmacro MUI_PAGE_WELCOME
!define MUI_PAGE_CUSTOMFUNCTION_PRE SkipPageIfInnerInstance
!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
Page custom InstallModePageCreate InstallModePageLeave
!define MUI_PAGE_CUSTOMFUNCTION_PRE SkipPageIfInnerInstance
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE DirectoryPageLeave
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

; Finish page settings
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT "Run Novadesk"
!define MUI_FINISHPAGE_RUN_FUNCTION FinishRun
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
UninstPage custom un.CompleteRemovePageCreate un.CompleteRemovePageLeave
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

;--------------------------------
; Data
;--------------------------------
Var InstallMode
Var RadioStandard
Var RadioPortable
Var DocsRoot
Var ScriptsRoot
Var RemoveCompletely
Var UnRemoveCheckbox
Var UserDocsDir
Var UserAppDataDir

; The UAC plug-in keeps the UI process unelevated and launches an elevated
; companion only when a standard installation actually needs administrator
; access. This is the same installation model used by Rainmeter.
!macro ElevateForStandardInstall
NovadeskUacTryAgain:
  !insertmacro UAC_RunElevated
  ${Switch} $0
    ${Case} 0
      ${IfThen} $1 = 1 ${|} Quit ${|}
      ${IfThen} $3 <> 0 ${|} ${Break} ${|}
      ${If} $1 = 3
        MessageBox MB_YESNO|MB_ICONEXCLAMATION|MB_TOPMOST "Administrator access is required to install Novadesk. Try again?" IDYES NovadeskUacTryAgain
      ${EndIf}
    ${Case} 1223
      Quit
    ${Case} 1062
      MessageBox MB_OK|MB_ICONSTOP "The logon service is not running, so Novadesk cannot be installed."
      Quit
    ${Default}
      MessageBox MB_OK|MB_ICONSTOP "Unable to obtain administrator access for the Novadesk installation. Error: $0"
      Quit
  ${EndSwitch}
  SetShellVarContext all
!macroend

;--------------------------------
; Sections
;--------------------------------

Section -CoreFiles SecCoreFiles
  SectionIn RO

  ${If} $InstallMode == "standard"
    ${IfNot} ${UAC_IsAdmin}
      ; UAC_IsAdmin can report membership instead of the current token state.
      System::Call "shell32::IsUserAnAdmin()i.r0"
      ${If} $0 = 0
        !insertmacro ElevateForStandardInstall
      ${EndIf}
    ${EndIf}
    SetShellVarContext all
  ${EndIf}

  SetRegView 64
  ; Enforce 64-bit redirection
  ${If} ${RunningX64}
    ${DisableX64FSRedirection}
  ${EndIf}

  SetOutPath "$INSTDIR"
  
  ; Kill process if running
  nsExec::ExecToStack 'taskkill /F /IM "novadesk.exe"'
  nsExec::ExecToStack 'taskkill /F /IM "Novadesk.exe"'
  nsExec::ExecToStack 'taskkill /F /IM "manage_novadesk.exe"'
  nsExec::ExecToStack 'taskkill /F /IM "ndpkg_installer.exe"'
  nsExec::ExecToStack 'taskkill /F /IM "nwm.exe"'
  Sleep 1000

  ; Add Novadesk files from dist
  SetOutPath "$INSTDIR"
  File "..\dist\novadesk.exe"
  File "..\dist\manage_novadesk.exe"
  File "..\dist\ndpkg_installer.exe"
  File /r "..\dist\images"

  ; Add installer stub from dist
  SetOutPath "$INSTDIR\nwm"
  File "..\dist\nwm\installer_stub.exe"
  
  ; Add nwm files from dist
  SetOutPath "$INSTDIR\nwm"
  File "..\dist\nwm\nwm.exe"
  ; Copy nwm template if it exists in dist
  File /r "..\dist\nwm\template"
  SetOutPath "$INSTDIR"

  ${If} $InstallMode == "standard"
    ; Ensure UserDocsDir and UserAppDataDir are never empty
    ${If} $UserDocsDir == ""
      SetShellVarContext current
      StrCpy $UserDocsDir "$DOCUMENTS"
      SetShellVarContext all
    ${EndIf}
    ${If} $UserAppDataDir == ""
      SetShellVarContext current
      StrCpy $UserAppDataDir "$APPDATA"
      SetShellVarContext all
    ${EndIf}
    StrCpy $DocsRoot "$UserDocsDir\Novadesk"
    CreateDirectory "$DocsRoot"
    ; In standard mode, widgets/addons live in Documents\Novadesk
    SetOutPath "$DocsRoot"
    File /r "..\dist\Widgets"
    File /r "..\dist\Addons"

    ; Create settings in AppData\Novadesk
    CreateDirectory "$UserAppDataDir\Novadesk"
    StrCpy $ScriptsRoot "$DocsRoot\Widgets\Fental\index.js"
    ${StrRep} $1 $ScriptsRoot "\" "\\"
    FileOpen $0 "$UserAppDataDir\Novadesk\manage_novadesk_settings.json" "w"
    FileWrite $0 "{$\r$\n"
    FileWrite $0 "  $\"loadedScripts$\": [$\r$\n"
    FileWrite $0 "    $\"$1$\"$\r$\n"
    FileWrite $0 "  ]$\r$\n"
    FileWrite $0 "}$\r$\n"
    FileClose $0

    ; Store installation folder
    WriteRegStr HKLM "Software\Novadesk" "Install_Dir" "$INSTDIR"

    ; Create uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"

    ; Add to Add/Remove Programs
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Novadesk" \
                     "DisplayName" "Novadesk v${VERSION}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Novadesk" \
                     "UninstallString" "$INSTDIR\Uninstall.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Novadesk" \
                     "DisplayIcon" "$INSTDIR\Novadesk.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Novadesk" \
                     "Publisher" "OfficialNovadesk"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Novadesk" \
                     "URLInfoAbout" "https://novadesk.pages.dev"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Novadesk" \
                     "HelpLink" "https://novadesk.pages.dev"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Novadesk" \
                     "DisplayVersion" "${VERSION}"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Novadesk" \
                     "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Novadesk" \
                     "NoRepair" 1

    ; Always create shortcuts in standard mode
    CreateDirectory "$SMPROGRAMS\Novadesk"
    CreateShortCut "$SMPROGRAMS\Novadesk\Novadesk.lnk" "$INSTDIR\manage_novadesk.exe" "" "$INSTDIR\manage_novadesk.exe" 0
    CreateShortCut "$DESKTOP\Novadesk.lnk" "$INSTDIR\manage_novadesk.exe" "" "$INSTDIR\manage_novadesk.exe" 0

    ; Always add to PATH in standard mode
    EnVar::SetHKLM
    EnVar::AddValue "PATH" "$INSTDIR"
    Pop $0
    DetailPrint "Add root to PATH returned=|$0|"
    EnVar::AddValue "PATH" "$INSTDIR\nwm"
    Pop $0
    DetailPrint "Add nwm to PATH returned=|$0|"

    ; Register .ndpkg file association with ndpkg_installer
    WriteRegStr HKLM "Software\Classes\.ndpkg" "" "Novadesk.ndpkg"
    WriteRegStr HKLM "Software\Classes\Novadesk.ndpkg" "" "Novadesk Package"
    WriteRegStr HKLM "Software\Classes\Novadesk.ndpkg\DefaultIcon" "" "$INSTDIR\ndpkg_installer.exe,0"
    WriteRegStr HKLM "Software\Classes\Novadesk.ndpkg\shell" "" "open"
    WriteRegStr HKLM "Software\Classes\Novadesk.ndpkg\shell\open\command" "" '$\"$INSTDIR\ndpkg_installer.exe$\" $\"%1$\"'
    System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
  ${Else}
    ; In portable mode, widgets/addons live in install root
    SetOutPath "$INSTDIR"
    File /r "..\dist\Widgets"
    File /r "..\dist\Addons"

    ; Create manage_novadesk_settings.json in installation root
    StrCpy $ScriptsRoot "$INSTDIR\Widgets\Fental\index.js"
    ${StrRep} $1 $ScriptsRoot "\" "\\"
    FileOpen $0 "$INSTDIR\manage_novadesk_settings.json" "w"
    FileWrite $0 "{$\r$\n"
    FileWrite $0 "  $\"loadedScripts$\": [$\r$\n"
    FileWrite $0 "    $\"$1$\"$\r$\n"
    FileWrite $0 "  ]$\r$\n"
    FileWrite $0 "}$\r$\n"
    FileClose $0

    DetailPrint "Portable mode selected: skipped uninstaller and global registry writes."
  ${EndIf}

SectionEnd

Function InstallModePageCreate
  ; Inner elevated instance skips all pre-install pages (same as Rainmeter pattern).
  ${If} ${UAC_IsInnerInstance}
    Abort
  ${EndIf}

  nsDialogs::Create 1018
  Pop $0
  ${If} $0 == error
    Abort
  ${EndIf}

  ${NSD_CreateLabel} 0 0 100% 24u "Choose installation type:"
  Pop $0
  ${NSD_CreateRadioButton} 0 28u 100% 12u "Standard (recommended): install with uninstaller and registry entries"
  Pop $RadioStandard
  ${NSD_CreateRadioButton} 0 44u 100% 12u "Portable: no uninstaller, no registry entries"
  Pop $RadioPortable

  ${NSD_Check} $RadioStandard
  StrCpy $InstallMode "standard"
  nsDialogs::Show
FunctionEnd

; Called by the inner (elevated) instance on the outer (unelevated) process to
; retrieve the user's selections via UAC sync registers ($1=InstallMode, $2=INSTDIR, $3=UserDocs, $4=UserAppData).
; HideWindow hides the outer's installer window so the user only sees the inner
; instance's INSTFILES + Finish pages — the same pattern Rainmeter uses in ExchangeSettings.
Function SyncSettingsToInner
  SetShellVarContext current
  StrCpy $1 $InstallMode
  StrCpy $2 $INSTDIR
  StrCpy $3 "$DOCUMENTS"
  StrCpy $4 "$APPDATA"
  HideWindow
FunctionEnd

; Pre-function for MUI pages: skips (Abort) the page when running as the inner
; elevated instance. Matches Rainmeter's pattern of checking UAC_IsInnerInstance
; at the top of every custom page function.
Function SkipPageIfInnerInstance
  ${If} ${UAC_IsInnerInstance}
    Abort
  ${EndIf}
FunctionEnd

Function FinishRun
  ; Explorer launches Novadesk with the desktop user's token instead of the elevated installer's token.
  ExecShell "" "$WINDIR\explorer.exe" '$\"$INSTDIR\manage_novadesk.exe$\"'
FunctionEnd

Function .onInit
  SetShellVarContext current
  StrCpy $UserDocsDir "$DOCUMENTS"
  StrCpy $UserAppDataDir "$APPDATA"
  StrCpy $InstallMode "standard"
  !insertmacro SelectSection ${SecCoreFiles}

  ; When the UAC plug-in relaunches this installer elevated (inner instance),
  ; the process starts over from .onInit. The inner instance must:
  ;   1. Verify it has admin rights.
  ;   2. Pull the user's selections ($InstallMode, $INSTDIR, $UserDocsDir, $UserAppDataDir)
  ;      from the outer process.
  ;      The outer's SyncSettingsToInner also calls HideWindow so the outer's
  ;      frozen "Installing..." page disappears and only the inner's window is visible.
  ;   3. Let the normal page loop continue — pages skip themselves via
  ;      SkipPageIfInnerInstance, so the inner jumps straight to INSTFILES + Finish.
  ${If} ${UAC_IsInnerInstance}
    ${IfNot} ${UAC_IsAdmin}
      MessageBox MB_OK|MB_ICONSTOP "Administrator access is required to complete the installation." /SD IDOK
      Quit
    ${EndIf}
    ; Retrieve user selections from the outer process; outer's window is hidden inside.
    !insertmacro UAC_AsUser_Call Function SyncSettingsToInner ${UAC_SYNCREGISTERS}
    StrCpy $InstallMode $1
    StrCpy $INSTDIR $2
    StrCpy $UserDocsDir $3
    StrCpy $UserAppDataDir $4
  ${EndIf}
FunctionEnd

Function InstallModePageLeave
  ${NSD_GetState} $RadioPortable $0
  ${If} $0 == ${BST_CHECKED}
    StrCpy $InstallMode "portable"
    StrCpy $INSTDIR "$EXEDIR"
  ${Else}
    StrCpy $InstallMode "standard"
  ${EndIf}

  ; Match Windows' elevation affordance to the selected install mode.
  GetDlgItem $1 $HWNDPARENT 1
  ${If} $InstallMode == "standard"
    SendMessage $1 ${BCM_SETSHIELD} 0 1
  ${Else}
    SendMessage $1 ${BCM_SETSHIELD} 0 0
  ${EndIf}
FunctionEnd

Function un.onInit
  !insertmacro ElevateForStandardInstall
FunctionEnd

Function un.CompleteRemovePageCreate
  nsDialogs::Create 1018
  Pop $0
  ${If} $0 == error
    Abort
  ${EndIf}

  ${NSD_CreateLabel} 0 0 100% 24u "Uninstall Options"
  Pop $0
  ${NSD_CreateCheckbox} 0 28u 100% 14u "Completely Remove Novadesk (remove Documents\Novadesk and AppData\Novadesk)"
  Pop $UnRemoveCheckbox
  ${NSD_Check} $UnRemoveCheckbox
  StrCpy $RemoveCompletely ${BST_CHECKED}
  nsDialogs::Show
FunctionEnd

Function un.CompleteRemovePageLeave
  ${NSD_GetState} $UnRemoveCheckbox $RemoveCompletely
FunctionEnd

Function DirectoryPageLeave
  ${If} $InstallMode == "portable"
    StrCpy $0 "0"

    ${StrStr} $1 "$INSTDIR" "$PROGRAMFILES64"
    ${If} $1 != ""
      StrCpy $0 "1"
    ${EndIf}

    ${StrStr} $1 "$INSTDIR" "$PROGRAMFILES"
    ${If} $1 != ""
      StrCpy $0 "1"
    ${EndIf}

    ${StrStr} $1 "$INSTDIR" "$WINDIR"
    ${If} $1 != ""
      StrCpy $0 "1"
    ${EndIf}

    ${If} $0 == "1"
      MessageBox MB_ICONEXCLAMATION|MB_OK "Portable mode cannot install into system-protected folders (Program Files / Windows). Go back and choose Standard mode for this location."
      Abort
    ${EndIf}
  ${EndIf}
FunctionEnd

;--------------------------------
; Uninstaller
;--------------------------------
Section "Uninstall"

  SetRegView 64
  ; Read the installation directory from the registry
  ReadRegStr $INSTDIR HKLM "Software\Novadesk" "Install_Dir"
  
  ; Kill process if running
  nsExec::ExecToStack 'taskkill /F /IM "novadesk.exe"'
  nsExec::ExecToStack 'taskkill /F /IM "manage_novadesk.exe"'
  nsExec::ExecToStack 'taskkill /F /IM "ndpkg_installer.exe"'
  nsExec::ExecToStack 'taskkill /F /IM "nwm.exe"'
  Sleep 1000

  ; Remove from PATH
  EnVar::SetHKLM
  EnVar::DeleteValue "PATH" "$INSTDIR"
  Pop $0
  DetailPrint "Remove root from PATH returned=|$0|"
  EnVar::DeleteValue "PATH" "$INSTDIR\nwm"
  Pop $0
  DetailPrint "Remove nwm from PATH returned=|$0|"
  
  ; Always remove registry entries
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Novadesk"
  DeleteRegKey HKLM "Software\Novadesk"
  DeleteRegKey HKLM "Software\Classes\Novadesk.ndpkg"
  DeleteRegKey HKLM "Software\Classes\.ndpkg"
  System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'

  ; Remove files
  Delete "$INSTDIR\novadesk.exe"
  Delete "$INSTDIR\manage_novadesk.exe"
  Delete "$INSTDIR\ndpkg_installer.exe"
  Delete "$INSTDIR\nwm\installer_stub.exe"
  Delete "$INSTDIR\nwm\nwm.exe"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir /r "$INSTDIR\images"
  RMDir /r "$INSTDIR\Widgets"
  RMDir /r "$INSTDIR\Addons"
  
  ${If} $RemoveCompletely == ${BST_CHECKED}
    ; Completely remove user data only when explicitly requested
    SetShellVarContext current
    RMDir /r "$APPDATA\Novadesk"
    RMDir /r "$DOCUMENTS\Novadesk"
    SetShellVarContext all
    RMDir /r "$APPDATA\Novadesk"
    RMDir /r "$DOCUMENTS\Novadesk"
  ${Else}
    DetailPrint "Keeping user data folders (Documents\Novadesk and AppData\Novadesk)."
  ${EndIf}

  ; Remove files from root directory if in portable mode
  Delete "$INSTDIR\settings.json"
  Delete "$INSTDIR\logs.log"
  Delete "$INSTDIR\config.json"
  Delete "$INSTDIR\manage_novadesk_settings.json"
  
  ; Remove nwm directory
  RMDir /r "$INSTDIR\nwm"

  ; Remove directories used
  RMDir "$INSTDIR"
  
  ; Remove desktop shortcut
  Delete "$DESKTOP\Novadesk.lnk"
  Delete "$SMPROGRAMS\Novadesk\Novadesk.lnk"
  RMDir "$SMPROGRAMS\Novadesk"

SectionEnd
