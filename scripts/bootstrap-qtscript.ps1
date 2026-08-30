[CmdletBinding()]
param(
    [Parameter(Mandatory)][string] $QtRoot,
    [string] $Prefix = (Join-Path $PWD '.deps\qtscript'),
    [string] $WorkRoot = (Join-Path $PWD '.deps\qtscript-work'),
    [int] $Parallel = [Environment]::ProcessorCount
)

$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'bootstrap-qtscript-quickjs.ps1') `
    -QtRoot $QtRoot -Prefix $Prefix -WorkRoot $WorkRoot -Parallel $Parallel
if ($LASTEXITCODE) { exit $LASTEXITCODE }
