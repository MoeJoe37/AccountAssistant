# Account Assistant

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![Qt 6](https://img.shields.io/badge/Qt-6.x-green.svg)
![CMake](https://img.shields.io/badge/Build-CMake-informational.svg)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)

**Account Assistant** is a desktop financial reporting application built with **C++17** and **Qt 6**. It helps users enter monthly accounting data, manage expenses and suppliers, calculate financial results, visualize metrics through charts, and export professional reports to **PDF** and **XLSX**.

> Runtime application version in `main.cpp`: **6.0.0**  
> CMake project version in `CMakeLists.txt`: **5.3.0**  
> Recommendation: update `project(AccountAssistant VERSION ...)` in `CMakeLists.txt` to keep build metadata consistent with the runtime version.

---

## Table of Contents

- [Overview](#overview)
- [Core Features](#core-features)
- [Application Workflow](#application-workflow)
- [Financial Calculations](#financial-calculations)
- [Charts and Reporting](#charts-and-reporting)
- [Import and Export](#import-and-export)
- [Settings](#settings)
- [Project Structure](#project-structure)
- [Requirements](#requirements)
- [Build From Source](#build-from-source)
- [Packaging on Windows](#packaging-on-windows)
- [Development Notes](#development-notes)
- [Known Limitations](#known-limitations)
- [License](#license)

---

## Overview

Account Assistant is designed for small-business financial tracking and internal reporting. The application organizes data around a 12-month financial period and provides separate modules for:

- Monthly sales, returns, purchases, inventory, and COGS data.
- Expense and account tracking.
- Supplier balance tracking.
- Calculated monthly and total financial results.
- Interactive chart generation.
- PDF report export.
- XLSX import/export for data backup and transfer.

The UI supports both **English** and **Arabic**, including right-to-left layout switching for Arabic mode.

---

## Core Features

### 1. Monthly Data Entry

The Data Entry tab supports two data-entry styles:

- **Card view**: expandable month cards for a modern, section-based workflow.
- **Classic table view**: spreadsheet-style layout for faster bulk input.

Supported monthly fields include:

- Sales
- Sales returns
- Supplier purchases
- Supplier payments
- Inventory opening stock
- Inventory closing stock
- COGS input, when using ongoing inventory mode

### 2. Inventory Modes

Account Assistant supports two inventory calculation modes:

#### Periodic Inventory

Used when COGS is calculated from opening inventory, purchases, and closing inventory.

#### Ongoing Inventory

Used when COGS is entered directly by the user.

When switching inventory mode, the app warns the user because changing the inventory model can clear or invalidate existing entered values. The user can cancel, clear the data, or save the current data before clearing.

### 3. Expenses Tab

The Expenses tab lets users maintain independent expense/account entries outside the monthly table.

Supported fields:

- Account name
- Account type
  - Payable
  - Receivable
- Amount

Additional behavior:

- Duplicate account name validation.
- Sorting by amount.
- Grouping/filtering by account type.
- Expense charts from the Expenses tab.
- XLSX import/export support.
- Dedicated clear button with confirmation warning.

### 4. Suppliers Tab

The Suppliers tab is designed for month-by-month supplier tracking.

Each supplier row includes:

- Supplier name
- Previous balance
- Purchases
- Total debt
- Payments
- Payment percentage of purchases
- Payment percentage of total debt
- Supplier balance

The supplier balance is carried forward to the next month as the next previous balance. Supplier names can also be propagated across months to keep supplier rows aligned.

Supplier rows support:

- Adding suppliers.
- Removing suppliers through the row context menu.
- Automatic calculated result fields.
- Supplier-specific graph generation.
- XLSX import/export.
- Dedicated clear button with confirmation warning.

### 5. Results Tab

The Results tab displays calculated output after the user presses **Calculate**.

It includes:

- Total net sales
- Total COGS
- Total profit/profit margin
- Monthly report cards
- Generated charts
- Hidden charts menu
- Drag-and-drop ordering
- Page separators for PDF layout control
- Page orientation control
- Clear results button with confirmation warning

Charts created from the Expenses and Suppliers tabs are preserved when recalculating from the main Calculate workflow.

---

## Application Workflow

1. Open the application.
2. Choose the inventory mode:
   - Periodic Inventory
   - Ongoing Inventory
3. Enter monthly financial data in the Data Entry tab.
4. Add expense accounts in the Expenses tab, if needed.
5. Add supplier data in the Suppliers tab, if needed.
6. Click **Calculate**.
7. Choose the charts and comparisons that should appear in the Results tab.
8. Review, reorder, hide, duplicate, edit, or remove charts.
9. Export the final report to PDF or save data to XLSX.

---

## Financial Calculations

### Net Sales

```text
Net Sales = Sales - Sales Returns
```

### COGS — Periodic Inventory

```text
COGS = Opening Inventory + Supplier Purchases - Closing Inventory
```

### COGS — Ongoing Inventory

```text
COGS = User-entered COGS Input
```

### Profit / Profit Margin Value

```text
Profit = Net Sales - COGS
```

### Supplier Total Debt

```text
Total Debt = Previous Balance + Purchases
```

### Supplier Payment Percentage of Purchases

```text
Payment % of Purchases = Payments / Purchases × 100
```

If purchases are zero, the percentage is shown as `0%`.

### Supplier Payment Percentage of Total Debt

```text
Payment % of Total Debt = Payments / Total Debt × 100
```

If total debt is zero, the percentage is shown as `0%`.

### Supplier Balance

```text
Supplier Balance = Total Debt - Payments
```

---

## Charts and Reporting

Account Assistant uses **Qt Charts** to generate report visuals.

Supported chart types include:

- Pie chart
- Candle chart
- Grouped bar chart
- Line chart
- Ranked bar chart
- Multi-metric comparison charts

Supported chart metrics include:

- Sales
- Sales return
- Purchases
- Expenses
- Inventory
- Net sales
- COGS
- Profit margin
- Supplier payments
- Supplier previous balance
- Supplier total debt
- Supplier payment percentage of purchases
- Supplier payment percentage of total debt
- Supplier balance
- Expense amount
- Inventory opening
- Inventory closing

### Chart Selection Features

The chart selection dialog supports:

- Metric charts.
- Custom comparisons.
- Multiple metrics in a single comparison.
- Month filtering.
- Automatic month selection based on months that contain data.
- Optional summary point at the end of charts.
- Count-as-100% reference selection for comparison charts.
- Duplicate and remove actions for chart rows.

### Results Layout Features

The Results tab supports:

- Drag-and-drop ordering of month cards and charts.
- Right-click actions for chart cards.
- Chart duplication.
- Chart editing.
- Chart removal.
- Hiding/restoring charts.
- Page separator insertion.
- Portrait or landscape PDF page mode.

---

## Import and Export

### XLSX Export

The app can export:

- Current tab only.
- All data.
- Data Entry sheet.
- Expenses sheet.
- Suppliers sheet.

Generated workbook sheets use structured markers such as:

- `DATA_ENTRY`
- `EXPENSES`
- `SUPPLIERS`

### XLSX Import

The app reads `.xlsx` workbooks and imports recognized sheets.

Import behavior:

- Data Entry imports replace the monthly data-entry section.
- Expenses imports merge new accounts and warn on conflicts.
- Suppliers imports add new supplier rows and replace matching existing supplier rows.
- Numeric fields are strictly validated.
- Text entered into numeric fields causes an import error instead of being silently converted.

### Expected XLSX Formats

#### Data Entry — Periodic Inventory

| Marker | Month | Sales | Sales Return | Supplier Purchases | Supplier Payments | Inventory First | Inventory Last |
|---|---|---:|---:|---:|---:|---:|---:|
| DATA_ENTRY | January | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |

#### Data Entry — Ongoing Inventory

| Marker | Month | Sales | Sales Return | Supplier Purchases | Supplier Payments | COGS Input |
|---|---|---:|---:|---:|---:|---:|
| DATA_ENTRY | January | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |

#### Expenses

| Marker | Account Name | Account Type | Amount |
|---|---|---|---:|
| EXPENSES | Rent | Payable | 0.00 |

#### Suppliers

| Marker | Month | Supplier Name | Previous Balance | Purchases | Total Debt | Payments | Payment % of Purchases | Payment % of Total Debt | Supplier Balance |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|
| SUPPLIERS | January | Supplier A | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |

### PDF Export

PDF export creates an A4 report using the current Results tab layout. The PDF export includes:

- Cover page.
- Summary data.
- Monthly report cards.
- Charts.
- User-controlled page breaks.
- Portrait or landscape orientation.

---

## Settings

The Settings dialog supports:

- Language:
  - English
  - Arabic
- Appearance:
  - Dark mode
  - Light mode
- Currency:
  - USD
  - IQD
- Text size:
  - Normal
  - Large
  - Extra large
- Data Entry view:
  - Card view
  - Classic spreadsheet view

Settings and entered data are persisted locally using Qt `QSettings` under the organization/application name `AccountAssistant`.

---

## Project Structure

```text
AccountAssistant/
├── CMakeLists.txt
└── src/
    ├── main.cpp                       # Application entry point
    ├── mainwindow.h/.cpp              # Main window, tabs, settings, import/export orchestration
    ├── appdata.h                      # Core data models, metrics, calculations, chart requests
    ├── translations.h                 # English/Arabic translation helpers
    ├── themebox.h                     # Themed message/dialog helpers
    ├── Datatablewidget.h/.cpp         # Modern card-based monthly data entry
    ├── ClassicDataTableWidget.h/.cpp  # Classic spreadsheet-style data entry
    ├── Accountswidget.h/.cpp          # Expenses/account management tab
    ├── Supplierswidget.h/.cpp         # Supplier month cards and supplier graph selection
    ├── Chartselectiondialog.h/.cpp    # Metric and comparison chart selection dialog
    ├── Resultswidget.h/.cpp           # Results page, cards, chart rendering, layout control
    ├── Draggablechartcard.h/.cpp      # Draggable chart cards and chart context menu actions
    ├── chartswidget.h/.cpp            # Chart-related UI component
    ├── pdfexporter.h/.cpp             # PDF report rendering and export
    └── settingsdialog.h/.cpp          # Application settings dialog
```

---

## Requirements

### Runtime

- Desktop operating system:
  - Windows
  - Linux
  - macOS
- Qt runtime libraries, when not bundled with the executable.

### Build Requirements

- CMake 3.20 or newer
- C++17-compatible compiler
  - MSVC 2022 on Windows
  - GCC or Clang on Linux
  - Apple Clang on macOS
- Qt 6 with the following modules:
  - Core
  - Gui
  - Widgets
  - Charts
  - PrintSupport
  - Svg
- ZLIB

---

## Build From Source

### Windows — Visual Studio Generator

```powershell
cmake -S . -B build `
  -DCMAKE_PREFIX_PATH="C:/Qt/6.11.0/msvc2022_64" `
  -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake"

cmake --build build --config Release
```

The executable will be generated under:

```text
build/Release/AccountAssistant.exe
```

The project automatically attempts to run `windeployqt` after the Windows build to copy required Qt runtime files next to the executable.

### Windows — Without vcpkg

Use this form if ZLIB is already discoverable by CMake in your environment:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.11.0/msvc2022_64"
cmake --build build --config Release
```

### Linux

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
cmake --build build --config Release
```

### macOS

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/macos
cmake --build build --config Release
```

---

## Packaging on Windows

After building in Release mode, verify that the output folder contains:

- `AccountAssistant.exe`
- Qt DLLs copied by `windeployqt`
- Required Qt plugin folders, such as:
  - `platforms/`
  - `imageformats/`
  - `iconengines/`, if used
  - `tls/`, if required by the deployed Qt build
- `icon.ico`, if provided

If `windeployqt` does not run automatically, run it manually:

```powershell
"C:/Qt/6.11.0/msvc2022_64/bin/windeployqt.exe" build/Release/AccountAssistant.exe
```

---

## Development Notes

### Data Model

The central application state is represented by `AppData` in `src/appdata.h`. It stores:

- Monthly data.
- Supplier entries.
- Expense/account entries.
- Inventory mode.
- Chart requests.
- Results flow order.
- Calculated totals.

### Chart Model

Charts are described using `ChartRequest`. This allows the app to rebuild, edit, duplicate, hide, restore, and export charts consistently across the Results tab and PDF export.

### Local Persistence

The app saves settings and entered data on close and restores them on startup using `QSettings`.

Saved state includes:

- Language
- Theme mode
- Currency
- Font size
- Table view mode
- Monthly data
- Accounts data
- Suppliers data

### UI Design

The application uses Qt Widgets with custom stylesheets. It supports:

- High-DPI rendering.
- Light and dark themes.
- Arabic RTL layout.
- Maximized startup window.
- Styled dialogs and menus.

---

## Known Limitations

- This is a desktop application, not a multi-user accounting server.
- There is no database backend; state is stored locally through `QSettings` and optional XLSX export.
- XLSX support is implemented directly through ZIP/XML parsing and writing. It is suitable for the app's supported templates, but it is not a full Excel engine.
- The app does not process Excel formulas, macros, or advanced workbook features.
- Financial terminology and formulas should be reviewed before using the app for formal accounting compliance.

---

## Recommended Future Improvements

- Align `CMakeLists.txt` version with `main.cpp`.
- Add automated unit tests for calculation logic.
- Add sample XLSX templates to the repository.
- Add screenshots or GIFs to this README.
- Add CI builds for Windows, Linux, and macOS.
- Add a formal license file.
- Add release packaging scripts.

---

## License

No license file was included in the inspected source package.

Until a license is added, this project should be treated as **All Rights Reserved** by default. Add a `LICENSE` file before publishing, distributing, or accepting external contributions.
