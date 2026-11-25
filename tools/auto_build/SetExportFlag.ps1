param(
    [Parameter(Mandatory=$true)]
    [ValidateSet("0", "1")]
    [string]$Value
)

$projectPath = Join-Path (Get-Location) "projects\standalone\HISE Standalone.jucer"
$projectText = [System.IO.File]::ReadAllText($projectPath)
$pattern = 'defines="HISE_DISABLE_EXPORT=[01]"'
$matches = [regex]::Matches($projectText, $pattern)

if ($matches.Count -ne 1) {
    Write-Error "Expected exactly one HISE_DISABLE_EXPORT definition in $projectPath. Found $($matches.Count)."
    exit 1
}

$updatedText = [regex]::Replace($projectText, $pattern, "defines=`"HISE_DISABLE_EXPORT=$Value`"")
$encoding = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($projectPath, $updatedText, $encoding)
