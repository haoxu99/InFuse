param([string]$DependencyRoot = 'C:\InFuseDeps')
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
$DependencyRoot = [IO.Path]::GetFullPath($DependencyRoot)
$opencvConfig = Join-Path $DependencyRoot 'opencv\build\x64\vc15\lib'
if (Test-Path -LiteralPath (Join-Path $opencvConfig 'OpenCVConfig.cmake')) {
    Write-Output $opencvConfig
    exit 0
}
New-Item -ItemType Directory -Path $DependencyRoot -Force | Out-Null
$downloadRoot = Join-Path $DependencyRoot 'downloads'
New-Item -ItemType Directory -Path $downloadRoot -Force | Out-Null
$archive = Join-Path $downloadRoot 'opencv-4.4.0-vc14_vc15.exe'
$url = 'https://github.com/opencv/opencv/releases/download/4.4.0/opencv-4.4.0-vc14_vc15.exe'
$expectedHash = 'cd480a6fe345f4285bf01626998ef8d65b92fbfb2db9f2905034e4c4786e0d50'
if (-not (Test-Path -LiteralPath $archive)) {
    Write-Host 'Downloading the official OpenCV 4.4.0 Windows package...'
    Invoke-WebRequest -UseBasicParsing -Uri $url -OutFile $archive
}
if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expectedHash) {
    throw "OpenCV checksum mismatch. Remove the invalid downloaded file and retry: $archive"
}
$extractArguments = '-o"' + $DependencyRoot + '" -y'
$extract = Start-Process -FilePath $archive -ArgumentList $extractArguments -WindowStyle Hidden -Wait -PassThru
if ($extract.ExitCode -ne 0) { throw "OpenCV extraction failed: $($extract.ExitCode)" }
if (-not (Test-Path -LiteralPath (Join-Path $opencvConfig 'OpenCVConfig.cmake'))) {
    throw "The OpenCV configuration was not extracted to $opencvConfig"
}
Write-Output $opencvConfig
