; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "DeepSea Database"
#define MyAppVersion "1.3.4"
#define MyAppPublisher "Novichkov & Sukin Sons."
#define MyAppExeName "DeepSeaBase.exe"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{484CA99A-68A0-46BD-8EAC-505A99A087B6}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={pf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
OutputBaseFilename=DeepSeaBaseInstall-{#MyAppVersion}
Compression=lzma
SolidCompression=yes

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 0,6.1

[Files]
Source: "K:\My\build\DeepSeaBase\release\DeepSeaBase.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\Qt\Qt5.2.1\5.2.1\mingw48_32\bin\icudt51.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\Qt\Qt5.2.1\5.2.1\mingw48_32\bin\icuin51.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\Qt\Qt5.2.1\5.2.1\mingw48_32\bin\icuuc51.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\Qt\Qt5.2.1\5.2.1\mingw48_32\bin\libgcc_s_dw2-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\Qt\Qt5.2.1\5.2.1\mingw48_32\bin\libstdc++-6.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\Qt\Qt5.2.1\5.2.1\mingw48_32\bin\libwinpthread-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\Qt\Qt5.2.1\5.2.1\mingw48_32\bin\Qt5Core.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\Qt\Qt5.2.1\5.2.1\mingw48_32\bin\Qt5Gui.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\Qt\Qt5.2.1\5.2.1\mingw48_32\bin\Qt5OpenGL.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\Qt\Qt5.2.1\5.2.1\mingw48_32\bin\Qt5PrintSupport.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\Qt\Qt5.2.1\5.2.1\mingw48_32\bin\Qt5Svg.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\Qt\Qt5.2.1\5.2.1\mingw48_32\bin\Qt5Widgets.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\My\build\DeepSeaBase\release\qwt.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\Qt\Qt5.2.1\5.2.1\mingw48_32\plugins\iconengines\*.dll"; DestDir: "{app}\iconengines"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"
Source: "K:\Qt\Qt5.2.1\5.2.1\mingw48_32\plugins\imageformats\*.dll"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"
Source: "K:\Qt\Qt5.2.1\5.2.1\mingw48_32\plugins\platforms\*.dll"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"
Source: "K:\Qt\Qt5.2.1\5.2.1\mingw48_32\plugins\printsupport\*.dll"; DestDir: "{app}\printsupport"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: quicklaunchicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
