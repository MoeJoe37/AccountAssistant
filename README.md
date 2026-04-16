# Account Assistant

Account Assistant is a bilingual (English / Arabic) Qt 6 desktop application for entering accounting data, reviewing results, generating charts, exporting reports, and saving/importing workbook data.

The current version is **4.0.0**.

---

## What the app does

The application is organized around a workflow:

1. Enter monthly accounting data.
2. Manage expense accounts in the dedicated Accounts tab.
3. Open the Results tab to generate charts and summary cards.
4. Export the finished report to PDF or save/load the data as an XLSX workbook.


---

## Main features

### Bilingual interface
- Full English / Arabic UI.
- Right-to-left layout when Arabic is selected.
- Arabic translations are used throughout the forms, dialogs, results, and exports.

### Data entry
- 12 monthly periods.
- Sales
- Sales Return
- Supplier Purchases
- Supplier Payments
- Expense Account name
- Expense Amount
- Inventory Opening
- Inventory Closing


### Accounts tab
- Add multiple accounts with:
  - account name
  - account type
  - amount
- Account type filtering:
  - All
  - Account Payable
  - Account Receivable
- Sort order:
  - Ascending
  - Descending
- Currency-style numeric display with thousands separators.
- Mouse wheel scrolling on the combo boxes is disabled so values do not change accidentally.

### Results tab
- Calculates and displays the aggregated accounting results.
- Supports multiple chart types:
  - pie charts
  - candlestick charts
  - line charts
  - grouped bar charts
  - comparison charts for two metrics / series
- The chart area is protected against accidental zooming, panning, dragging, and wheel-based interaction.
- Chart legends are handled so labels stay readable and the color swatches match the chart series.

### PDF export
- Exports the current results and selected charts to a PDF file.
- Uses a custom PDF rendering path so charts remain readable in the exported report.
- Legends are rendered in the PDF so chart colors and labels remain clear.

### Import / export
- Save the current workbook to `.xlsx`.
- Import the workbook back from `.xlsx`.
- Data is also persisted locally between launches.

### Appearance and settings
- Light and dark modes.
- Adjustable text size.
- Classic table view option in Settings.
- Theme-aware confirmation dialogs, including the Clear Data popup.

---

## Calculations

The app derives the key accounting totals using the following logic:

```text
Net Sales = Sales - Sales Return

COGS = Inventory Opening + Supplier Purchases - Inventory Closing

Profit Margin = Net Sales - COGS
```

Expense summaries are built from the Accounts tab. If the Accounts tab is empty, the legacy monthly expense fields are used as a fallback.

---

## Charts

The Results tab and PDF export support chart requests that are built from the selected metrics and comparisons.

Supported chart families include:

- **Pie**
- **Candlestick**
- **Line**
- **Grouped bar**
- **Comparison pie**
- **Comparison line**
- **Comparison candlestick / bar**

The charts are intentionally non-zoomable and non-draggable so the layout remains stable.

---

## Requirements

- CMake 3.20 or newer
- C++17 compiler
- Qt 6
- vcpkg
- Qt components:
  - Core
  - Gui
  - Widgets
  - Charts
  - PrintSupport
  - Svg
- Zlib

The project is configured for MSVC 2022 on Windows, but the CMake project is cross-platform.

---

## Build instructions

### Using vcpkg on Windows

```powershell
cmake -B build ^
  -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake" ^
  -DCMAKE_PREFIX_PATH="C:/Qt/6.11.0/msvc2022_64"
cmake --build build --config Release
```

### Running the app

```powershell
build\Release\AccountAssistant.exe
```

The build system copies `icon.ico` next to the executable when available.

---

## Project structure

```text
AccountAssistant/
├── CMakeLists.txt
├── icon.ico
├── README.md
└── src/
    ├── main.cpp
    ├── mainwindow.h / .cpp
    ├── Datatablewidget.h / .cpp
    ├── ClassicDataTableWidget.h / .cpp
    ├── Accountswidget.h / .cpp
    ├── Resultswidget.h / .cpp
    ├── chartswidget.h / .cpp
    ├── Chartselectiondialog.h / .cpp
    ├── Draggablechartcard.h / .cpp
    ├── pdfexporter.h / .cpp
    ├── settingsdialog.h / .cpp
    ├── appdata.h
    └── translations.h
```

---

## Notes

- The application remembers its settings and entered data locally.
- PDF export is only available after results have been generated.
- The default interface is modern card-based entry, with a classic spreadsheet-style view available from Settings.
- Numeric fields use grouped formatting so large values remain readable.
- The UI is designed to remain stable in both light and dark themes.

