param(
  [ValidateSet('debug', 'release', 'all')]
  [string]$Preset = 'debug'
)

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$BuildRoot = Join-Path $ProjectRoot 'build'
$Targets = if ($Preset -eq 'all') { @('debug', 'release') } else { @($Preset) }

foreach ($Name in $Targets) {
  $Path = Join-Path $BuildRoot $Name
  if (Test-Path -LiteralPath $Path) {
    $ResolvedPath = (Resolve-Path -LiteralPath $Path).Path
    if (-not $ResolvedPath.StartsWith($BuildRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
      throw "Refusing to delete path outside build root: $ResolvedPath"
    }
    Remove-Item -LiteralPath $ResolvedPath -Recurse -Force
  }
}
