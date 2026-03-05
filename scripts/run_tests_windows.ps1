[CmdletBinding()]
param(
    [string]$Configuration = "Debug",
    [string]$SolutionDir,
    [string]$Platform = "x64",
    [switch]$SkipBuild,
    [string]$AppBinary,
    [bool]$UseIsolatedBuildOutput = $true,
    [int]$TimeoutSeconds = 120
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-MSBuildPath {
    $msbuild = Get-Command msbuild -ErrorAction SilentlyContinue
    if ($msbuild) {
        return $msbuild.Source
    }

    $vswherePath = Join-Path "${env:ProgramFiles(x86)}" "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswherePath) {
        $resolved = & $vswherePath -latest -products * -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
        if (-not [string]::IsNullOrWhiteSpace($resolved) -and (Test-Path $resolved)) {
            return $resolved
        }
    }

    return $null
}

if ([string]::IsNullOrWhiteSpace($SolutionDir)) {
    $SolutionDir = (Resolve-Path (Join-Path $PSScriptRoot ".." )).Path
}

$SolutionDir = [System.IO.Path]::GetFullPath($SolutionDir)
$standaloneProject = Join-Path $SolutionDir "Builds\VisualStudio2022\Kronos_StandalonePlugin.vcxproj"
$sharedCodeProject = Join-Path $SolutionDir "Builds\VisualStudio2022\Kronos_SharedCode.vcxproj"

if (-not (Test-Path $standaloneProject)) {
    throw "Standalone project not found: $standaloneProject"
}

if (-not (Test-Path $sharedCodeProject)) {
    throw "Shared code project not found: $sharedCodeProject"
}

if (-not $SkipBuild) {
    $msbuildPath = Resolve-MSBuildPath
    if ([string]::IsNullOrWhiteSpace($msbuildPath)) {
        throw "msbuild not found (PATH/vswhere). Run from Developer PowerShell for VS2022 or install Build Tools + MSBuild."
    }

    $isolatedBuildRoot = $null
    $isolatedSharedOutDir = $null
    $isolatedSharedIntDir = $null
    $isolatedStandaloneOutDir = $null
    $isolatedStandaloneIntDir = $null
    $isolatedSharedOverrideProps = $null
    $isolatedStandaloneOverrideProps = $null

    if ($UseIsolatedBuildOutput) {
        $isolatedBuildId = [Guid]::NewGuid().ToString("N")
        $isolatedBuildRoot = Join-Path $SolutionDir ("Builds\VisualStudio2022\.test-build\{0}" -f $isolatedBuildId)

        $isolatedSharedOutDir = Join-Path $isolatedBuildRoot ("{0}\{1}\SharedCode\" -f $Platform, $Configuration)
        $isolatedSharedIntDir = Join-Path $isolatedBuildRoot ("obj\{0}\{1}\SharedCode\" -f $Platform, $Configuration)
        $isolatedStandaloneOutDir = Join-Path $isolatedBuildRoot ("{0}\{1}\StandalonePlugin\" -f $Platform, $Configuration)
        $isolatedStandaloneIntDir = Join-Path $isolatedBuildRoot ("obj\{0}\{1}\StandalonePlugin\" -f $Platform, $Configuration)

        New-Item -ItemType Directory -Path $isolatedSharedOutDir -Force | Out-Null
        New-Item -ItemType Directory -Path $isolatedSharedIntDir -Force | Out-Null
        New-Item -ItemType Directory -Path $isolatedStandaloneOutDir -Force | Out-Null
        New-Item -ItemType Directory -Path $isolatedStandaloneIntDir -Force | Out-Null

        $isolatedSharedOverrideProps = Join-Path $isolatedBuildRoot "sharedcode-test-overrides.props"
        $isolatedStandaloneOverrideProps = Join-Path $isolatedBuildRoot "standalone-test-overrides.props"

        $sharedOverrideXml = @"
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemDefinitionGroup>
    <ClCompile>
      <PreprocessorDefinitions>JUCE_UNIT_TESTS=1;%(PreprocessorDefinitions)</PreprocessorDefinitions>
    </ClCompile>
    <ResourceCompile>
      <PreprocessorDefinitions>JUCE_UNIT_TESTS=1;%(PreprocessorDefinitions)</PreprocessorDefinitions>
    </ResourceCompile>
  </ItemDefinitionGroup>
</Project>
"@

        $escapedSharedOutDir = $isolatedSharedOutDir.Replace("&", "&amp;")
        $standaloneOverrideXml = @"
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemDefinitionGroup>
    <Link>
      <AdditionalLibraryDirectories>${escapedSharedOutDir};%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>
      <AdditionalDependencies>KronosTests.lib;%(AdditionalDependencies)</AdditionalDependencies>
    </Link>
  </ItemDefinitionGroup>
</Project>
"@

        Set-Content -Path $isolatedSharedOverrideProps -Value $sharedOverrideXml -Encoding UTF8
        Set-Content -Path $isolatedStandaloneOverrideProps -Value $standaloneOverrideXml -Encoding UTF8

        Write-Host "Using isolated test build output: $isolatedBuildRoot"
    }

    $previousCL = $null
    $hadCL = Test-Path Env:CL
    if ($hadCL) {
        $previousCL = $env:CL
    }

    $previousAutoRelease = $null
    $hadAutoRelease = Test-Path Env:KRONOS_WINDOWS_AUTO_RELEASE
    if ($hadAutoRelease) {
        $previousAutoRelease = $env:KRONOS_WINDOWS_AUTO_RELEASE
    }

    try {
        if ($hadCL -and $previousCL -match '(^|\s)/D\s*JUCE_UNIT_TESTS=1(\s|$)') {
            $env:CL = $previousCL
        }
        elseif ($hadCL -and $previousCL -match '(^|\s)/DJUCE_UNIT_TESTS=1(\s|$)') {
            $env:CL = $previousCL
        }
        elseif ($hadCL) {
            $env:CL = "/DJUCE_UNIT_TESTS=1 $previousCL"
        }
        else {
            $env:CL = "/DJUCE_UNIT_TESTS=1"
        }

        # Prevent Release post-build packaging while running the dedicated test build.
        $env:KRONOS_WINDOWS_AUTO_RELEASE = "0"

        # Force a full rebuild so JUCE_UNIT_TESTS=1 is applied where test code is compiled
        # (SharedCode), then rebuild the Standalone host that links against it.
        if ($UseIsolatedBuildOutput) {
            $sharedBuildProps = "Configuration={0};Platform={1};OutDir={2};IntDir={3};TargetName=KronosTests;ForceImportBeforeCppTargets={4}" -f $Configuration, $Platform, $isolatedSharedOutDir, $isolatedSharedIntDir, $isolatedSharedOverrideProps
            & $msbuildPath $sharedCodeProject "/m" "/t:Rebuild" "/p:$sharedBuildProps"
        }
        else {
            & $msbuildPath $sharedCodeProject "/m" "/t:Rebuild" "/p:Configuration=$Configuration;Platform=$Platform"
        }

        if ($LASTEXITCODE -ne 0) {
            throw "SharedCode test build failed with exit code $LASTEXITCODE"
        }

        if ($UseIsolatedBuildOutput) {
            $standaloneBuildProps = "Configuration={0};Platform={1};BuildProjectReferences=false;OutDir={2};IntDir={3};ForceImportBeforeCppTargets={4}" -f $Configuration, $Platform, $isolatedStandaloneOutDir, $isolatedStandaloneIntDir, $isolatedStandaloneOverrideProps
            & $msbuildPath $standaloneProject "/m" "/t:Rebuild" "/p:$standaloneBuildProps"
        }
        else {
            & $msbuildPath $standaloneProject "/m" "/t:Rebuild" "/p:Configuration=$Configuration;Platform=$Platform"
        }

        if ($LASTEXITCODE -ne 0) {
            throw "Standalone test build failed with exit code $LASTEXITCODE"
        }

        if ([string]::IsNullOrWhiteSpace($AppBinary) -and $UseIsolatedBuildOutput) {
            $AppBinary = Join-Path $isolatedStandaloneOutDir "Kronos.exe"
        }
    }
    finally {
        if ($hadCL) {
            $env:CL = $previousCL
        }
        else {
            if (Test-Path Env:CL) {
                Remove-Item Env:CL
            }
        }

        if ($hadAutoRelease) {
            $env:KRONOS_WINDOWS_AUTO_RELEASE = $previousAutoRelease
        }
        else {
            if (Test-Path Env:KRONOS_WINDOWS_AUTO_RELEASE) {
                Remove-Item Env:KRONOS_WINDOWS_AUTO_RELEASE
            }
        }
    }
}

if ([string]::IsNullOrWhiteSpace($AppBinary)) {
    $AppBinary = Join-Path $SolutionDir ("Builds\\VisualStudio2022\\{0}\\{1}\\Standalone Plugin\\Kronos.exe" -f $Platform, $Configuration)
}

if (-not (Test-Path $AppBinary)) {
    throw "Test runner app not found: $AppBinary"
}

$resultFile = Join-Path ([System.IO.Path]::GetTempPath()) ("kronos-tests-{0}.result" -f ([Guid]::NewGuid().ToString("N")))
$logFile = Join-Path ([System.IO.Path]::GetTempPath()) ("kronos-tests-{0}.log" -f ([Guid]::NewGuid().ToString("N")))
$logStreamOffset = 0L

function Write-NewTestLogLines {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][ref]$Offset
    )

    if (-not (Test-Path $Path)) {
        return
    }

    $length = (Get-Item $Path).Length
    if ($length -lt $Offset.Value) {
        $Offset.Value = 0L
    }

    if ($length -le $Offset.Value) {
        return
    }

    $stream = [System.IO.File]::Open($Path, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
    try {
        $stream.Seek($Offset.Value, [System.IO.SeekOrigin]::Begin) | Out-Null

        $reader = New-Object System.IO.StreamReader($stream, [System.Text.Encoding]::UTF8, $true, 1024, $true)
        try {
            while (-not $reader.EndOfStream) {
                $line = $reader.ReadLine()
                if ($null -ne $line) {
                    Write-Host $line
                }
            }

            $Offset.Value = $stream.Position
        }
        finally {
            $reader.Dispose()
        }
    }
    finally {
        $stream.Dispose()
    }
}

$previousTestMode = $null
$hadTestMode = Test-Path Env:KRONOS_TEST_MODE
if ($hadTestMode) {
    $previousTestMode = $env:KRONOS_TEST_MODE
}

$previousRunTests = $null
$hadRunTests = Test-Path Env:KRONOS_RUN_TESTS
if ($hadRunTests) {
    $previousRunTests = $env:KRONOS_RUN_TESTS
}

$previousResultPath = $null
$hadResultPath = Test-Path Env:KRONOS_TEST_RESULTS_FILE
if ($hadResultPath) {
    $previousResultPath = $env:KRONOS_TEST_RESULTS_FILE
}

$previousLogPath = $null
$hadLogPath = Test-Path Env:KRONOS_TEST_LOG_FILE
if ($hadLogPath) {
    $previousLogPath = $env:KRONOS_TEST_LOG_FILE
}

$process = $null
try {
    $env:KRONOS_TEST_MODE = "1"
    $env:KRONOS_RUN_TESTS = "1"
    $env:KRONOS_TEST_RESULTS_FILE = $resultFile
    $env:KRONOS_TEST_LOG_FILE = $logFile

    Write-Host "Running embedded JUCE tests..."
    New-Item -ItemType File -Path $logFile -Force | Out-Null
    $process = Start-Process -FilePath $AppBinary -WorkingDirectory (Split-Path -Parent $AppBinary) -PassThru

    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        Write-NewTestLogLines -Path $logFile -Offset ([ref]$logStreamOffset)

        if ((Test-Path $resultFile) -and ((Get-Item $resultFile).Length -gt 0)) {
            break
        }

        if ($null -ne $process -and $process.HasExited -and (-not (Test-Path $resultFile))) {
            break
        }

        Start-Sleep -Seconds 1
    }

    if ($null -ne $process -and (-not $process.HasExited)) {
        Stop-Process -Id $process.Id -Force
        $process.WaitForExit()
    }

    Write-NewTestLogLines -Path $logFile -Offset ([ref]$logStreamOffset)

    if (-not (Test-Path $resultFile) -or (Get-Item $resultFile).Length -eq 0) {
        throw "No test results produced. Check log file: $logFile"
    }

    $failuresRaw = (Get-Content -Raw -Path $resultFile).Trim()
    $failureCount = 0
    if (-not [int]::TryParse($failuresRaw, [ref]$failureCount)) {
        throw "Could not parse test result '$failuresRaw'. Check log file: $logFile"
    }

    if ($failureCount -lt 0) {
        throw "Embedded tests were unavailable (JUCE_UNIT_TESTS disabled). Check log file: $logFile"
    }

    if ($failureCount -ne 0) {
        throw "Tests failed: $failureCount failing assertions. Check log file: $logFile"
    }

    Write-Host "All tests passed."
}
finally {
    if ($hadTestMode) {
        $env:KRONOS_TEST_MODE = $previousTestMode
    }
    else {
        if (Test-Path Env:KRONOS_TEST_MODE) {
            Remove-Item Env:KRONOS_TEST_MODE
        }
    }

    if ($hadRunTests) {
        $env:KRONOS_RUN_TESTS = $previousRunTests
    }
    else {
        if (Test-Path Env:KRONOS_RUN_TESTS) {
            Remove-Item Env:KRONOS_RUN_TESTS
        }
    }

    if ($hadResultPath) {
        $env:KRONOS_TEST_RESULTS_FILE = $previousResultPath
    }
    else {
        if (Test-Path Env:KRONOS_TEST_RESULTS_FILE) {
            Remove-Item Env:KRONOS_TEST_RESULTS_FILE
        }
    }

    if ($hadLogPath) {
        $env:KRONOS_TEST_LOG_FILE = $previousLogPath
    }
    else {
        if (Test-Path Env:KRONOS_TEST_LOG_FILE) {
            Remove-Item Env:KRONOS_TEST_LOG_FILE
        }
    }
}
