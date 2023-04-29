Unicode True
!include "MUI2.nsh"
!include "StrFunc.nsh"
!include "WinVer.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"

${StrLoc}

Name "happynet"
OutFile "happynet_install.exe"

RequestExecutionLevel admin

BrandingText "Happynet Installer"
!define PRODUCT_VERSION "1.6.0.0"
!define PRODUCT_PUBLISHER "happyn.cn"

InstallDir "$PROGRAMFILES\happynet"
InstallDirRegKey HKLM "Software\Happynet" "Path"

Function finishpageaction
CreateShortcut "$DESKTOP\happynet.lnk" "$INSTDIR\happynet.exe"
FunctionEnd

!define MUI_FINISHPAGE_SHOWREADME ""
!define MUI_FINISHPAGE_SHOWREADME_CHECKED
!define MUI_FINISHPAGE_SHOWREADME_TEXT "创建桌面图标"
!define MUI_FINISHPAGE_SHOWREADME_FUNCTION finishpageaction

!define MUI_FINISHPAGE_RUN "$INSTDIR\happynet.exe"
!define MUI_FINISHPAGE_RUN_TEXT "现在运行happynet"
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
VIAddVersionKey  ProductName "Happynet Windows Client"
VIAddVersionKey  Comments "happyn for easy net"
VIAddVersionKey  CompanyName ${PRODUCT_PUBLISHER}
VIAddVersionKey  LegalCopyright "Copyright ${PRODUCT_PUBLISHER}"
VIAddVersionKey  FileDescription "happynet.exe"
VIAddVersionKey  FileVersion ${PRODUCT_VERSION}


;--------------------------------

Var is64bit

Icon "..\happynet\happyn.ico"

Section "happynet"
  SectionIn RO
  SetOutPath $INSTDIR

  CreateDirectory "$SMPROGRAMS\happynet"
  File "..\happynet\happyn.ico"

  WriteUninstaller "happynet_uninst.exe"
  WriteRegStr HKLM "SOFTWARE\Happynet" "Path" '$INSTDIR'
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Happynet" "DisplayName" "happynet"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Happynet" "UninstallString" '"$INSTDIR\happynet_uninst.exe"'
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Happynet" "QuietUninstallString" '"$INSTDIR\happynet_uninst.exe" /S'
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Happynet" "InstallLocation" '"$INSTDIR"'
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Happynet" "DisplayIcon" '"$INSTDIR\happyn.ico"'
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Happynet" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Happynet" "Publisher" "${PRODUCT_PUBLISHER}"


; --------------------------------------------------------
; happynedge.exe
; --------------------------------------------------------
  SetOutPath $INSTDIR
  ${If} ${RunningX64}
    File "n2n_release\x64\happynedge.exe"
    File "n2n_release\x64\happynssm.exe"
    File "n2n_release\x64\happynportfwd.exe"
    File "n2n_release\x64\happynroute.exe"
    File "n2n_release\x64\happynmonitor.exe"
    File "n2n_release\happynet.ico"
  ${Else}
    ${If} ${IsWinXP}
      File "n2n_release\winxp\happynedge.exe"
    ${Else}
      File "n2n_release\x86\happynedge.exe"
      File "n2n_release\x86\happynssm.exe"
      File "n2n_release\x86\happynportfwd.exe"
      File "n2n_release\x86\happynroute.exe"
      File "n2n_release\x86\happynmonitor.exe"
      File "n2n_release\happynet.ico"
    ${EndIf}
  ${EndIf}


; --------------------------------------------------------
; dll files
; --------------------------------------------------------
  SetOutPath "$INSTDIR\platforms"
  File "n2n_release\platforms\qwindows.dll"
  File "n2n_release\platforms\qoffscreen.dll"
  File "n2n_release\platforms\qminimal.dll"

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

    ${If} ${IsNativeAMD64}
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
    ${ElseIf} ${IsNativeARM64}
      File "..\tap_driver\NDIS6_arm\tapinstall.exe"
      File "..\tap_driver\NDIS6_arm\OemVista.inf"
      File "..\tap_driver\NDIS6_arm\tap0901.cat"
      File "..\tap_driver\NDIS6_arm\tap0901.sys"
      DetailPrint  "INSTALL NDIS6_arm"
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
  EnumRegKey $0 HKLM "SOFTWARE\happynet" 0
  ${If} ${Errors}
    DetailPrint  "Value not found"
    WriteRegStr HKLM "SOFTWARE\Happynet\Parameters" "community" "community"
    WriteRegStr HKLM "SOFTWARE\Happynet\Parameters" "enckey" "happyn"
    WriteRegStr HKLM "SOFTWARE\Happynet\Parameters" "ip_address" ""
    WriteRegStr HKLM "SOFTWARE\Happynet\Parameters" "keyfile" ""
    WriteRegDWORD HKLM "SOFTWARE\Happynet\Parameters" "local_port" 0x00000000
    WriteRegStr HKLM "SOFTWARE\Happynet\Parameters" "mac_address" ""
    WriteRegDWORD HKLM "SOFTWARE\Happynet\Parameters" "mtu" 0x00000000
    WriteRegDWORD HKLM "SOFTWARE\Happynet\Parameters" "multicast" 0x00000000
    WriteRegDWORD HKLM "SOFTWARE\Happynet\Parameters" "packet_forwarding" 0x00000000
    WriteRegDWORD HKLM "SOFTWARE\Happynet\Parameters" "header_encry" 0x00000000
    WriteRegDWORD HKLM "SOFTWARE\Happynet\Parameters" "data_compress" 0x00000001
    WriteRegDWORD HKLM "SOFTWARE\Happynet\Parameters" "select_rtt" 0x00000001
    WriteRegStr HKLM "SOFTWARE\Happynet\Parameters" "subnet_mask" "255.255.255.0"
    WriteRegStr HKLM "SOFTWARE\Happynet\Parameters" "supernode_addr" "vip00.happyn.cc"
    WriteRegDWORD HKLM "SOFTWARE\Happynet\Parameters" "supernode_port" 0x00007530
    WriteRegStr HKLM "SOFTWARE\Happynet\Parameters" "adapter" ""
    WriteRegStr HKLM "SOFTWARE\Happynet\Parameters" "custom_param" ""
    WriteRegDWORD HKLM "SOFTWARE\Happynet\Parameters" "system_service" 0x00000000
    WriteRegDWORD HKLM "SOFTWARE\Happynet\Parameters" "auto_start" 0x00000000
    WriteRegDWORD HKLM "SOFTWARE\Happynet\Parameters" "auto_tray" 0x00000000
  ${Else}
    ${IF} $0 == ""
          DetailPrint   "NUL exists and it's empty"
          WriteRegStr HKLM "SOFTWARE\Happynet\Parameters" "community" "community"
          WriteRegStr HKLM "SOFTWARE\Happynet\Parameters" "enckey" "happyn"
          WriteRegStr HKLM "SOFTWARE\Happynet\Parameters" "ip_address" ""
          WriteRegStr HKLM "SOFTWARE\Happynet\Parameters" "keyfile" ""
          WriteRegDWORD HKLM "SOFTWARE\Happynet\Parameters" "local_port" 0x00000000
          WriteRegStr HKLM "SOFTWARE\Happynet\Parameters" "mac_address" ""
          WriteRegDWORD HKLM "SOFTWARE\Happynet\Parameters" "mtu" 0x00000000
          WriteRegDWORD HKLM "SOFTWARE\Happynet\Parameters" "multicast" 0x00000000
          WriteRegDWORD HKLM "SOFTWARE\Happynet\Parameters" "packet_forwarding" 0x00000000
          WriteRegDWORD HKLM "SOFTWARE\Happynet\Parameters" "header_encry" 0x00000000
          WriteRegDWORD HKLM "SOFTWARE\Happynet\Parameters" "data_compress" 0x00000001
          WriteRegDWORD HKLM "SOFTWARE\Happynet\Parameters" "select_rtt" 0x00000001
          WriteRegStr HKLM "SOFTWARE\Happynet\Parameters" "subnet_mask" "255.255.255.0"
          WriteRegStr HKLM "SOFTWARE\Happynet\Parameters" "supernode_addr" "vip00.happyn.cc"
          WriteRegDWORD HKLM "SOFTWARE\Happynet\Parameters" "supernode_port" 0x00007530
          WriteRegStr HKLM "SOFTWARE\Happynet\Parameters" "adapter" ""
          WriteRegStr HKLM "SOFTWARE\Happynet\Parameters" "custom_param" ""
          WriteRegDWORD HKLM "SOFTWARE\Happynet\Parameters" "system_service" 0x00000000
          WriteRegDWORD HKLM "SOFTWARE\Happynet\Parameters" "auto_start" 0x00000000
          WriteRegDWORD HKLM "SOFTWARE\Happynet\Parameters" "auto_tray" 0x00000000
      ${ELSE}
          DetailPrint   "Value isn't empty"
          ;--------------------------------
          ;add new option
          EnumRegKey $0 HKLM "SOFTWARE\Happynet\Parameters\select_rtt" 0
          ${If} ${Errors}
              DetailPrint  "select_rtt not found"
              WriteRegDWORD HKLM "SOFTWARE\Happynet\Parameters" "select_rtt" 0x00000001
          ${ENDIF}

          EnumRegKey $0 HKLM "SOFTWARE\Happynet\Parameters\adapter" 0
          ${If} ${Errors}
              DetailPrint  "adapter not found"
              WriteRegDWORD HKLM "SOFTWARE\Happynet\Parameters" "adapter" ""
          ${ENDIF}

      ${ENDIF}
  ${EndIf}


; --------------------------------------------------------
; GUI TOOL
; --------------------------------------------------------
  SetOutPath $INSTDIR

  CreateShortCut "$SMPrograms\happynet\happynet.lnk" "$INSTDIR\happynet.exe"
  File "..\Release\happynet.exe"

; --------------------------------------------------------
; FINAL
; --------------------------------------------------------
  CreateShortCut "$SMPROGRAMS\happynet\Uninstall happynet.lnk" "$INSTDIR\happynet_uninst.exe"
SectionEnd

Function .onInit
  System::Call "kernel32::GetCurrentProcess() i.s"
  System::Call "kernel32::IsWow64Process(is, *i.s)"
  Pop $is64bit
FunctionEnd

UninstallText "This will uninstall happynet client.  Click 'Uninstall' to continue."

Section "Uninstall"
  nsExec::ExecToLog '"$INSTDIR\drv\tapinstall" remove TAP0901'
  ;nsExec::ExecToLog '"$INSTDIR\happynssm.exe" stop Happynet'
  ;nsExec::ExecToLog '"$INSTDIR\happynssm.exe" remove Happynet confirm'
  SimpleSC::StopService "Happynet" 1 30
  SimpleSC::RemoveService "Happynet"
  Delete "$INSTDIR\drv\*.*"
  Delete "$INSTDIR\platforms\*.*"
  RMDIR "$INSTDIR\platforms"
  Delete "$INSTDIR\*.*"
  Delete "$SMPROGRAMS\happynet\*.*"
  RMDir "$SMPROGRAMS\happynet"
  RMDir "$INSTDIR\drv"
  RMDir "$INSTDIR"
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Happynet"
  DeleteRegKey HKLM "SOFTWARE\Happynet"
  DeleteRegValue HKCU "SOFTWARE\Microsoft\Windows\CurrentVersion\Run" "Happynet"
  Delete "$DESKTOP\happynet.lnk"

  ; MAKE SURE DELETE ALL REGITEMS INSTALLED BY OTHER USER
  ;IntOp $0 0 + 0
  ;EnumStart:
  ;  EnumRegKey $R1 HKEY_USERS "" $0
  ;  IntOp $0 $0 + 1
  ;  StrCmp $R1 ".DEFAULT" EnumStart
  ;  StrCmp $R1 "" EnumEnd
  ;  DeleteRegValue HKU "$R1\Software\Microsoft\Windows\CurrentVersion\Run" "Happynet"
  ;  Goto EnumStart
  ;EnumEnd:
SectionEnd
