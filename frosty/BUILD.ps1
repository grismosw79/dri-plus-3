param(
    [Parameter(Mandatory=$true)][string]$MmcFolder,
    [Parameter(Mandatory=$true)][string]$CandidateRuntime,
    [string]$OutputDir = (Join-Path $PSScriptRoot 'build')
)
$ErrorActionPreference = 'Stop'
$compiler = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
$refs = @('FrostyCore.dll','FrostySdk.dll') | ForEach-Object {
    (Resolve-Path -LiteralPath (Join-Path $MmcFolder $_)).Path
}
$payloads = [ordered]@{
    'DRIPlus3-RM18R22.dll' = 'd3899e8cd372ca73ce53adcc6e6a8919c06d3082e91174f51284c8775ffebfce'
    'DRIPlus3-RM18R22.exe' = 'a4cff9e12840b44d0833aca7645e8ab0fd0a9d604a4f2087ff5ab1fc45664144'
}
$arguments = @('/nologo','/optimize+','/target:library','/platform:x64','/filealign:512')
$arguments += $refs | ForEach-Object { '/reference:' + $_ }
foreach ($item in $payloads.GetEnumerator()) {
    $path = (Resolve-Path -LiteralPath (Join-Path $CandidateRuntime $item.Key)).Path
    if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $item.Value) {
        throw "The runtime input does not match the RM18R22 candidate: $($item.Key)"
    }
    $arguments += "/resource:$path,DRIPlus.$($item.Key)"
}
$output = [IO.Directory]::CreateDirectory($OutputDir).FullName
$arguments += '/out:' + (Join-Path $output 'DRIPlus3.dll')
$arguments += (Join-Path $PSScriptRoot 'DRIPlus3.cs')
$arguments += (Join-Path $PSScriptRoot 'LaunchWatcher.cs')
& $compiler @arguments
if ($LASTEXITCODE -ne 0) { throw "C# compiler exit $LASTEXITCODE" }
Get-Item -LiteralPath (Join-Path $output 'DRIPlus3.dll')
