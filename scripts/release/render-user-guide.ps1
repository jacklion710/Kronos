[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$SourceMarkdown,
    [Parameter(Mandatory = $true)]
    [string]$OutputDir,
    [string]$ProductName = "Kronos"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not (Test-Path $SourceMarkdown)) {
    throw "User guide source not found: $SourceMarkdown"
}

$sourceDirectory = Split-Path -Parent ((Resolve-Path $SourceMarkdown).Path)
$OutputDir = [System.IO.Path]::GetFullPath($OutputDir)
New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null

$markdownTarget = Join-Path $OutputDir ("{0} User Guide.md" -f $ProductName)
$htmlTarget = Join-Path $OutputDir ("{0} User Guide.html" -f $ProductName)
$textTarget = Join-Path $OutputDir ("{0} User Guide.txt" -f $ProductName)

Copy-Item -Path $SourceMarkdown -Destination $markdownTarget -Force

$pandoc = Get-Command pandoc -ErrorAction SilentlyContinue
if ($null -ne $pandoc) {
    & $pandoc.Source $SourceMarkdown "-s" "-o" $htmlTarget
    if ($LASTEXITCODE -ne 0) {
        throw "pandoc failed with exit code $LASTEXITCODE"
    }
}
else {
    Copy-Item -Path $SourceMarkdown -Destination $textTarget -Force

    $sourceText = Get-Content -Raw -Path $SourceMarkdown
    $escaped = [System.Net.WebUtility]::HtmlEncode($sourceText)
    $html = @"
<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <title>$ProductName User Guide</title>
</head>
<body>
  <pre>$escaped</pre>
</body>
</html>
"@

    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($htmlTarget, $html, $utf8NoBom)
}

$manualScreenshotsSource = Join-Path $sourceDirectory "manual_screenshots"
if (Test-Path $manualScreenshotsSource) {
    $manualScreenshotsTarget = Join-Path $OutputDir "manual_screenshots"
    if (Test-Path $manualScreenshotsTarget) {
        Remove-Item -Recurse -Force -Path $manualScreenshotsTarget
    }

    New-Item -ItemType Directory -Path $manualScreenshotsTarget -Force | Out-Null
    $allowedExtensions = @(".png", ".jpg", ".jpeg", ".gif", ".svg", ".webp")
    Get-ChildItem -Path $manualScreenshotsSource -File -Recurse | Where-Object {
        $allowedExtensions -contains $_.Extension.ToLowerInvariant()
    } | ForEach-Object {
        Copy-Item -Path $_.FullName -Destination (Join-Path $manualScreenshotsTarget $_.Name) -Force
    }
}

Write-Host "Rendered user guide assets in: $OutputDir"
