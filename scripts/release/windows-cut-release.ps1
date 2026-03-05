[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Version,
    [string]$SolutionDir,
    [string]$Configuration = "Release",
    [string]$Platform = "x64",
    [string]$ProductName = "Kronos",
    [string]$OutputRoot,
    [string]$UserGuideSource,
    [string]$InnoCompilerPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ($Version -notmatch '^[0-9]+\.[0-9]+\.[0-9]+(-beta\.[0-9]+)?$') {
    throw "Invalid version '$Version'. Expected x.y.z or x.y.z-beta.N"
}

if ([string]::IsNullOrWhiteSpace($SolutionDir)) {
    $scriptRoot = Split-Path -Parent $PSScriptRoot
    $SolutionDir = (Resolve-Path (Join-Path $scriptRoot "..\.." )).Path
}

$SolutionDir = [System.IO.Path]::GetFullPath($SolutionDir)
$coreVersion = ($Version -split '-', 2)[0]

$parts = $coreVersion.Split('.')
$major = [int]$parts[0]
$minor = [int]$parts[1]
$patch = [int]$parts[2]
$versionCodeDec = ($major * 65536) + ($minor * 256) + $patch
$versionCodeHex = ('0x{0:x}' -f $versionCodeDec)

function Set-FileContent {
    param(
        [string]$Path,
        [string]$Content
    )

    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $Content, $utf8NoBom)
}

function Update-RegexValue {
    param(
        [string]$Path,
        [hashtable]$Rules
    )

    if (-not (Test-Path $Path)) {
        throw "Missing file: $Path"
    }

    $text = Get-Content -Raw -Path $Path
    foreach ($key in $Rules.Keys) {
        $replacement = $Rules[$key]
        $text = [regex]::Replace($text, $key, $replacement)
    }

    Set-FileContent -Path $Path -Content $text
}

$jucerPath = Join-Path $SolutionDir "Kronos.jucer"
if (Test-Path $jucerPath) {
    $jucer = Get-Content -Raw -Path $jucerPath
    if ($jucer -match '<JUCERPROJECT\b[^>]*\sversion="[^"]*"') {
        $jucer = [regex]::Replace($jucer, '(<JUCERPROJECT\b[^>]*?)\sversion="[^"]*"', ('$1 version="{0}"' -f $coreVersion))
    }
    else {
        $jucer = [regex]::Replace($jucer, '(<JUCERPROJECT\b)', ('$1 version="{0}"' -f $coreVersion), 1)
    }
    Set-FileContent -Path $jucerPath -Content $jucer
}

$definesPath = Join-Path $SolutionDir "JuceLibraryCode\JucePluginDefines.h"
Update-RegexValue -Path $definesPath -Rules @{
    'JucePlugin_Version\s+[0-9]+\.[0-9]+\.[0-9]+' = ('JucePlugin_Version                {0}' -f $coreVersion)
    'JucePlugin_VersionCode\s+0x[0-9a-fA-F]+' = ('JucePlugin_VersionCode            {0}' -f $versionCodeHex)
    'JucePlugin_VersionString\s+"[0-9]+\.[0-9]+\.[0-9]+"' = ('JucePlugin_VersionString          "{0}"' -f $coreVersion)
    'JUCE_APP_VERSION\s+[0-9]+\.[0-9]+\.[0-9]+' = ('JUCE_APP_VERSION={0}' -f $coreVersion)
    'JUCE_APP_VERSION_HEX\s+0x[0-9a-fA-F]+' = ('JUCE_APP_VERSION_HEX={0}' -f $versionCodeHex)
    'aradocumentarchive\.[0-9]+\.[0-9]+\.[0-9]+' = ('aradocumentarchive.{0}' -f $coreVersion)
}

$headerPath = Join-Path $SolutionDir "JuceLibraryCode\JuceHeader.h"
if (Test-Path $headerPath) {
    Update-RegexValue -Path $headerPath -Rules @{
        'versionString\s+=\s+"[0-9]+\.[0-9]+\.[0-9]+"' = ('versionString  = "{0}"' -f $coreVersion)
        'versionNumber\s+=\s+0x[0-9a-fA-F]+' = ('versionNumber  = {0}' -f $versionCodeHex)
    }
}

$vcxprojFiles = Get-ChildItem -Path (Join-Path $SolutionDir "Builds\VisualStudio2022") -Filter *.vcxproj -File
foreach ($vcxproj in $vcxprojFiles) {
    Update-RegexValue -Path $vcxproj.FullName -Rules @{
        'JucePlugin_Version=[0-9]+\.[0-9]+\.[0-9]+' = ('JucePlugin_Version={0}' -f $coreVersion)
        'JucePlugin_VersionCode=0x[0-9a-fA-F]+' = ('JucePlugin_VersionCode={0}' -f $versionCodeHex)
        'JucePlugin_VersionString=(?:&quot;|\\&quot;)[0-9]+\.[0-9]+\.[0-9]+(?:&quot;|\\&quot;)' = ('JucePlugin_VersionString=&quot;{0}&quot;' -f $coreVersion)
        'JUCE_APP_VERSION=[0-9]+\.[0-9]+\.[0-9]+' = ('JUCE_APP_VERSION={0}' -f $coreVersion)
        'JUCE_APP_VERSION_HEX=0x[0-9a-fA-F]+' = ('JUCE_APP_VERSION_HEX={0}' -f $versionCodeHex)
        '-version &quot;[0-9]+\.[0-9]+\.[0-9]+&quot;' = ('-version &quot;{0}&quot;' -f $coreVersion)
    }
}

Write-Host "[windows-cut-release] Updated version metadata to core version $coreVersion"

$msbuild = Get-Command msbuild -ErrorAction SilentlyContinue
if (-not $msbuild) {
    throw "msbuild not found on PATH. Run from Developer PowerShell for VS2022 or install Build Tools."
}

$vst3Proj = Join-Path $SolutionDir "Builds\VisualStudio2022\Kronos_VST3.vcxproj"
$standaloneProj = Join-Path $SolutionDir "Builds\VisualStudio2022\Kronos_StandalonePlugin.vcxproj"

try {
    # Avoid duplicate packaging when VS post-build hooks are enabled.
    $previousAutoRelease = $null
    if (Test-Path Env:KRONOS_WINDOWS_AUTO_RELEASE) {
        $previousAutoRelease = $env:KRONOS_WINDOWS_AUTO_RELEASE
    }

    $env:KRONOS_WINDOWS_AUTO_RELEASE = "0"

    & $msbuild.Source $vst3Proj "/m" "/p:Configuration=$Configuration;Platform=$Platform"
    if ($LASTEXITCODE -ne 0) {
        throw "VST3 build failed with exit code $LASTEXITCODE"
    }

    & $msbuild.Source $standaloneProj "/m" "/p:Configuration=$Configuration;Platform=$Platform"
    if ($LASTEXITCODE -ne 0) {
        throw "Standalone build failed with exit code $LASTEXITCODE"
    }
}
finally {
    if ($null -eq $previousAutoRelease) {
        if (Test-Path Env:KRONOS_WINDOWS_AUTO_RELEASE) {
            Remove-Item Env:KRONOS_WINDOWS_AUTO_RELEASE
        }
    }
    else {
        $env:KRONOS_WINDOWS_AUTO_RELEASE = $previousAutoRelease
    }
}

$releaseScript = Join-Path $PSScriptRoot "windows-release.ps1"
$params = @{
    Version = $Version
    SolutionDir = $SolutionDir
    Configuration = $Configuration
    Platform = $Platform
    ProductName = $ProductName
    IgnoreAutoReleaseGuard = $true
}

if ($OutputRoot) { $params.OutputRoot = $OutputRoot }
if ($UserGuideSource) { $params.UserGuideSource = $UserGuideSource }
if ($InnoCompilerPath) { $params.InnoCompilerPath = $InnoCompilerPath }

& $releaseScript @params
if ($LASTEXITCODE -ne 0) {
    throw "windows-release.ps1 failed with exit code $LASTEXITCODE"
}
