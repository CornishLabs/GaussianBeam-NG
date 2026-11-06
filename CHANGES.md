# Changelog

## Unreleased

### Added

- **Relative waist position column:** shows the distance from each optic to the waist of the beam leaving it. This is the waist position minus the optic's end position (its position plus its width, for thick optics). Negative values mean a virtual waist before the optic. The column is editable, like the "Waist position" column. Short name `zw-z`; it can be hidden or reordered via the table's column settings.
  - Files: `src/GaussianBeam.h` (new `BeamRelativeWaistPosition` property, added at the end of the list so saved settings keep their meaning), `gui/Names.cpp`, `gui/GaussianBeamModel.cpp`, `gui/GaussianBeamDelegate.cpp`, `gui/OpticsWidgets.cpp`.
  - Because the number of available columns changed, saved table column choices reset to the defaults once.
  - Removed "Relative waist position" from `TODO`.
- **Wavefronts on the graph:** the beams are drawn with their wavefronts (surfaces of constant phase), spaced at round-number intervals of about 40 pixels. To scale, the wavefront curvature would be invisible (its sag across the beam, w²/2R, is typically below a micrometre), so each wavefront is drawn perpendicular, on screen, to the beam's rays. This is the real wavefront with the view's horizontal/vertical stretch applied to it: flat at the waist, most curved near the Rayleigh range, and an arc centred on the waist in the far field. Wavefronts can be turned off with "Show wavefronts" in the view properties panel (the zoom icon in the corner of the view); the choice is saved.
  - Files: `gui/OpticsView.h`, `gui/OpticsView.cpp` (`BeamItem::wavefront`, `BeamItem::drawWavefronts`, `OpticsScene::showWavefronts`), `gui/OpticsWidgets.h`, `gui/OpticsWidgets.cpp`, `gui/OpticsViewProperties.ui`.
  - Removed "Wavefront on graph" from `TODO`.
- **Dark mode**: a moon/sun button at the right-hand end of the toolbar switches between light and dark mode. The choice is saved (registry value `Theme\dark`). On first run, the app follows the Windows "app mode" setting.
  - Everything switches: widgets, table, optics view background, rulers, optics outlines, waist labels, and the window title bars. On Windows 10 20H1 and later, title bars use `DwmSetWindowAttribute`.
  - **The laser beams look the same in both modes.** Beams are drawn slightly transparent, so on a dark background they would look darker. In dark mode they are drawn opaque, in the colour they have on the light background. Wavefront lines are unchanged.
  - Files: `gui/Theme.h`, `gui/Theme.cpp` (new: palettes, view colours, icons, title bars), `gui/main.cpp`, `gui/GaussianBeamWindow.h`, `gui/GaussianBeamWindow.cpp` (toggle button; the toolbar can no longer be moved, so the button stays top-right), `gui/OpticsView.h`, `gui/OpticsView.cpp`, `gui/OpticsWidgets.h`, `gui/OpticsWidgets.cpp`.

### Changed

- **New icons**: the KDE Oxygen PNG icons are replaced by a consistent set of outline SVG icons drawn for GaussianBeam-NG (`gui/icons/`). They are recoloured to match light or dark mode.
  - Icons are now set in code (`Theme::icon`), not in the `.ui` files.
  - The table and status-bar configuration icons are now "sliders". The view-properties icon is now a frame.
  - The corner icons show a tooltip and a hand cursor.
  - **New build dependency:** the Qt SVG module (`QT += svg`). For the shared MSYS2 build, install `mingw-w64-ucrt-x86_64-qt5-svg`. The static Qt package already includes it. On Windows the app also links `dwmapi`.
  - Files: `gui/icons/*.svg` (new), `gui/images/` (removed), `gui/GaussianBeamRessource.qrc`, `gui/GaussianBeamWindow.ui`, `gui/GaussianBeamWidget.ui`, `gui/GaussianBeamWidget.h`, `gui/GaussianBeamWidget.cpp`, `GaussianBeam.pro`, `INSTALL`, `README.md`.
- **Fusion style**: the app uses Qt's Fusion style in both modes, because the native Windows style cannot be made dark. This gives the light mode a flatter look too. The rulers and view corner are now neutral grey instead of cream. The rulers are at least as tall as their labels, whatever the style. (`gui/Theme.cpp`, `gui/OpticsWidgets.cpp`)
- **Default input beam position:** a new bench now starts with the input beam waist at 0 mm instead of 10 mm, so the default lenses are at 50 and 100 mm. Saved files are unaffected. (`src/OpticsBench.cpp`)

### Fixed

- **Stale drawing after editing a downstream waist** (bug present in GaussianBeam 0.4): editing a waist or waist position for an optic after the input beam recalculates all earlier beams, including the input beam, but the view was only told about the edited beam and those after it. Earlier beams kept their old drawing and waist labels. `src/OpticsBench.cpp` now announces changes from the input beam onwards in this case.

## GaussianBeam-NG 0.4.1

GaussianBeam-NG 0.4.1 is a modified version of GaussianBeam 0.4 (2008) by
Jérôme Lodewyck. Changes made by Daniel Ruttley to build and run with
Qt 5 on Windows 10/11. The changes are guarded by `QT_VERSION` checks, so
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
