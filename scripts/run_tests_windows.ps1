[CmdletBinding()]
param(
    [string]$Configuration = "Debug",
    [string]$SolutionDir,
    [string]$Platform = "x64",
    [switch]$SkipBuild,
    [string]$AppBinary,
    [int]$TimeoutSeconds = 120
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($SolutionDir)) {
    $SolutionDir = (Resolve-Path (Join-Path $PSScriptRoot ".." )).Path
}

$SolutionDir = [System.IO.Path]::GetFullPath($SolutionDir)
$standaloneProject = Join-Path $SolutionDir "Builds\VisualStudio2022\Kronos_StandalonePlugin.vcxproj"

if (-not (Test-Path $standaloneProject)) {
    throw "Standalone project not found: $standaloneProject"
}

if (-not $SkipBuild) {
    $msbuild = Get-Command msbuild -ErrorAction SilentlyContinue
    if (-not $msbuild) {
        throw "msbuild not found on PATH. Run from Developer PowerShell for VS2022 or install Build Tools."
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

        & $msbuild.Source $standaloneProject "/m" "/p:Configuration=$Configuration;Platform=$Platform"
        if ($LASTEXITCODE -ne 0) {
            throw "Standalone test build failed with exit code $LASTEXITCODE"
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
    $process = Start-Process -FilePath $AppBinary -WorkingDirectory (Split-Path -Parent $AppBinary) -PassThru

    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
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
