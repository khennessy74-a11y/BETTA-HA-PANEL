# SPDX-License-Identifier: LicenseRef-FNCL-1.1
# Regenerate BETTA shared Material Design Icon fonts with LVGL font converter.
# Usage: .\tools\generate_mdi_fonts.ps1 -MdiFont .\materialdesignicons-webfont.ttf

param([Parameter(Mandatory = $true)][string]$MdiFont)
$ErrorActionPreference = "Stop"
if (-not (Test-Path $MdiFont)) { throw "MDI font not found: $MdiFont" }
$repoRoot = Split-Path -Parent $PSScriptRoot
$outDir = Join-Path $repoRoot "main\ui\fonts"

# Preserve the current shared glyph set and add the first HA-focused batch.
# Batch: lightbulb, power, thermometer, robot-vacuum, home-automation,
# script-text, window-shutter and vacuum.
$existing = @(
  0xF0001,0xF0002,0xF0019,0xF001C,0xF0028,0xF0029,0xF0041,0xF0059,
  0xF005D,0xF0064,0xF0082,0xF0102,0xF0129,0xF013D,0xF0150,0xF0171,
  0xF01D7,0xF01DB,0xF0209,0xF0210,0xF0216,0xF0223,0xF0224,0xF022E,
  0xF0299,0xF029A,0xF02DB,0xF0321,0xF0322,0xF0338,0xF035C,0xF035D,
  0xF035E,0xF03B1,0xF0411,0xF0415,0xF042E,0xF044D,0xF0493,0xF04E2,
  0xF04E5,0xF0503,0xF052E,0xF0540,0xF05AF,0xF05B0,0xF05B1,0xF05B2,
  0xF05B3,0xF05B4,0xF06E8,0xF06E9,0xF0748,0xF076D,0xF07A2,0xF0810,
  0xF08B0,0xF08B5,0xF08CE,0xF08ED,0xF08EE,0xF08EF,0xF08F0,0xF08F1,
  0xF08F2,0xF08F3,0xF08F4,0xF08F5,0xF08F6,0xF08F7,0xF0D24,0xF0E0A
)
$haBatch = @(0xF0335,0xF0425,0xF050F,0xF070D,0xF07D1,0xF0BC1,0xF111C,0xF19A1)
$range = (($existing + $haBatch) | Sort-Object -Unique | ForEach-Object { [string]$_ }) -join ","

foreach ($size in @(42,56,72)) {
  $output = Join-Path $outDir ("mditop50icons{0}.c" -f $size)
  Write-Host "Generating $output"
  & npx --yes lv_font_conv --font $MdiFont --size $size --bpp 4 --no-compress --range $range --format lvgl --output $output
  if ($LASTEXITCODE -ne 0) { throw "LVGL font conversion failed for $size px" }
}
Write-Host "MDI fonts regenerated with the Home Assistant icon batch."
