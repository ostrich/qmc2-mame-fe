[CmdletBinding()]
param(
    [Parameter(Mandatory)][string] $Prefix
)

$ErrorActionPreference = 'Stop'
$version = '1.1.7'
$sha256 = '4c8af345586a9a08fbfd2f613fcac748226d91a75627aa3581b297dd513046fe'
$archive = Join-Path ([System.IO.Path]::GetTempPath()) "jom-$version.zip"
$archiveVersion = $version -replace '\.', '_'
$url = "https://download.qt.io/official_releases/jom/jom_$archiveVersion.zip"

Invoke-WebRequest $url -OutFile $archive
$actualHash = (Get-FileHash $archive -Algorithm SHA256).Hash.ToLowerInvariant()
if ($actualHash -ne $sha256) {
    throw "Unexpected SHA256 for jom $version`: $actualHash"
}

New-Item -ItemType Directory -Path $Prefix -Force | Out-Null
Expand-Archive $archive -DestinationPath $Prefix -Force
Remove-Item $archive -Force
Write-Host "jom $version installed in $Prefix"
