param(
  [ValidateSet('debug', 'release')]
  [string]$Preset = 'debug'
)

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
Push-Location $ProjectRoot
try {
  cmake --preset $Preset
}
finally {
  Pop-Location
}
