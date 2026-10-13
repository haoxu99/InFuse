param(
    [string]$DependencyRoot = 'C:\InFuseDeps',
    [string]$OpenCVDir = '',
    [string]$CMakePath = '',
    [string]$PythonPath = '',
    [switch]$SkipTests
)
$ErrorActionPreference = 'Stop'
$sourceRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$buildRoot = Join-Path $sourceRoot 'build'
if (-not $CMakePath) {
    $portableCMake = Join-Path $DependencyRoot 'cmake-3.31.6-windows-x86_64\bin\cmake.exe'
    $CMakePath = if (Test-Path -LiteralPath $portableCMake) { $portableCMake } else { (Get-Command cmake -ErrorAction Stop).Source }
}
if (-not $PythonPath) {
    $fixedPython = Join-Path $DependencyRoot 'Python312\python.exe'
    $PythonPath = if (Test-Path -LiteralPath $fixedPython) { $fixedPython } else { (Get-Command python -ErrorAction Stop).Source }
}
if (-not $OpenCVDir) { $OpenCVDir = & (Join-Path $PSScriptRoot 'download_opencv.ps1') -DependencyRoot $DependencyRoot }
if (-not (Test-Path -LiteralPath (Join-Path $OpenCVDir 'OpenCVConfig.cmake'))) { throw "Missing OpenCV configuration: $OpenCVDir" }
& $CMakePath -S $sourceRoot -B $buildRoot -G 'Visual Studio 17 2022' -A x64 "-DOpenCV_DIR=$OpenCVDir" "-DPython3_EXECUTABLE=$PythonPath" -DBUILD_TESTING=ON
if ($LASTEXITCODE -ne 0) { throw 'InFuse CMake configuration failed.' }
& $CMakePath --build $buildRoot --config Release --parallel 4 -- /nologo /verbosity:quiet
if ($LASTEXITCODE -ne 0) { throw 'InFuse compilation failed.' }
if (-not $SkipTests) {
    $ctestPath = Join-Path (Split-Path $CMakePath -Parent) 'ctest.exe'
    & $ctestPath --test-dir $buildRoot -C Release --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw 'InFuse tests failed.' }
}
Write-Host ('Executable: ' + (Join-Path $buildRoot 'Release\infuse.exe'))
