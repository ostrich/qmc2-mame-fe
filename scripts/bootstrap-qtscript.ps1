[CmdletBinding()]
param(
    [Parameter(Mandatory)][string] $QtRoot,
    [string] $Prefix = (Join-Path $PWD '.deps\qtscript'),
    [string] $WorkRoot = (Join-Path $PWD '.deps\qtscript-work'),
    [int] $Parallel = [Environment]::ProcessorCount
)

$ErrorActionPreference = 'Stop'
$portRevision = 'b38a30b0f2324d23aa172d47c174b3f770753c8c'
$portDir = Join-Path $WorkRoot 'port'
$sourceDir = Join-Path $WorkRoot 'src'
$buildDir = Join-Path $WorkRoot 'build'

if (-not (Test-Path (Join-Path $portDir '.git'))) {
    git clone https://github.com/JulienMaille/qtscript-qt6.git $portDir
}
git -C $portDir fetch --quiet origin $portRevision
git -C $portDir checkout --quiet --detach $portRevision
& (Join-Path $portDir 'scripts\apply-patches.ps1') -SourceDir $sourceDir
$dependencyPatch = Join-Path $PSScriptRoot 'patches\qtscript-disable-werror.patch'
git -C $sourceDir apply $dependencyPatch
if ($LASTEXITCODE) { throw 'QtScript dependency patch failed' }

$qtCmake = Join-Path $QtRoot 'bin\qt-cmake-private.bat'
if (-not (Test-Path $qtCmake)) { throw "qt-cmake-private not found below $QtRoot" }

& $qtCmake -S $sourceDir -B $buildDir -G 'Ninja Multi-Config' `
    "-DCMAKE_INSTALL_PREFIX=$($Prefix.Replace('\', '/'))" `
    -DQT_BUILD_TESTS=OFF -DQT_BUILD_EXAMPLES=OFF `
    -DWARNINGS_ARE_ERRORS=OFF
if ($LASTEXITCODE) { throw 'QtScript configure failed' }
cmake --build $buildDir --config RelWithDebInfo --parallel $Parallel
if ($LASTEXITCODE) { throw 'QtScript build failed' }
cmake --install $buildDir --config RelWithDebInfo
if ($LASTEXITCODE) { throw 'QtScript install failed' }

if (-not (Test-Path (Join-Path $Prefix 'mkspecs\modules\qt_lib_script.pri'))) {
    throw 'QtScript qmake module was not installed'
}
Write-Host "QtScript $portRevision installed in $Prefix"
