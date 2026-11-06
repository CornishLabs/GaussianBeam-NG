# This file is part of GaussianBeam-NG. Copyright (C) 2026 Daniel Ruttley.
# GNU Library General Public License version 2.
#
# Builds the Windows application icon, gui/appicon/GaussianBeam-NG.ico, from
# the SVG drawings next to it. Run it after changing either SVG:
#
#     powershell -ExecutionPolicy Bypass -File tools\make-icon.ps1
#
# The SVGs are rendered by Microsoft Edge in headless mode (no other tools are
# needed), and the images are packed into an .ico file as PNGs, which Windows
# Vista and later support. Sizes up to 48 px use the simplified drawing
# (GaussianBeam-NG-small.svg), since the wavefronts become clutter at those sizes.

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

$iconDir = Join-Path (Split-Path -Parent $PSScriptRoot) "gui\appicon"
$output = Join-Path $iconDir "GaussianBeam-NG.ico"
$sizes = 256, 128, 64, 48, 40, 32, 24, 20, 16
$smallMax = 48

$edge = @("${env:ProgramFiles(x86)}\Microsoft\Edge\Application\msedge.exe",
          "$env:ProgramFiles\Microsoft\Edge\Application\msedge.exe") | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $edge) { throw "Microsoft Edge was not found" }

# Lay out every size side by side on one transparent page
$work = Join-Path $env:TEMP ("gaussianbeam-icon-" + [guid]::NewGuid())
New-Item -ItemType Directory $work | Out-Null
try {
	$x = 0
	$images = @()
	$boxes = @()
	foreach ($size in $sizes) {
		$svg = if ($size -le $smallMax) { "GaussianBeam-NG-small.svg" } else { "GaussianBeam-NG.svg" }
		$url = "file:///" + ((Join-Path $iconDir $svg) -replace '\\', '/')
		$images += "<img src='$url' style='position:absolute;left:${x}px;top:0;width:${size}px;height:${size}px'>"
		$boxes += , @($x, $size)
		$x += $size + 8
	}
	$html = "<!DOCTYPE html><html style='background:transparent'><body style='margin:0;background:transparent'>" + ($images -join "") + "</body></html>"
	$page = Join-Path $work "icon.html"
	[IO.File]::WriteAllText($page, $html)

	$shot = Join-Path $work "icon.png"
	$arguments = "--headless", "--disable-gpu", "--force-device-scale-factor=1", "--hide-scrollbars",
	             "--default-background-color=00000000", "--allow-file-access-from-files",
	             "--user-data-dir=`"$(Join-Path $work 'profile')`"", "--window-size=$x,256",
	             "--screenshot=`"$shot`"", "`"file:///$($page -replace '\\', '/')`""
	Start-Process $edge -ArgumentList $arguments -Wait -WindowStyle Hidden
	if (-not (Test-Path $shot)) { throw "Edge did not produce a screenshot" }

	# Crop each size and keep it as PNG data
	$sheet = [System.Drawing.Bitmap]::FromFile($shot)
	$pngs = @()
	foreach ($box in $boxes) {
		$size = $box[1]
		$image = $sheet.Clone((New-Object System.Drawing.Rectangle $box[0], 0, $size, $size), [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
		$stream = New-Object IO.MemoryStream
		$image.Save($stream, [System.Drawing.Imaging.ImageFormat]::Png)
		$pngs += , $stream.ToArray()
		$image.Dispose()
	}
	$sheet.Dispose()

	# .ico file: header, one 16-byte directory entry per image, then the images
	$file = New-Object IO.MemoryStream
	$writer = New-Object IO.BinaryWriter $file
	$writer.Write([UInt16]0); $writer.Write([UInt16]1); $writer.Write([UInt16]$sizes.Count)
	$offset = 6 + 16*$sizes.Count
	for ($i = 0; $i -lt $sizes.Count; $i++) {
		$side = if ($sizes[$i] -ge 256) { 0 } else { $sizes[$i] }   # 0 means 256
		$writer.Write([byte]$side); $writer.Write([byte]$side)
		$writer.Write([byte]0); $writer.Write([byte]0)                 # no palette, reserved
		$writer.Write([UInt16]1); $writer.Write([UInt16]32)            # planes, bits per pixel
		$writer.Write([UInt32]$pngs[$i].Length); $writer.Write([UInt32]$offset)
		$offset += $pngs[$i].Length
	}
	foreach ($png in $pngs) { $writer.Write($png) }
	$writer.Flush()
	[IO.File]::WriteAllBytes($output, $file.ToArray())
	"Wrote $output ($($sizes -join ', ') px)"
}
finally {
	Remove-Item $work -Recurse -Force -ErrorAction SilentlyContinue
}
