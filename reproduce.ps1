$ErrorActionPreference = 'Stop'
$fixedPython = 'C:\InFuseDeps\Python312\python.exe'
$pythonExecutable = if (Test-Path -LiteralPath $fixedPython) { $fixedPython } else { (Get-Command python -ErrorAction Stop).Source }
if (-not (Test-Path -LiteralPath (Join-Path $PSScriptRoot 'build\Release\infuse.exe'))) {
    & (Join-Path $PSScriptRoot 'scripts\build_windows.ps1') -PythonPath $pythonExecutable
}
& $pythonExecutable (Join-Path $PSScriptRoot 'scripts\reproduce.py')
if ($LASTEXITCODE -ne 0) { throw 'InFuse reproduction failed. Inspect the run log in results/reproduction.' }
