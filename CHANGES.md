# Changelog

## Unreleased

### Added

- **Relative waist position column:** shows the distance from each optic to the waist of the beam leaving it. This is the waist position minus the optic's end position (its position plus its width, for thick optics). Negative values mean a virtual waist before the optic. The column is editable, like the "Waist position" column. Short name `zw-z`; it can be hidden or reordered via the table's column settings.
  - Files: `src/GaussianBeam.h` (new `BeamRelativeWaistPosition` property, added at the end of the list so saved settings keep their meaning), `gui/Names.cpp`, `gui/GaussianBeamModel.cpp`, `gui/GaussianBeamDelegate.cpp`, `gui/OpticsWidgets.cpp`.
  - Because the number of available columns changed, saved table column choices reset to the defaults once.
  - Removed "Relative waist position" from `TODO`.

### Fixed

- **Stale drawing after editing a downstream waist** (bug present in GaussianBeam 0.4): editing a waist or waist position for an optic after the input beam recalculates all earlier beams, including the input beam, but the view was only told about the edited beam and those after it. Earlier beams kept their old drawing and waist labels. `src/OpticsBench.cpp` now announces changes from the input beam onwards in this case.

## GaussianBeam-NG 0.4.1 (2026-11-05)

GaussianBeam-NG 0.4.1 is a modified version of GaussianBeam 0.4 (2008) by
Jérôme Lodewyck. Changes made 2026-11-05 by Daniel Ruttley to build and run
with Qt 5 on Windows 10/11. The changes are guarded by `QT_VERSION` checks, so
the code should still build with Qt 4, although this has not been tested.

### Renamed to GaussianBeam-NG

- `GaussianBeam.pro`:
  - the program is now called `GaussianBeam-NG`, version `0.4.1`;
  - the `.exe` is `GaussianBeam-NG.exe`;
  - name and version are passed to the code as `APP_NAME`/`APP_VERSION`;
  - Windows file properties (product, description, version) are set.
- `gui/main.cpp`: the application name and version come from `APP_NAME`/`APP_VERSION`. Settings are therefore stored under `HKEY_CURRENT_USER\Software\GaussianBeam-NG`, separately from the original GaussianBeam's.
- `gui/GaussianBeamWindow.cpp`, `gui/GaussianBeamWindow.ui`: the window title is now "GaussianBeam-NG 0.4.1" (prefixed with the file name when a file is open).
- The `.xml` file format is unchanged, so files are compatible with GaussianBeam 0.4.

### Build fixes for Qt 5

- `GaussianBeam.pro`:
  - adds the `widgets` module;
  - defines `_USE_MATH_DEFINES` (for `M_PI` with MSVC);
  - applies GCC-only flags to GCC/Clang only;
  - restricts `CODECFORTR`/`CODECFORSRC` to Qt 4;
  - links the compiler runtime statically when built with a static Qt.
- `gui/main.cpp`: `QTextCodec::setCodecForTr` (removed in Qt 5) is only used with Qt 4.
- `gui/GaussianBeamModel.cpp`: replaced `QAbstractItemModel::reset()` (removed in Qt 5) with `beginResetModel()`/`endResetModel()`.
- `gui/GaussianBeamDelegate.cpp`, `gui/GaussianBeamModel.cpp`, `gui/OpticsView.cpp`: include `<QtWidgets>` with Qt 5.
- `gui/GaussianBeamSave.cpp`: include `<QXmlStreamWriter>` from QtCore instead of QtXml.
- `gui/OpticsWidgets.cpp`: added missing `<cmath>` and `<cstdlib>` includes.
- `gui/GaussianBeamWindow.h`: fixed a mismatched include guard.

### Behaviour fixes

- **Black optics view:** since Qt 5, pens are 1 scene unit wide (1 m here) rather than 1 pixel, so outlines covered the whole view. Beam, optics and text pens are now cosmetic (always 1 pixel wide), and the background is filled with white.
- **Beam drawn incorrectly:** `QStyleOptionGraphicsItem::matrix` is no longer filled in, so the beam's resolution was computed for 1 m per pixel and the beam was drawn as straight cones. The scale is now taken from the painter's world transform.
- **Waist labels:** drawn in pixel coordinates so they are readable.
- **Dragging optics:** optics now set `ItemSendsGeometryChanges`, which Qt (4.6+) requires before it sends position-change notifications. Dragged optics now stay on the optical axis and update the table.
- **Window position:** the main window always opens centred on the primary monitor, instead of at a saved position that may be on a disconnected monitor.
- **Translations:** `.qm` files are also looked for next to the executable.

### Documentation

- `INSTALL`: added Windows build instructions using MSYS2.
- Added `README.md`, `CHANGES.md` and `.gitignore`.
