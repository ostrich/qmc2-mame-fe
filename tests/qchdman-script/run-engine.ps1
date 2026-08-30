[CmdletBinding()]
param(
    [Parameter(Mandatory)][string] $Prefix,
    [string] $QMake = 'qmake',
    [string] $Name = 'qt6',
    [ValidateRange(1, 100)][int] $Repeat = 1,
    [string] $BuildRoot = (Join-Path $env:TEMP "qchdman-script-$([Guid]::NewGuid())")
)
$ErrorActionPreference = 'Stop'
$testRoot = $PSScriptRoot
$env:QMAKEPATH = "$(Join-Path $Prefix 'lib\qt6');$env:QMAKEPATH"
$env:QTSCRIPT_PREFIX = $Prefix
$env:PATH = "$(Join-Path $Prefix 'bin');$env:PATH"
New-Item -ItemType Directory -Path $BuildRoot -Force | Out-Null
Push-Location $BuildRoot
try {
    & $QMake (Join-Path $testRoot 'qchdman-script.pro') CONFIG+=release
    if ($LASTEXITCODE) { throw 'qchdman scripting harness configure failed' }
    jom
    if ($LASTEXITCODE) { throw 'qchdman scripting harness build failed' }
    $env:QT_QPA_PLATFORM = 'offscreen'
    $env:QCHDMAN_FAKE_CHDMAN = Join-Path $BuildRoot 'fake-chdman\release\fake-chdman.exe'
    $harness = Join-Path $BuildRoot 'harness\release\tst_qchdman_script.exe'
    for ($attempt = 1; $attempt -le $Repeat; ++$attempt) {
        $env:QCHDMAN_TEST_RESULTS = Join-Path $BuildRoot "$Name-run-$attempt.json"
        $runLog = Join-Path $BuildRoot "$Name-run-$attempt.txt"
        $run = Start-Process $harness -ArgumentList @('-o', "$runLog,txt") -NoNewWindow -Wait -PassThru
        Write-Host "=== $Name qchdman contract run $attempt/$Repeat (exit $($run.ExitCode)) ==="
        if (Test-Path $runLog) { Get-Content $runLog }
        if ($run.ExitCode) {
            Write-Host "qchdman scripting harness exited with native code $($run.ExitCode); isolating test functions"
            foreach ($testName in @(
                'coreUtilities',
                'languageRuntimeBridge',
                'repeatedRuns',
                'inputDialogs',
                'versionOnePersistence',
                'projectProperties',
                'projectLifecycleAndFailures',
                'scriptInterruptionAndStress',
                'debuggerWorkflow',
                'structuredResultRoundTrip',
                'slotManifestComplete'
            )) {
                $isolatedLog = Join-Path $BuildRoot "$Name-run-$attempt-$testName.txt"
                $isolated = Start-Process $harness -ArgumentList @($testName, '-o', "$isolatedLog,txt") -NoNewWindow -Wait -PassThru
                Write-Host "--- $testName (exit $($isolated.ExitCode)) ---"
                if (Test-Path $isolatedLog) { Get-Content $isolatedLog }
            }
            throw "qchdman scripting harness failed with native exit code $($run.ExitCode) on run $attempt/$Repeat"
        }
        $compareArgs = @((Join-Path $testRoot 'tools\compare_results.py'),
            (Join-Path $testRoot 'reference\qt5-5.15.19.json'), $env:QCHDMAN_TEST_RESULTS)
        if ($Name -eq 'quickjs') {
            $compareArgs += @('--engine', 'quickjs', '--allowlist', (Join-Path $testRoot 'allowed-differences.json'))
        }
        python @compareArgs
        if ($LASTEXITCODE) { throw "$Name differs from the Qt 5 contract on run $attempt/$Repeat" }
    }
} finally {
    Pop-Location
}
