param(
  [Parameter(Mandatory=$true)][string]$Converter,
  [Parameter(Mandatory=$true)][string]$Root
)

# 対象拡張子（必要なら増やす）
$exts = @("*.png","*.jpg","*.jpeg","*.bmp","*.tga")

foreach ($e in $exts) {
  Get-ChildItem -Path $Root -Recurse -Filter $e | ForEach-Object {
    $src = $_.FullName
    $dds = [System.IO.Path]::ChangeExtension($src, ".dds")

    # ddsが無い or 元画像の方が新しいときだけ変換（増分ビルド）
    if (!(Test-Path $dds) -or ((Get-Item $src).LastWriteTime -gt (Get-Item $dds).LastWriteTime)) {
      Write-Host "[TextureConverter] $src"
      & $Converter $src
      if ($LASTEXITCODE -ne 0) { throw "TextureConverter failed: $src" }
    }
  }
}