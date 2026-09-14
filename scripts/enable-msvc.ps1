[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
if (-not $env:GITHUB_ENV) { throw 'This helper requires a GitHub Actions environment file' }

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $installation) { throw 'Visual Studio C++ tools were not found' }
$vcvars = Join-Path $installation 'VC\Auxiliary\Build\vcvars64.bat'
$variables = cmd /c "call `"$vcvars`" >nul 2>&1 && set"
if ($LASTEXITCODE) { throw 'Failed to initialize the MSVC developer environment' }
$exportNames = @(
  'Path', 'INCLUDE', 'LIB', 'LIBPATH', 'VCINSTALLDIR',
  'VCToolsInstallDir', 'VisualStudioVersion', 'VSINSTALLDIR',
  'UniversalCRTSdkDir', 'UCRTVersion', 'WindowsSdkDir', 'WindowsSDKVersion'
)
foreach ($line in $variables) {
  if ($line -match '^([^=]+)=(.*)$' -and $exportNames -contains $Matches[1]) {
    "$($Matches[1])=$($Matches[2])" | Out-File -FilePath $env:GITHUB_ENV -Encoding utf8 -Append
  }
}
