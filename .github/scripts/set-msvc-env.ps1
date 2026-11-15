# Loads the MSVC developer environment and republishes every variable that
# changed, so later steps see what vcvars set without depending on the
# node20-based setup action. prepare.py needs Platform; breakpad's gyp needs
# VSINSTALLDIR; the compiler needs PATH, INCLUDE, LIB and LIBPATH.
param(
    [string] $Toolset = ''
)

$ErrorActionPreference = 'Stop'

$vcvars = Join-Path $env:ProgramFiles 'Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path $vcvars)) {
    throw "vcvars64.bat not found at $vcvars"
}

$before = @{}
foreach ($entry in (Get-ChildItem Env:)) {
    $before[$entry.Name] = $entry.Value
}

# vcvars writes a banner, so capture the environment by diffing `set` output
# around it rather than parsing the banner.
$cmd = @(
    '@echo off'
    "call `"$vcvars`" $(if ($Toolset) { "-vcvars_ver=$Toolset" })"
    'if errorlevel 1 exit /b 1'
    'set'
) -join "`r`n"

$temp = Join-Path $env:RUNNER_TEMP 'vcvars-env.cmd'
Set-Content -Path $temp -Value $cmd -Encoding ascii
$output = & cmd.exe /c $temp
if ($LASTEXITCODE -ne 0) {
    throw "vcvars failed with exit code $LASTEXITCODE"
}

$changed = 0
foreach ($line in $output) {
    $eq = $line.IndexOf('=')
    if ($eq -lt 1) { continue }
    $name = $line.Substring(0, $eq)
    $value = $line.Substring($eq + 1)
    if ($before.ContainsKey($name) -and $before[$name] -eq $value) { continue }
    # GITHUB_ENV cannot carry a multi-line value, and nothing here needs one.
    if ($value -match "[`r`n]") { continue }
    "$name=$value" | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8
    $changed++
}

if ($changed -eq 0) {
    throw 'vcvars changed no environment variables'
}

# prepare.py reads Platform to pick the target arch; vcvars does not set it.
'Platform=x64' | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8

Write-Host "Exported $changed environment variables from vcvars"
foreach ($required in @('PATH', 'INCLUDE', 'LIB', 'LIBPATH', 'VCToolsVersion', 'VSINSTALLDIR')) {
    if (-not [Environment]::GetEnvironmentVariable($required)) {
        Write-Host "::warning::$required was not set by vcvars"
    }
}
