# GaussianBeam-NG

GaussianBeam-NG is a program for designing optical systems for Gaussian laser beams. You place optics (lenses, mirrors, interfaces, dielectric slabs and generic ABCD matrices) along an optical axis. It then calculates and draws the beam's waist, waist position, Rayleigh range and divergence after each optic. You can drag optics in the view to see how the beam changes.

It has a light and a dark mode. Switch between them with the moon/sun button in the top-right corner of the window. The first time it runs, it follows the Windows light/dark app setting.

GaussianBeam-NG is a continuation of **GaussianBeam 0.4** (2008), originally written by **Jérôme Lodewyck** for Qt 4. The original no longer builds with current tools. GaussianBeam-NG builds with Qt 5 and runs on Windows 10 and 11. See [CHANGES.md](CHANGES.md) for exactly what was changed and why.

The original project is at <http://gaussianbeam.sourceforge.net>. GaussianBeam-NG is not affiliated with it.

## Download

A ready-to-run Windows executable is available on the [Releases](../../releases) page. It is a single `.exe` with Qt built in, so there is nothing to install. Download it and double-click to run.

GaussianBeam-NG opens and saves the same `.xml` files as the original GaussianBeam. It saves its window size, last-used wavelength and light/dark mode in the Windows registry, under `HKEY_CURRENT_USER\Software\GaussianBeam-NG`.

## Building from source

See [INSTALL](INSTALL). In short, on Windows, using [MSYS2](https://www.msys2.org), open the **UCRT64** terminal and run:

```
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make mingw-w64-ucrt-x86_64-qt5-base mingw-w64-ucrt-x86_64-qt5-svg
qmake-qt5
mingw32-make
```

This produces `release/GaussianBeam-NG.exe`. To build a single self-contained `.exe`, install `mingw-w64-ucrt-x86_64-qt5-static` and run `/ucrt64/qt5-static/bin/qmake` instead of `qmake-qt5`.

On Linux, install the Qt 5 development packages, then run `qmake` and `make`.

The program name and version are set at the top of `GaussianBeam.pro`.

The optional plotting feature (`CONFIG += plot` in `GaussianBeam.pro`) needs the Qwt library and has not been ported.

## Versioning

GaussianBeam-NG uses [semantic versioning](https://semver.org). Version 0.4.1 is GaussianBeam 0.4 plus the fixes needed to build and run with Qt 5. It has the same features and file format as GaussianBeam 0.4. Versions 0.5.0 and 0.6.0 add new features (see [CHANGES.md](CHANGES.md)) and keep the same file format.

## Licence

GaussianBeam-NG is free software, distributed under the GNU General Public License version 2. See [COPYING](COPYING). The individual source files carry the GNU Library General Public License version 2 notice. Both permit modification and redistribution under these terms.

- Original GaussianBeam code © 2007–2008 Jérôme Lodewyck.
- GaussianBeam-NG changes © 2026 Daniel Ruttley, under the same licence.
- The icons were drawn for GaussianBeam-NG and are under the same licence. (Earlier versions used icons from KDE's Oxygen icon theme.)
- The Windows release executable is statically linked against Qt 5, which is used under the terms of the GNU GPL.
