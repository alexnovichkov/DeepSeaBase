; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "DeepSea Database"
#define MyAppPublisher "Novichkov & Sukin Sons."
#define MyAppExeName "DeepSeaBase.exe"
#define FileHandle  FileOpen("src/version.js")
#define FileLine StringChange(FileRead(FileHandle), "var _version=""","")
#define MyAppVersion  StringChange(FileLine, """;","")

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
OutputBaseFilename=DeepSeaBaseInstall-{#MyAppVersion}-x64
Compression=lzma
SolidCompression=yes
ArchitecturesAllowed=x64 ia64
ArchitecturesInstallIn64BitMode=x64 ia64

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 0,6.1

[Files]
Source: "K:\My\build\build-DeepSeaBase-Desktop_Qt_5_12_8_MinGW_64_bit-Release\bin\DeepSeaBase.exe"; DestDir: "{app}"; Flags: ignoreversion
;Source: "K:\Qt\Qt5.5.0\5.5\mingw492_32\bin\icudt54.dll"; DestDir: "{app}"; Flags: ignoreversion
;Source: "K:\Qt\Qt5.5.0\5.5\mingw492_32\bin\icuin54.dll"; DestDir: "{app}"; Flags: ignoreversion
;Source: "K:\Qt\Qt5.5.0\5.5\mingw492_32\bin\icuuc54.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qt\Qt5.12.8\5.12.8\mingw73_64\bin\libgcc_s_seh-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qt\Qt5.12.8\5.12.8\mingw73_64\bin\libstdc++-6.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qt\Qt5.12.8\5.12.8\mingw73_64\bin\libwinpthread-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qt\Qt5.12.8\5.12.8\mingw73_64\bin\Qt5Core.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qt\Qt5.12.8\5.12.8\mingw73_64\bin\Qt5Gui.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qt\Qt5.12.8\5.12.8\mingw73_64\bin\Qt5OpenGL.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qt\Qt5.12.8\5.12.8\mingw73_64\bin\Qt5PrintSupport.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qt\Qt5.12.8\5.12.8\mingw73_64\bin\Qt5Svg.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qt\Qt5.12.8\5.12.8\mingw73_64\bin\Qt5Widgets.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qt\Qt5.12.8\5.12.8\mingw73_64\bin\Qt5WinExtras.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qt\Qt5.12.8\5.12.8\mingw73_64\bin\Qt5Multimedia.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qt\Qt5.12.8\5.12.8\mingw73_64\bin\Qt5Network.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qwt-6.4.0-svn\lib64\qwt.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\My\programming\sources\build-libsamplerate-0.1.8-Desktop_Qt_5_12_8_MinGW_64_bit-Release\release\samplerate.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "K:\My\programming\sources\fftw-3.3.5-dll64\libfftw3-3.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qt\Qt5.12.8\5.12.8\mingw73_64\plugins\iconengines\*.dll"; DestDir: "{app}\iconengines"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"
Source: "C:\Qt\Qt5.12.8\5.12.8\mingw73_64\plugins\imageformats\*.dll"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"
Source: "C:\Qt\Qt5.12.8\5.12.8\mingw73_64\plugins\platforms\*.dll"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"
Source: "C:\Qt\Qt5.12.8\5.12.8\mingw73_64\plugins\printsupport\*.dll"; DestDir: "{app}\printsupport"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"
Source: "C:\Qt\Qt5.12.8\5.12.8\mingw73_64\plugins\audio\*.dll"; DestDir: "{app}\audio"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"
Source: "C:\Qt\Qt5.12.8\5.12.8\mingw73_64\plugins\mediaservice\*.dll"; DestDir: "{app}\mediaservice"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"

Source: "K:\My\programming\sources\TDMS\tdm_dev\dev\bin\64-bit\*.dll"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "K:\My\programming\sources\TDMS\tdm_dev\dev\bin\64-bit\DataModels\USI\1_0\usi_1_0.xsd"; DestDir: "{app}\DataModels\USI\1_0"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "K:\My\programming\sources\TDMS\tdm_dev\dev\bin\64-bit\DataModels\USI\TDM\1_0\USI_TDM_1_0.xml"; DestDir: "{app}\DataModels\USI\TDM\1_0"; Flags: ignoreversion recursesubdirs createallsubdirs

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
