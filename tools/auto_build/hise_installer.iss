#define MyAppName "HISE"
#ifndef MyAppVersion
  #define MyAppVersion "0.0.0"
#endif
#ifndef MyOutputVersion
  #define MyOutputVersion "0_0_0"
#endif
#ifndef MyHiseExe
  #define MyHiseExe "..\\..\\projects\\standalone\\Builds\\VisualStudio2026\\x64\\Release\\App\\HISE.exe"
#endif

[Setup]
AppId={{9E18A9A9-6ADF-4F86-B20B-BEE81023F1FC}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher=HISE
DefaultDirName={autopf}\HISE
DefaultGroupName=HISE
ArchitecturesInstallIn64BitMode=x64
SetupLogging=yes
OutputBaseFilename=HISE_{#MyOutputVersion}_Windows
Compression=lzma
SolidCompression=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "{#MyHiseExe}"; DestDir: "{app}"; Flags: ignoreversion 64bit

[Icons]
Name: "{group}\HISE"; Filename: "{app}\HISE.exe"
