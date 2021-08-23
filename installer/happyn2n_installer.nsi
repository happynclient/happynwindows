Unicode True
!include "MUI2.nsh"
!include "StrFunc.nsh"
!include "WinVer.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"

${StrLoc}

Name "Happyn2n"
OutFile "happyn2n_install.exe"

RequestExecutionLevel admin

BrandingText "Happyn2n Installer $$Rev: 2 $$"
!define PRODUCT_VERSION "0.1.0.0"
!define PRODUCT_PUBLISHER "happyn.cc"

InstallDir "$PROGRAMFILES\Happyn2n"
InstallDirRegKey HKLM "Software\Happyn2n" "Path"

!define MUI_FINISHPAGE_RUN "$INSTDIR\happyn2n.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Launch Happyn2n"
!insertmacro MUI_PAGE_LICENSE "../COPYING"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "simpchinese"
;--------------------------------
;Version Information
VIProductVersion ${PRODUCT_VERSION}
VIAddVersionKey  ProductVersion ${PRODUCT_VERSION}
VIAddVersionKey  ProductName "Happyn2n Windows Client"
VIAddVersionKey  Comments "happyn for easy net"
VIAddVersionKey  CompanyName ${PRODUCT_PUBLISHER}
VIAddVersionKey  LegalCopyright "Copyright ${PRODUCT_PUBLISHER}"
VIAddVersionKey  FileDescription "Happyn2n.exe"
VIAddVersionKey  FileVersion ${PRODUCT_VERSION}


;--------------------------------

Var is64bit

Icon "..\Happyn2n\happyn.ico"

Section "Happyn2n"
  SectionIn RO
  SetOutPath $INSTDIR
  
  CreateDirectory "$SMPROGRAMS\Happyn2n"
  File "..\Happyn2n\happyn.ico"

  WriteUninstaller "happyn2n_uninst.exe"
  WriteRegStr HKLM "SOFTWARE\Happyn2n" "Path" '$INSTDIR'
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Happyn2n" "DisplayName" "Happyn2n"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Happyn2n" "UninstallString" '"$INSTDIR\happyn2n_uninst.exe"'
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Happyn2n" "QuietUninstallString" '"$INSTDIR\happyn2n_uninst.exe" /S'
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Happyn2n" "InstallLocation" '"$INSTDIR"'
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Happyn2n" "DisplayIcon" '"$INSTDIR\happyn.ico"'
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Happyn2n" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Happyn2n" "Publisher" "${PRODUCT_PUBLISHER}"
  
  
; --------------------------------------------------------
; edge.exe
; --------------------------------------------------------  
  SetOutPath $INSTDIR
  ${If} ${RunningX64}
    File "n2n_release\x64\edge.exe"
  ${Else}
    ${If} ${IsWinXP}
      File "n2n_release\winxp\edge.exe"
    ${Else}  
      File "n2n_release\x86\edge.exe"
    ${EndIf}
  ${EndIf}
  
  
; --------------------------------------------------------
; TAP DRIVER
; --------------------------------------------------------

  SetOutPath "$INSTDIR\drv"
  
  ${IfNot} ${AtLeastWinVista}

    ${If} ${RunningX64}
      File "..\tap_driver\NDIS5_x64\tapinstall.exe"
      File "..\tap_driver\NDIS5_x64\OemWin2k.inf"
      File "..\tap_driver\NDIS5_x64\tap0901.cat"
      File "..\tap_driver\NDIS5_x64\tap0901.sys"
      DetailPrint  "INSTALL NDIS5_x64"    
      nsExec::ExecToStack '"$INSTDIR\drv\tapinstall" find TAP0901'
      Pop $1
        Pop $2
        ${StrLoc} $0 $2 "No matching devices" 0
        nsExec::ExecToLog '"$INSTDIR\drv\tapinstall" remove TAP0901'
        nsExec::ExecToLog '"$INSTDIR\drv\tapinstall" install OemWin2k.inf TAP0901'
    ${Else}
      File "..\tap_driver\NDIS5_x86\tapinstall.exe"
      File "..\tap_driver\NDIS5_x86\OemWin2k.inf"
      File "..\tap_driver\NDIS5_x86\tap0901.cat"
      File "..\tap_driver\NDIS5_x86\tap0901.sys"
      DetailPrint  "INSTALL NDIS5_x86"      
      nsExec::ExecToStack '"$INSTDIR\drv\tapinstall" find TAP0901'
      Pop $1
        Pop $2
        ${StrLoc} $0 $2 "No matching devices" 0
        nsExec::ExecToLog '"$INSTDIR\drv\tapinstall" remove TAP0901'
        nsExec::ExecToLog '"$INSTDIR\drv\tapinstall" install OemWin2k.inf TAP0901'
    ${EndIf}
  
  ${Else}

    ${If} ${RunningX64}
      File "..\tap_driver\NDIS6_x64\tapinstall.exe"
      File "..\tap_driver\NDIS6_x64\OemVista.inf"
      File "..\tap_driver\NDIS6_x64\tap0901.cat"
      File "..\tap_driver\NDIS6_x64\tap0901.sys"
      DetailPrint  "INSTALL NDIS6_x64"
      nsExec::ExecToStack '"$INSTDIR\drv\tapinstall" find TAP0901'
      Pop $1
      Pop $2
      ${StrLoc} $0 $2 "No matching devices" 0
      nsExec::ExecToLog '"$INSTDIR\drv\tapinstall" remove TAP0901'
      nsExec::ExecToLog '"$INSTDIR\drv\tapinstall" install OemVista.inf TAP0901'  
    ${Else}
      File "..\tap_driver\NDIS6_x86\tapinstall.exe"
      File "..\tap_driver\NDIS6_x86\OemVista.inf"
      File "..\tap_driver\NDIS6_x86\tap0901.cat"
      File "..\tap_driver\NDIS6_x86\tap0901.sys"
      DetailPrint  "INSTALL NDIS6_x86"
      nsExec::ExecToStack '"$INSTDIR\drv\tapinstall" find TAP0901'
      Pop $1
      Pop $2
      ${StrLoc} $0 $2 "No matching devices" 0
      nsExec::ExecToLog '"$INSTDIR\drv\tapinstall" remove TAP0901'
      nsExec::ExecToLog '"$INSTDIR\drv\tapinstall" install OemVista.inf TAP0901'  
    ${EndIf}

  ${EndIf}

; --------------------------------------------------------
; SERVICE
; --------------------------------------------------------
  SetOutPath $INSTDIR

  ClearErrors
  EnumRegKey $0 HKLM "SOFTWARE\Happyn2n" 0
  ${If} ${Errors}
    DetailPrint  "Value not found"
    WriteRegStr HKLM "SOFTWARE\Happyn2n\Parameters" "community" "community"
    WriteRegStr HKLM "SOFTWARE\Happyn2n\Parameters" "enckey" "enckey"
    WriteRegStr HKLM "SOFTWARE\Happyn2n\Parameters" "ip_address" ""
    WriteRegStr HKLM "SOFTWARE\Happyn2n\Parameters" "keyfile" ""
    WriteRegDWORD HKLM "SOFTWARE\Happyn2n\Parameters" "local_port" 0x00000000
    WriteRegStr HKLM "SOFTWARE\Happyn2n\Parameters" "mac_address" ""
    WriteRegDWORD HKLM "SOFTWARE\Happyn2n\Parameters" "mtu" 0x00000000
    WriteRegDWORD HKLM "SOFTWARE\Happyn2n\Parameters" "multicast" 0x00000000
    WriteRegDWORD HKLM "SOFTWARE\Happyn2n\Parameters" "packet_forwarding" 0x00000001
    WriteRegDWORD HKLM "SOFTWARE\Happyn2n\Parameters" "header_encry" 0x00000000
    WriteRegStr HKLM "SOFTWARE\Happyn2n\Parameters" "subnet_mask" "255.255.255.0"
    WriteRegStr HKLM "SOFTWARE\Happyn2n\Parameters" "supernode_addr" "vip00.happyn.cc"
    WriteRegDWORD HKLM "SOFTWARE\Happyn2n\Parameters" "supernode_port" 0x00007530
    WriteRegStr HKLM "SOFTWARE\Happyn2n\Parameters" "custom_param" "-l rvip.happyn.cc:30000"
    WriteRegDWORD HKLM "SOFTWARE\Happyn2n\Parameters" "auto_start" 0x00000000
    WriteRegDWORD HKLM "SOFTWARE\Happyn2n\Parameters" "auto_tray" 0x00000000
  ${Else}
    ${IF} $0 == ""
          DetailPrint   "NUL exists and it's empty"
          WriteRegStr HKLM "SOFTWARE\Happyn2n\Parameters" "community" "community"
          WriteRegStr HKLM "SOFTWARE\Happyn2n\Parameters" "enckey" "enckey"
          WriteRegStr HKLM "SOFTWARE\Happyn2n\Parameters" "ip_address" ""
          WriteRegStr HKLM "SOFTWARE\Happyn2n\Parameters" "keyfile" ""
          WriteRegDWORD HKLM "SOFTWARE\Happyn2n\Parameters" "local_port" 0x00000000
          WriteRegStr HKLM "SOFTWARE\Happyn2n\Parameters" "mac_address" ""
          WriteRegDWORD HKLM "SOFTWARE\Happyn2n\Parameters" "mtu" 0x00000000
          WriteRegDWORD HKLM "SOFTWARE\Happyn2n\Parameters" "multicast" 0x00000000
          WriteRegDWORD HKLM "SOFTWARE\Happyn2n\Parameters" "packet_forwarding" 0x00000001
          WriteRegDWORD HKLM "SOFTWARE\Happyn2n\Parameters" "header_encry" 0x00000000
          WriteRegStr HKLM "SOFTWARE\Happyn2n\Parameters" "subnet_mask" "255.255.255.0"
          WriteRegStr HKLM "SOFTWARE\Happyn2n\Parameters" "supernode_addr" "vip00.happyn.cc"
          WriteRegDWORD HKLM "SOFTWARE\Happyn2n\Parameters" "supernode_port" 0x00007530
          WriteRegStr HKLM "SOFTWARE\Happyn2n\Parameters" "custom_param" "-l rvip.happyn.cc:30000"
          WriteRegDWORD HKLM "SOFTWARE\Happyn2n\Parameters" "auto_start" 0x00000000
          WriteRegDWORD HKLM "SOFTWARE\Happyn2n\Parameters" "auto_tray" 0x00000000
      ${ELSE}
          DetailPrint   "Value isn't empty"
      ${ENDIF}
  ${EndIf}

; --------------------------------------------------------
; GUI TOOL
; --------------------------------------------------------
  SetOutPath $INSTDIR

  CreateShortCut "$SMPrograms\Happyn2n\Happyn2n.lnk" "$INSTDIR\happyn2n.exe"
  File "..\Release\happyn2n.exe"

; --------------------------------------------------------
; FINAL
; --------------------------------------------------------
  CreateShortCut "$SMPROGRAMS\Happyn2n\Uninstall Happyn2n.lnk" "$INSTDIR\happyn2n_uninst.exe"
SectionEnd

Function .onInit
  System::Call "kernel32::GetCurrentProcess() i.s"
  System::Call "kernel32::IsWow64Process(is, *i.s)"
  Pop $is64bit
FunctionEnd

UninstallText "This will uninstall Happyn2n.  Click 'Uninstall' to continue."

Section "Uninstall"
  nsExec::ExecToLog '"$INSTDIR\drv\tapinstall" remove TAP0901'
  Delete "$INSTDIR\drv\*.*"
  Delete "$INSTDIR\*.*"
  Delete "$SMPROGRAMS\Happyn2n\*.*"
  RMDir "$SMPROGRAMS\Happyn2n"
  RMDir "$INSTDIR\drv"
  RMDir "$INSTDIR"
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Happyn2n"
  DeleteRegKey HKLM "SOFTWARE\Happyn2n"
SectionEnd