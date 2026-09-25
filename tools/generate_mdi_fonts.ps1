# SPDX-License-Identifier: LicenseRef-FNCL-1.1
# Regenerate BETTA shared Material Design Icon fonts with LVGL font converter.
# Usage: .\tools\generate_mdi_fonts.ps1 -MdiFont .\materialdesignicons-webfont.ttf

param([Parameter(Mandatory = $true)][string]$MdiFont)
$ErrorActionPreference = "Stop"
if (-not (Test-Path $MdiFont)) { throw "MDI font not found: $MdiFont" }
$repoRoot = Split-Path -Parent $PSScriptRoot
$outDir = Join-Path $repoRoot "main\ui\fonts"

# Keep one explicit shared glyph manifest for all 42/56/72 px fonts.
# This prevents the generator's "existing" and "new batch" lists drifting apart.
$glyphs = @(
  0xF0001,0xF0002,0xF0019,0xF001C,0xF0028,0xF0029,0xF0041,0xF0059,
  0xF005D,0xF0064,0xF0082,0xF0102,0xF0129,0xF013D,0xF0150,0xF0171,
  0xF01D7,0xF01DB,0xF0209,0xF0210,0xF0216,0xF0223,0xF0224,0xF022E,
  0xF0299,0xF029A,0xF02DB,0xF0321,0xF0322,0xF0338,0xF035C,0xF035D,
  0xF035E,0xF03B1,0xF0411,0xF0415,0xF042E,0xF044D,0xF0493,0xF04E2,
  0xF04E5,0xF0503,0xF052E,0xF0540,0xF05AF,0xF05B0,0xF05B1,0xF05B2,
  0xF05B3,0xF05B4,0xF06E8,0xF06E9,0xF0748,0xF076D,0xF07A2,0xF0810,
  0xF08B0,0xF08B5,0xF08CE,0xF08ED,0xF08EE,0xF08EF,0xF08F0,0xF08F1,
  0xF08F2,0xF08F3,0xF08F4,0xF08F5,0xF08F6,0xF08F7,0xF0D24,0xF0E0A,
  0xF0335,0xF0425,0xF050F,0xF070D,0xF07D1,0xF0BC1,0xF111C,0xF19A1,
  0xF033E,0xF033F,0xF06D9,0xF06DA,0xF081A,0xF081C,0xF00AC,0xF1011,
  # Heating/button icons used by the editor must be present in the generated
  # assets as well as the registry. U+F0438 is mdi:radiator.
  0xF0438,0xF0D91,0xF0079,0xF0084,0xF0A72,0xF0D3E,
  # Core Home Assistant weather conditions. The forecast header uses the
  # 42/56/72 px shared MDI fonts; without these glyphs it falls back to the
  # dedicated 20 px weather font and renders a tiny current-condition icon.
  0xF0590,0xF0591,0xF0592,0xF0593,0xF0594,0xF0595,0xF0596,0xF0597,
  0xF0598,0xF0599,0xF059A,0xF059B,0xF059C,0xF059D,0xF059E,0xF067E,
  0xF067F,0xF0898,0xF0E6E,0xF0F2F,0xF0F30,0xF0F31,0xF0F32,0xF0F33,
  0xF0F34,0xF0F35,0xF0F36,0xF0F37
)
$range = ($glyphs | Sort-Object -Unique | ForEach-Object { [string]$_ }) -join ","

foreach ($size in @(42,56,72)) {
  $output = Join-Path $outDir ("mditop50icons{0}.c" -f $size)
  Write-Host "Generating $output"
  & npx --yes lv_font_conv --font $MdiFont --size $size --bpp 4 --no-compress --range $range --format lvgl --output $output
  if ($LASTEXITCODE -ne 0) { throw "LVGL font conversion failed for $size px" }

  $generated = Get-Content -Raw $output
  $generated = $generated -replace '#ifdef LV_LVGL_H_INCLUDE_SIMPLE\r?\n#include "lvgl.h"\r?\n#else\r?\n#include "lvgl/lvgl.h"\r?\n#endif', '#include "lvgl.h"'
  Set-Content -Path $output -Value $generated -NoNewline
}
Write-Host "MDI fonts regenerated from the shared glyph manifest."
