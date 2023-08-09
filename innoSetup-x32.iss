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
#define CurrentDate GetDateTimeString('dd/mm/yyyy', '.', '');

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
OutputBaseFilename=DeepSeaBaseInstall-{#MyAppVersion}-{#CurrentDate}-x32
Compression=lzma
SolidCompression=yes
PrivilegesRequired=none
Uninstallable=IsTaskSelected('installmode/normal')

;[Run]
;Filename: "{app}\vc_redist.x86.exe"; Components: plugins

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[CustomMessages]
russian.Main=�������� ����� ���������
russian.Plugins=������ TDMS (National Instruments)

[Tasks]
Name: installmode; Description: "����� ���������"; GroupDescription: "����� ���������"; 
Name: installmode/normal; Description: "������� ���������"; GroupDescription: "����� ���������"; Flags: exclusive
Name: installmode/portable; Description: "Portable ���������"; GroupDescription: "����� ���������"; Flags: unchecked exclusive
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Components]
Name: "main"; Description: "{cm:Main}"; Types: full compact custom; Flags: fixed
Name: "plugins"; Description: "{cm:Plugins}"; Types: custom full

[Files]
Source: {#PathToExe}\DeepSeaBase.exe; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: {#PathToQt}\bin\libgcc_s_dw2-1.dll; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: {#PathToQt}\bin\libstdc++-6.dll; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: {#PathToQt}\bin\libwinpthread-1.dll; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: {#PathToQt}\bin\Qt5Core.dll; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: {#PathToQt}\bin\Qt5Gui.dll; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: {#PathToQt}\bin\Qt5OpenGL.dll; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: {#PathToQt}\bin\Qt5PrintSupport.dll; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: {#PathToQt}\bin\Qt5Svg.dll; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: {#PathToQt}\bin\Qt5Widgets.dll; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: {#PathToQt}\bin\Qt5WinExtras.dll; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: {#PathToQt}\bin\Qt5Multimedia.dll; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: {#PathToQt}\bin\Qt5Network.dll; DestDir: "{app}"; Flags: ignoreversion; Components: main
;Source: "C:\Qwt-6.2.0-dev\x32\lib\qwt.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "E:\My\build\DeepSeaBase\3rdParty\bin\samplerate.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "E:\My\programming\sources\fftw-3.3.5-dll32\libfftw3-3.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "E:\My\programming\ADS\ADSx32-release\lib\qtadvanceddocking.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: {#PathToQt}\plugins\iconengines\*.dll; DestDir: "{app}\iconengines"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"; Components: main
Source: {#PathToQt}\plugins\imageformats\*.dll; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"; Components: main
Source: {#PathToQt}\plugins\platforms\*.dll; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"; Components: main
Source: {#PathToQt}\plugins\printsupport\*.dll; DestDir: "{app}\printsupport"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"; Components: main
Source: {#PathToQt}\plugins\audio\*.dll; DestDir: "{app}\audio"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"; Components: main
Source: {#PathToQt}\plugins\mediaservice\*.dll; DestDir: "{app}\mediaservice"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"; Components: main
Source: {#PathToQt}\plugins\styles\*.dll; DestDir: "{app}\styles"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*d.dll"; Components: main

Source: {#PathToExe+"/plugins/*.dll"}; DestDir: "{app}\plugins"; Flags: ignoreversion  createallsubdirs recursesubdirs overwritereadonly uninsremovereadonly; Attribs: readonly; Components: plugins
Source: "E:\My\programming\sources\TDMS\nilib-master\dll\*.dll"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: plugins
Source: "E:\My\programming\sources\TDMS\nilib-master\dll\DataModels\USI\1_0\usi_1_0.xsd"; DestDir: "{app}\DataModels\USI\1_0"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: plugins
Source: "E:\My\programming\sources\TDMS\nilib-master\dll\DataModels\USI\TDM\1_0\USI_TDM_1_0.xml"; DestDir: "{app}\DataModels\USI\TDM\1_0"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: plugins
;Source: "E:\My\build\DeepSeaBase\vc_redist.x86.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: plugins
Source: "E:\My\build\DeepSeaBase\3rdParty\easylogging\*.conf"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "E:\My\build\DeepSeaBase\src\DeepSeaBase-logging.bat"; DestDir: "{app}"; Flags: ignoreversion; Components: main

Source: "E:\My\build\DeepSeaBase\src\*.html"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "E:\My\build\DeepSeaBase\src\style.css"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "E:\My\build\DeepSeaBase\src\ads.css"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "E:\My\build\DeepSeaBase\src\ads-linux.css"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "E:\My\build\DeepSeaBase\src\version.js"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "E:\My\build\DeepSeaBase\src\icons\*.png"; DestDir: "{app}\icons"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: main
Source: "E:\My\build\DeepSeaBase\src\icons\*.ico"; DestDir: "{app}\icons"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: main
Source: "E:\My\build\DeepSeaBase\src\icons\*.svg"; DestDir: "{app}\icons"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: main
Source: "E:\My\build\DeepSeaBase\portable"; DestDir: "{app}"; Tasks: installmode/portable; Components: main


[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: installmode/normal
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"; Tasks: installmode/normal
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon  installmode/normal

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
