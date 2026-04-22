# Account Assistant 5.3

Account Assistant 5.3 is a stability, structure, and usability release focused on improving maintainability, visual consistency, and the inventory workflow without removing any existing features or interface elements.

## Highlights

### Inventory workflow improvements
- Unified the results experience so inventory result views behave more consistently.
- Improved the inventory mode switch flow with a dedicated confirmation dialog.
- Added safer handling when switching between inventory modes to prevent accidental data loss.
- Improved support for inventory-related calculations in the results flow.

### Results and charting improvements
- Added support for summary-style metric output across selected periods.
- Improved chart editing behavior from the Results tab so existing graph settings are restored correctly when editing.
- Fixed issues where recalculating could clear result values or produce empty graphs.
- Improved percentage display behavior across chart types.
- Improved metric color consistency across cards, legends, and chart series.

### Table and results stability
- Fixed regressions affecting table-based views.
- Improved compatibility between results, charts, and inventory modes.
- Reduced issues caused by repeated calculate/edit cycles.

### UI and dialog improvements
- Improved the inventory switch warning dialog behavior and layout.
- Fixed theme-related inconsistencies in that popup.
- Made the inventory switch popup fixed-size for more predictable behavior.

### Internal cleanup and maintainability
- Refactored shared UI styling logic into centralized theme helpers.
- Reduced duplicated UI refresh and results rebuild paths.
- Cleaned up project packaging by removing stale build output from the release archive.

## Performance and optimization

This release includes safe structural cleanup intended to improve responsiveness and reduce redundant work in the UI update flow:
- less duplicated results rebuild logic
- fewer repeated styling paths
- cleaner shared helpers for dialogs, charts, and widgets

These changes were made without removing features or changing the overall interface structure.

## Upgrade notes
- For best results, perform a clean rebuild when upgrading to 5.3.
- If you are updating from an older local build, remove the old `build` directory before configuring again.

## Clean build example

```powershell
rmdir /s /q build
cmake -B build -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake" -DCMAKE_PREFIX_PATH="C:/Qt/6.11.0/msvc2022_64"
cmake --build build --config Release
```

## Included in this release
- Source code for version 5.3
- Updated inventory switch dialog behavior
- Results and chart handling fixes
- Visual consistency improvements for metrics and charts
- Internal code cleanup and optimization

## Version
**Release:** 5.3.0

