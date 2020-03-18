; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "DeepSea Database"
#define MyAppVersion "1.6.9.2"
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
Source: "K:\My\build\build-DeepSeaBase-Desktop_Qt_5_10_1_MinGW_32bit-Release\bin\DeepSeaBase.exe"; DestDir: "{app}"; Flags: ignoreversion
;Source: "K:\Qt\Qt5.5.0\5.5\mingw492_32\bin\icudt54.dll"; DestDir: "{app}"; Flags: ignoreversion
;Source: "K:\Qt\Qt5.5.0\5.5\mingw492_32\bin\icuin54.dll"; DestDir: "{app}"; Flags: ignoreversion
;Source: "K:\Qt\Qt5.5.0\5.5\mingw492_32\bin\icuuc54.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\Qt\Qt5.10.1\5.10.1\mingw53_32\bin\libgcc_s_dw2-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\Qt\Qt5.10.1\5.10.1\mingw53_32\bin\libstdc++-6.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\Qt\Qt5.10.1\5.10.1\mingw53_32\bin\libwinpthread-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\Qt\Qt5.10.1\5.10.1\mingw53_32\bin\Qt5Core.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\Qt\Qt5.10.1\5.10.1\mingw53_32\bin\Qt5Gui.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\Qt\Qt5.10.1\5.10.1\mingw53_32\bin\Qt5OpenGL.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\Qt\Qt5.10.1\5.10.1\mingw53_32\bin\Qt5PrintSupport.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\Qt\Qt5.10.1\5.10.1\mingw53_32\bin\Qt5Svg.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\Qt\Qt5.10.1\5.10.1\mingw53_32\bin\Qt5Widgets.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\Qt\Qt5.10.1\5.10.1\mingw53_32\bin\Qt5WinExtras.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\Qt\Qt5.10.1\5.10.1\mingw53_32\bin\Qt5Multimedia.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\Qt\Qt5.10.1\5.10.1\mingw53_32\bin\Qt5Network.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qwt-6.1.2\lib\qwt.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\My\build\DeepSeaBase\3rdParty\bin\samplerate.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\Qt\Qt5.10.1\5.10.1\mingw53_32\plugins\iconengines\*.dll"; DestDir: "{app}\iconengines"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"
Source: "K:\Qt\Qt5.10.1\5.10.1\mingw53_32\plugins\imageformats\*.dll"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"
Source: "K:\Qt\Qt5.10.1\5.10.1\mingw53_32\plugins\platforms\*.dll"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"
Source: "K:\Qt\Qt5.10.1\5.10.1\mingw53_32\plugins\printsupport\*.dll"; DestDir: "{app}\printsupport"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"

Source: "K:\My\build\DeepSeaBase\src\help.html"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\My\build\DeepSeaBase\src\graphs.htm"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\My\build\DeepSeaBase\src\files.html"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\My\build\DeepSeaBase\src\style.css"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\My\build\DeepSeaBase\src\version.js"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\My\build\DeepSeaBase\src\icons\*.png"; DestDir: "{app}\icons"; Flags: ignoreversion recursesubdirs createallsubdirs
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: quicklaunchicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
