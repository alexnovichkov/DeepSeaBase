; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "DeepSea Database"
#define MyAppPublisher "Novichkov & Sukin Sons."
#define MyAppExeName "DeepSeaBase.exe"
#define FileHandle  FileOpen("src/version.js")
#define FileLine StringChange(FileRead(FileHandle), "var _version=""","")
#define MyAppVersion  StringChange(FileLine, """;","")
#define PathToExe "E:\My\build\build-DeepSeaBase-Desktop_Qt_5_12_8_MinGW_32_bit-Release\bin"
#define PathToQt "K:\Qt\Qt5.12.8\5.12.8\mingw73_32"

#define WithTDMS

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
#ifdef WithTDMS
OutputBaseFilename=DeepSeaBaseInstall-{#MyAppVersion}-x32-TDMS
#else
OutputBaseFilename=DeepSeaBaseInstall-{#MyAppVersion}-x32
#endif
Compression=lzma
SolidCompression=yes
PrivilegesRequired=none
Uninstallable=IsTaskSelected('installmode/normal')

#ifdef WithTDMS
[Run]
Filename: "{app}\vc_redist.x86.exe"
#endif

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Tasks]
Name: installmode; Description: "����� ���������"; GroupDescription: "����� ���������"; 
Name: installmode/normal; Description: "������� ���������"; GroupDescription: "����� ���������"; Flags: exclusive
Name: installmode/portable; Description: "Portable ���������"; GroupDescription: "����� ���������"; Flags: unchecked exclusive
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: {#PathToExe}\DeepSeaBase.exe; DestDir: "{app}"; Flags: ignoreversion
Source: {#PathToQt}\bin\libgcc_s_dw2-1.dll; DestDir: "{app}"; Flags: ignoreversion
Source: {#PathToQt}\bin\libstdc++-6.dll; DestDir: "{app}"; Flags: ignoreversion
Source: {#PathToQt}\bin\libwinpthread-1.dll; DestDir: "{app}"; Flags: ignoreversion
Source: {#PathToQt}\bin\Qt5Core.dll; DestDir: "{app}"; Flags: ignoreversion
Source: {#PathToQt}\bin\Qt5Gui.dll; DestDir: "{app}"; Flags: ignoreversion
Source: {#PathToQt}\bin\Qt5OpenGL.dll; DestDir: "{app}"; Flags: ignoreversion
Source: {#PathToQt}\bin\Qt5PrintSupport.dll; DestDir: "{app}"; Flags: ignoreversion
Source: {#PathToQt}\bin\Qt5Svg.dll; DestDir: "{app}"; Flags: ignoreversion
Source: {#PathToQt}\bin\Qt5Widgets.dll; DestDir: "{app}"; Flags: ignoreversion
Source: {#PathToQt}\bin\Qt5WinExtras.dll; DestDir: "{app}"; Flags: ignoreversion
Source: {#PathToQt}\bin\Qt5Multimedia.dll; DestDir: "{app}"; Flags: ignoreversion
Source: {#PathToQt}\bin\Qt5Network.dll; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qwt-6.2.0-dev\x32\lib\qwt.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\My\build\DeepSeaBase\3rdParty\bin\samplerate.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\My\programming\sources\fftw-3.3.5-dll32\libfftw3-3.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\My\programming\ADS\ADSx32-release\lib\qtadvanceddocking.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: {#PathToQt}\plugins\iconengines\*.dll; DestDir: "{app}\iconengines"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"
Source: {#PathToQt}\plugins\imageformats\*.dll; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"
Source: {#PathToQt}\plugins\platforms\*.dll; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"
Source: {#PathToQt}\plugins\printsupport\*.dll; DestDir: "{app}\printsupport"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"
Source: {#PathToQt}\plugins\audio\*.dll; DestDir: "{app}\audio"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"
Source: {#PathToQt}\plugins\mediaservice\*.dll; DestDir: "{app}\mediaservice"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"
Source: {#PathToQt}\plugins\styles\*.dll; DestDir: "{app}\styles"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"

#ifdef WithTDMS
Source: "E:\My\programming\sources\TDMS\nilib-master\dll\*.dll"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "E:\My\programming\sources\TDMS\nilib-master\dll\DataModels\USI\1_0\usi_1_0.xsd"; DestDir: "{app}\DataModels\USI\1_0"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "E:\My\programming\sources\TDMS\nilib-master\dll\DataModels\USI\TDM\1_0\USI_TDM_1_0.xml"; DestDir: "{app}\DataModels\USI\TDM\1_0"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "E:\My\build\DeepSeaBase\vc_redist.x86.exe"; DestDir: "{app}"; Flags: ignoreversion
#endif

Source: "E:\My\build\DeepSeaBase\src\help.html"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\My\build\DeepSeaBase\src\graphs.htm"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\My\build\DeepSeaBase\src\files.html"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\My\build\DeepSeaBase\src\style.css"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\My\build\DeepSeaBase\src\version.js"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\My\build\DeepSeaBase\src\icons\*.png"; DestDir: "{app}\icons"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "E:\My\build\DeepSeaBase\src\icons\*.ico"; DestDir: "{app}\icons"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "E:\My\build\DeepSeaBase\src\icons\*.svg"; DestDir: "{app}\icons"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "E:\My\build\DeepSeaBase\portable"; DestDir: "{app}"; Tasks: installmode/portable


[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: installmode/normal
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"; Tasks: installmode/normal
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon  installmode/normal

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
