[CmdletBinding()]
param(
    [Parameter(Mandatory)][string] $QtRoot,
    [string] $Prefix = (Join-Path $PWD '.deps\qtscript-quickjs'),
    [string] $WorkRoot = (Join-Path $PWD '.deps\qtscript-quickjs-work'),
    [int] $Parallel = [Environment]::ProcessorCount
)
$ErrorActionPreference = 'Stop'
$portRepository = 'https://github.com/JulienMaille/qtscript-qt6.git'
$portRevision = '09a5abc7b5cc41c8d99b34f0a66fa44f61d3a98e'
$quickjsRepository = 'https://github.com/quickjs-ng/quickjs.git'
$quickjsRevision = '954dc53628e36891f93c359aa60895c2ae3dac6b'
$portDir = Join-Path $WorkRoot 'port'
$quickjsDir = Join-Path $portDir 'third_party\quickjs-ng'
$sourceDir = Join-Path $WorkRoot 'src'
$buildDir = Join-Path $WorkRoot 'build'
$quickjsWork = Join-Path $WorkRoot 'quickjs-build'

if (-not (Test-Path (Join-Path $portDir '.git'))) { git clone $portRepository $portDir }
git -C $portDir fetch --quiet origin $portRevision
git -C $portDir checkout --quiet --detach $portRevision
if (-not (Test-Path (Join-Path $quickjsDir '.git'))) { git clone $quickjsRepository $quickjsDir }
git -C $quickjsDir fetch --quiet origin $quickjsRevision
git -C $quickjsDir checkout --quiet --detach $quickjsRevision
& (Join-Path $portDir 'scripts\build-quickjs-ng.ps1') -Configuration Release -WorkRoot $quickjsWork -QuickJsSource $quickjsDir -Parallel $Parallel
if ($LASTEXITCODE) { throw 'QuickJS-NG build failed' }

Remove-Item $sourceDir, $buildDir -Recurse -Force -ErrorAction SilentlyContinue
& (Join-Path $portDir 'scripts\apply-patches.ps1') -SourceDir $sourceDir
Get-ChildItem (Join-Path $PSScriptRoot 'qtscript-quickjs-patches\*.patch') | Sort-Object Name | ForEach-Object {
    git -C $sourceDir apply $_.FullName
    if ($LASTEXITCODE) { throw "Failed to apply $($_.Name)" }
}
$quickjsLibrary = Join-Path $quickjsWork 'Release\build\Release\qjs.lib'
$qtCmake = Join-Path $QtRoot 'bin\qt-cmake-private.bat'
& $qtCmake -S $sourceDir -B $buildDir -G 'Ninja Multi-Config' `
    "-DCMAKE_INSTALL_PREFIX=$($Prefix.Replace('\', '/'))" `
    "-DQTSCRIPT_QUICKJS_INCLUDE_DIR=$($quickjsDir.Replace('\', '/'))" `
    "-DQTSCRIPT_QUICKJS_LIBRARY=$($quickjsLibrary.Replace('\', '/'))" `
    -DQT_BUILD_TESTS=OFF -DQT_BUILD_EXAMPLES=OFF
if ($LASTEXITCODE) { throw 'QtScript QuickJS configure failed' }
cmake --build $buildDir --config Release --parallel $Parallel
if ($LASTEXITCODE) { throw 'QtScript QuickJS build failed' }
cmake --install $buildDir --config Release
if ($LASTEXITCODE) { throw 'QtScript QuickJS install failed' }
Write-Host "QtScript QuickJS $portRevision with QuickJS-NG $quickjsRevision installed in $Prefix"
