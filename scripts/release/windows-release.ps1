[CmdletBinding()]
param(
    [string]$Version,
    [string]$SolutionDir,
    [string]$BuildRoot,
    [string]$Configuration = "Release",
    [string]$Platform = "x64",
    [string]$ProductName = "Kronos",
    [string]$OutputRoot,
    [string]$UserGuideSource,
    [string]$InnoCompilerPath,
    [switch]$TriggeredByPostBuild,
    [switch]$IgnoreAutoReleaseGuard
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

try {
    if ($Configuration -ne "Release") {
        Write-Host "[windows-release] Skipping packaging for Configuration=$Configuration"
        exit 0
    }

    if (-not $IgnoreAutoReleaseGuard -and $env:KRONOS_WINDOWS_AUTO_RELEASE -eq "0") {
        Write-Host "[windows-release] Skipping packaging because KRONOS_WINDOWS_AUTO_RELEASE=0"
        exit 0
    }

    if ([string]::IsNullOrWhiteSpace($SolutionDir)) {
        $scriptRoot = Split-Path -Parent $PSScriptRoot
        $SolutionDir = (Resolve-Path (Join-Path $scriptRoot "..\.." )).Path
    }

    $SolutionDir = [System.IO.Path]::GetFullPath($SolutionDir)

    if ([string]::IsNullOrWhiteSpace($BuildRoot)) {
        $candidateBuildRoot = Join-Path $SolutionDir "Builds\VisualStudio2022"
        if (Test-Path $candidateBuildRoot) {
            $BuildRoot = $candidateBuildRoot
        }
        else {
            $BuildRoot = $SolutionDir
        }
    }

    $BuildRoot = [System.IO.Path]::GetFullPath($BuildRoot)

    if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
        $OutputRoot = Join-Path $SolutionDir "dist\windows"
    }

    if ([string]::IsNullOrWhiteSpace($UserGuideSource)) {
        $UserGuideSource = Join-Path $SolutionDir "Kronos_Manual\Kronos Manual\Kronos Manual.md"
    }

    if ([string]::IsNullOrWhiteSpace($Version)) {
        $definesPath = Join-Path $SolutionDir "JuceLibraryCode\JucePluginDefines.h"
        if (Test-Path $definesPath) {
            $defines = Get-Content -Raw -Path $definesPath
            $m = [regex]::Match($defines, 'JucePlugin_VersionString\s+"([^"]+)"')
            if ($m.Success) {
                $Version = $m.Groups[1].Value
            }
        }
    }

    if ([string]::IsNullOrWhiteSpace($Version)) {
        throw "Version is required. Pass -Version or ensure JucePlugin_VersionString exists in JuceLibraryCode/JucePluginDefines.h"
    }

    $releaseRoot = Join-Path $BuildRoot ("{0}\{1}" -f $Platform, $Configuration)
    $standalonePath = Join-Path $releaseRoot "Standalone Plugin\$ProductName.exe"
    $vst3Path = Join-Path $releaseRoot "VST3\$ProductName.vst3"

    $windowsTestScript = Join-Path $SolutionDir "scripts\run_tests_windows.ps1"
    if (-not (Test-Path $windowsTestScript)) {
        throw "[windows-release][tests] Windows test runner not found: $windowsTestScript"
    }

    Write-Host "[windows-release] Running embedded test suite gate (Release)..."
    try {
        & $windowsTestScript -Configuration $Configuration -Platform $Platform -SolutionDir $SolutionDir
    }
    catch {
        throw "[windows-release][tests] $($_.Exception.Message)"
    }

    function Wait-ForArtifacts {
        param(
            [int]$Seconds = 90
        )

        $sw = [System.Diagnostics.Stopwatch]::StartNew()
        while ($sw.Elapsed.TotalSeconds -lt $Seconds) {
            if ((Test-Path $standalonePath) -and (Test-Path $vst3Path)) {
                return $true
            }
            Start-Sleep -Seconds 2
        }

        return ((Test-Path $standalonePath) -and (Test-Path $vst3Path))
    }

    if (-not (Wait-ForArtifacts)) {
        $msg = "[windows-release] Release artifacts not ready yet. Missing: "
        if (-not (Test-Path $standalonePath)) { $msg += "Standalone " }
        if (-not (Test-Path $vst3Path)) { $msg += "VST3" }

        if ($TriggeredByPostBuild) {
            Write-Host $msg
            Write-Host "[windows-release] Post-build invocation will exit without failure."
            exit 0
        }

        throw $msg
    }

    $OutputRoot = [System.IO.Path]::GetFullPath($OutputRoot)
    $stagingRoot = Join-Path $OutputRoot ".staging\$Version"
    $lockDir = Join-Path $OutputRoot ".packaging.lock"

    New-Item -ItemType Directory -Path $OutputRoot -Force | Out-Null

    $lockAcquired = $false
    $lockStopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    while (-not $lockAcquired -and $lockStopwatch.Elapsed.TotalSeconds -lt 120) {
        try {
            New-Item -ItemType Directory -Path $lockDir -ErrorAction Stop | Out-Null
            $lockAcquired = $true
        }
        catch {
            Start-Sleep -Seconds 2
        }
    }

    if (-not $lockAcquired) {
        if ($TriggeredByPostBuild) {
            Write-Host "[windows-release] Another packaging run is in progress. Skipping this invocation."
            exit 0
        }

        throw "Unable to acquire packaging lock: $lockDir"
    }

    try {
        if (Test-Path $stagingRoot) {
            Remove-Item -Recurse -Force -Path $stagingRoot
        }

        $standaloneStage = Join-Path $stagingRoot "standalone"
        $vst3Stage = Join-Path $stagingRoot "vst3"
        $guideStage = Join-Path $stagingRoot "guide"

        New-Item -ItemType Directory -Path $standaloneStage -Force | Out-Null
        New-Item -ItemType Directory -Path $vst3Stage -Force | Out-Null
        New-Item -ItemType Directory -Path $guideStage -Force | Out-Null

        Copy-Item -Path $standalonePath -Destination (Join-Path $standaloneStage "$ProductName.exe") -Force
        Copy-Item -Path $vst3Path -Destination (Join-Path $vst3Stage "$ProductName.vst3") -Recurse -Force

        if (-not (Test-Path $UserGuideSource)) {
            throw "User guide source not found: $UserGuideSource"
        }

        $renderUserGuideScript = Join-Path $SolutionDir "scripts\release\render-user-guide.ps1"
        if (-not (Test-Path $renderUserGuideScript)) {
            throw "User guide renderer not found: $renderUserGuideScript"
        }

        & $renderUserGuideScript -SourceMarkdown $UserGuideSource -OutputDir $guideStage -ProductName $ProductName

        if ([string]::IsNullOrWhiteSpace($InnoCompilerPath)) {
            if (-not [string]::IsNullOrWhiteSpace($env:KRONOS_INNO_COMPILER)) {
                $InnoCompilerPath = $env:KRONOS_INNO_COMPILER
            }
            elseif (Get-Command iscc.exe -ErrorAction SilentlyContinue) {
                $InnoCompilerPath = (Get-Command iscc.exe -ErrorAction SilentlyContinue).Source
            }
            elseif (Test-Path "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe") {
                $InnoCompilerPath = "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe"
            }
            elseif (Test-Path "${env:ProgramFiles}\Inno Setup 6\ISCC.exe") {
                $InnoCompilerPath = "${env:ProgramFiles}\Inno Setup 6\ISCC.exe"
            }
            elseif (Test-Path "${env:ProgramFiles(x86)}\Inno Setup 5\ISCC.exe") {
                $InnoCompilerPath = "${env:ProgramFiles(x86)}\Inno Setup 5\ISCC.exe"
            }
            elseif (Test-Path "${env:ProgramFiles}\Inno Setup 5\ISCC.exe") {
                $InnoCompilerPath = "${env:ProgramFiles}\Inno Setup 5\ISCC.exe"
            }
        }

        if ([string]::IsNullOrWhiteSpace($InnoCompilerPath) -or -not (Test-Path $InnoCompilerPath)) {
            throw "Inno Setup compiler not found. Set -InnoCompilerPath or KRONOS_INNO_COMPILER to ISCC.exe"
        }

        $outputExeName = "$ProductName-$Version-setup"
        $issPath = Join-Path $stagingRoot "$ProductName-$Version.iss"

        $escapedProductName = $ProductName.Replace('"', '""')
        $escapedVersion = $Version.Replace('"', '""')
        $escapedOutputRoot = $OutputRoot.Replace('"', '""')
        $escapedStagingRoot = $stagingRoot.Replace('"', '""')
        $escapedExeName = $outputExeName.Replace('"', '""')

        $iss = @"
[Setup]
AppId={{${escapedProductName}-Installer}}
AppName=${escapedProductName}
AppVersion=${escapedVersion}
DefaultDirName={pf}\${escapedProductName}
DefaultGroupName=${escapedProductName}
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
OutputDir=${escapedOutputRoot}
OutputBaseFilename=${escapedExeName}
Compression=lzma
SolidCompression=yes
WizardStyle=modern
DisableDirPage=yes

[Types]
Name: "full"; Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "standalone"; Description: "Standalone app"; Types: full custom
Name: "vst3"; Description: "VST3 plugin"; Types: full custom
Name: "guide"; Description: "User Guide"; Types: full custom

[Files]
Source: "${escapedStagingRoot}\standalone\*"; DestDir: "{code:GetStandaloneInstallDir}"; Flags: recursesubdirs createallsubdirs ignoreversion; Components: standalone
Source: "${escapedStagingRoot}\vst3\*"; DestDir: "{code:GetVst3InstallDir}"; Flags: recursesubdirs createallsubdirs ignoreversion; Components: vst3
Source: "${escapedStagingRoot}\guide\*"; DestDir: "{code:GetStandaloneInstallDir}\User Guide"; Flags: recursesubdirs createallsubdirs ignoreversion; Components: guide

[Code]
var
  StandaloneDirPage: TInputDirWizardPage;
  Vst3DirPage: TInputDirWizardPage;

function IsStandaloneRelatedSelected: Boolean;
begin
  Result := IsComponentSelected('standalone') or IsComponentSelected('guide');
end;

procedure InitializeWizard;
begin
  StandaloneDirPage := CreateInputDirPage(
    wpSelectComponents,
    'Standalone Install Location',
    'Choose where to install the standalone app.',
    'Select the destination folder for the standalone app and user guide files.',
    False,
    ''
  );
  StandaloneDirPage.Add('Standalone destination folder:');
  StandaloneDirPage.Values[0] := ExpandConstant('{pf}\${escapedProductName}');

  Vst3DirPage := CreateInputDirPage(
    StandaloneDirPage.ID,
    'VST3 Install Location',
    'Choose where to install the VST3 plugin.',
    'Select the parent VST3 folder. The installer will place ${escapedProductName}.vst3 inside this location.',
    False,
    ''
  );
  Vst3DirPage.Add('VST3 root folder:');
  Vst3DirPage.Values[0] := ExpandConstant('{cf}\VST3');
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := False;
  if (PageID = StandaloneDirPage.ID) and (not IsStandaloneRelatedSelected()) then
    Result := True
  else if (PageID = Vst3DirPage.ID) and (not IsComponentSelected('vst3')) then
    Result := True;
end;

function GetStandaloneInstallDir(Value: string): string;
begin
  if Assigned(StandaloneDirPage) and (StandaloneDirPage.Values[0] <> '') then
    Result := StandaloneDirPage.Values[0]
  else
    Result := ExpandConstant('{pf}\${escapedProductName}');
end;

function GetVst3InstallDir(Value: string): string;
begin
  if Assigned(Vst3DirPage) and (Vst3DirPage.Values[0] <> '') then
    Result := Vst3DirPage.Values[0]
  else
    Result := ExpandConstant('{cf}\VST3');
end;
"@

        $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
        [System.IO.File]::WriteAllText($issPath, $iss, $utf8NoBom)

        & $InnoCompilerPath $issPath | Out-Host
        if ($LASTEXITCODE -ne 0) {
            throw "Inno Setup build failed with exit code $LASTEXITCODE"
        }

        $installerPath = Join-Path $OutputRoot ($outputExeName + ".exe")
        if (-not (Test-Path $installerPath)) {
            throw "Expected installer not found: $installerPath"
        }

        $hash = (Get-FileHash -Path $installerPath -Algorithm SHA256).Hash.ToLowerInvariant()
        $hashFile = "$installerPath.sha256"
        Set-Content -Path $hashFile -Value ("{0}  {1}" -f $hash, $installerPath) -Encoding UTF8

        Write-Host "[windows-release] Installer: $installerPath"
        Write-Host "[windows-release] Checksum:  $hashFile"
    }
    finally {
        if (Test-Path $lockDir) {
            Remove-Item -Recurse -Force -Path $lockDir
        }
    }
}
catch {
    $isTestGateFailure = $_.Exception.Message.StartsWith("[windows-release][tests]")

    if ($TriggeredByPostBuild -and -not $isTestGateFailure) {
        Write-Host "[windows-release] Post-build packaging failed but will not fail the build."
        Write-Host "[windows-release] Error: $($_.Exception.Message)"
        exit 0
    }

    throw
}
