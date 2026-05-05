# Account Assistant

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![Qt 6](https://img.shields.io/badge/Qt-6.x-green.svg)
![CMake](https://img.shields.io/badge/Build-CMake-informational.svg)
![License](https://img.shields.io/badge/License-AGPL--3.0--or--later-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)

**Account Assistant** is a desktop accounting and financial reporting application built with **C++17** and **Qt 6**. It helps users enter monthly accounting data, manage expense accounts, track other revenues and suppliers, calculate real-time financial results, generate charts, and export professional reports to **PDF** and **XLSX**.

> Current application version: **7.1.0**  
> Runtime version in `main.cpp` and CMake project version in `CMakeLists.txt` should remain aligned.

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
- [Recommended Future Improvements](#recommended-future-improvements)
- [License](#license)

---

## Overview

Account Assistant is designed for small-business financial tracking and internal reporting. The application organizes accounting data around a 12-month financial period and provides separate modules for:

- Monthly sales, returns, purchases, inventory, and COGS data.
- Month-based fixed expense/account tracking.
- Other revenues tracking.
- Supplier balance tracking.
- Real-time calculated results.
- Operating profit and summary reporting.
- Interactive chart generation.
- PDF report export.
- XLSX import/export for backup, review, and transfer.

The UI supports both **English** and **Arabic**, including right-to-left layout switching for Arabic mode.

---

## Core Features

### 1. Monthly Data Entry

The **Data Entry** tab supports two data-entry styles:

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

Money fields follow the selected currency and disable mouse-wheel value changes to prevent accidental edits.

### 2. Inventory Modes

Account Assistant supports two inventory calculation modes.

#### Periodic Inventory

Used when COGS is calculated from opening inventory, purchases, and closing inventory.

#### Ongoing Inventory

Used when COGS is entered directly by the user.

When switching inventory mode, the app warns the user because changing the inventory model can clear or invalidate existing entered values. The user can cancel, clear the data, or save the current data before clearing.

### 3. Expenses Tab

The **Expenses** tab is a month-based account entry screen. It uses the same 12-month workflow as the rest of the application, allowing the user to select one month at a time and enter values for that month.

Each month contains the same account list so users can enter account values month by month without rebuilding the account structure.

Each account row includes:

- Account name
- Amount
- Account type:
  - Account Receivable
  - Account Payable

Additional behavior:

- Fixed default account list.
- Add Account button.
- Right-click account deletion.
- Newly added accounts are added to all months.
- Deleted accounts are removed from all months.
- Group filter:
  - All
  - Account Receivable
  - Account Payable
- Dedicated Clear Data button with confirmation warning.
- Amount fields have no spin arrows.
- Amount fields are not affected by mouse-wheel scrolling.
- XLSX import/export support.

### 4. Other Revenues Tab

The **Other Revenues** tab tracks additional non-trading revenue streams on a month-by-month basis.

It follows the same 12-month dropdown structure used in the Data Entry tab. Each month includes two fields:

- Acquired privileges
- Other miscellaneous revenues

Other Revenues behavior:

- Values are stored separately for each month.
- Money fields follow the selected currency.
- IQD values do not display unnecessary `.00` decimals.
- Fields have no spin arrows.
- Fields are not affected by mouse-wheel scrolling.
- Dedicated Clear Data button with confirmation warning.
- XLSX import/export support.

### 5. Suppliers Tab

The **Suppliers** tab is designed for month-by-month supplier tracking.

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
- Dedicated Clear Data button with confirmation warning.

### 6. Results Tab

The **Results** tab now updates financial numbers in real time. The user no longer needs to press Calculate to refresh result totals.

It includes:

- Total net sales
- Total COGS
- Total Trading Result
- Total Operating Profit
- Monthly report cards
- Generated charts
- Hidden charts menu
- Drag-and-drop ordering
- Page separators for PDF layout control
- Page orientation control
- Clear Results button with confirmation warning

The old **Calculate** button behavior has been replaced with **Create Graph**. Results are calculated automatically in the background, while **Create Graph** is used for chart and comparison generation.

Charts created from the Expenses and Suppliers tabs are preserved when creating new graphs from the main Results workflow.

### 7. Summary Tab

The **Summary** tab, translated in Arabic as **الخلاصة**, calculates the final operating summary in the background.

It shows the expense accounts and their signed contribution next to them:

- Account Payable values are treated as negative.
- Account Receivable values are treated as positive.

The Summary tab includes:

- Signed expense account list.
- Other Revenues contribution.
- Trading Result contribution.
- Operating Profit output.
- Real-time recalculation.
- Clear Summary button with confirmation warning.

---

## Application Workflow

1. Open the application.
2. Choose the inventory mode:
   - Periodic Inventory
   - Ongoing Inventory
3. Enter monthly financial data in the Data Entry tab.
4. Enter monthly account values in the Expenses tab.
5. Enter monthly Other Revenues values, if needed.
6. Add supplier data in the Suppliers tab, if needed.
7. Review real-time calculations in the Results and Summary tabs.
8. Use **Create Graph** to generate charts and custom comparisons.
9. Review, reorder, hide, duplicate, edit, or remove charts.
10. Export the final report to PDF or save data to XLSX.

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

### Trading Result

```text
Trading Result = Net Sales - COGS
```

### Other Revenues

```text
Other Revenues = Acquired Privileges + Other Miscellaneous Revenues
```

### Signed Expenses Total

```text
Signed Expenses Total = Total Account Receivable - Total Account Payable
```

Receivable accounts are added as positive values. Payable accounts are subtracted as negative values.

### Operating Profit

```text
Operating Profit = Trading Result + Other Revenues + Signed Expenses Total
```

Because payable expenses are already negative in the signed expenses total, they reduce Operating Profit automatically.

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
- Horizontal bar chart
- Line chart
- Ranked bar chart
- Multi-metric comparison charts

Supported chart metrics include:

- Sales
- Sales return
- Purchases
- Expenses
- Other revenues
- Acquired privileges
- Other miscellaneous revenues
- Inventory
- Net sales
- COGS
- Trading Result
- Operating Profit
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
- X/Y axis metric selection.
- Month filtering.
- Automatic month selection based on months that contain data.
- Optional summary point at the end of charts.
- Count-as-100% reference selection for comparison charts.
- Duplicate and remove actions for chart rows.

### Chart Display Improvements

Charts include these display improvements:

- Money values are shortened in chart axes and labels, such as `1.1B`, `950M`, and `42K`.
- Chart colors are spaced apart to avoid confusing similar colors in the same chart.
- Pie chart labels and legends are optimized for readability.
- Zero-value pie slices can be excluded from legends where appropriate.

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
- Other Revenues sheet.
- Suppliers sheet.

Generated workbook sheets use structured markers such as:

- `DATA_ENTRY`
- `EXPENSES`
- `OTHER_REVENUES`
- `SUPPLIERS`

### XLSX Import

The app reads `.xlsx` workbooks and imports recognized sheets.

Import behavior:

- Data Entry imports replace the monthly data-entry section.
- Expenses imports update month-based account data.
- Other Revenues imports replace monthly other revenue values.
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

| Marker | Month | Account Name | Amount | Account Type |
|---|---|---|---:|---|
| EXPENSES | January | Salaries and wages | 0.00 | Account Payable |

#### Other Revenues

| Marker | Month | Acquired Privileges | Other Miscellaneous Revenues |
|---|---|---:|---:|
| OTHER_REVENUES | January | 0.00 | 0.00 |

#### Suppliers

| Marker | Month | Supplier Name | Previous Balance | Purchases | Total Debt | Payments | Payment % of Purchases | Payment % of Total Debt | Supplier Balance |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|
| SUPPLIERS | January | Supplier A | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |

### PDF Export

PDF export creates an A4 accounting report using the current Results tab layout.

The exported PDF includes:

- Report title: **Accounting Report**
- Summary data
- Monthly report cards
- Operating Profit values
- Charts
- User-controlled page breaks
- Portrait or landscape orientation

The PDF no longer shows the removed first-page account list from older versions.

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

### Currency Display Rules

Currency display follows the selected language and currency:

- English IQD format: `111,444,222 IQD`
- Arabic IQD format: `دع 111,444,222`
- IQD values are displayed without unnecessary `.00` decimals.
- USD values keep normal decimal formatting where appropriate.

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
    ├── Accountswidget.h/.cpp          # Month-based expenses/account management tab
    ├── OtherRevenuesWidget.h/.cpp     # Month-based other revenues tab
    ├── Supplierswidget.h/.cpp         # Supplier month cards and supplier graph selection
    ├── SummaryWidget.h/.cpp           # Summary tab and operating profit breakdown
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
- Expense/account entries.
- Other revenues.
- Supplier entries.
- Inventory mode.
- Chart requests.
- Results flow order.
- Calculated totals.
- Operating profit data.

### Chart Model

Charts are described using `ChartRequest`. This allows the app to rebuild, edit, duplicate, hide, restore, and export charts consistently across the Results tab and PDF export.

### Real-Time Recalculation

Results and Summary values are recalculated automatically after data changes. Recalculation is debounced so rapid field edits do not repeatedly rebuild heavy UI sections.

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
- Other revenues data
- Suppliers data
- Generated chart requests and result layout state

### UI Design

The application uses Qt Widgets with custom stylesheets. It supports:

- High-DPI rendering.
- Light and dark themes.
- Arabic RTL layout.
- Maximized startup window.
- Styled dialogs and menus.
- Safer language switching between English and Arabic.

---

## Known Limitations

- This is a desktop application, not a multi-user accounting server.
- There is no database backend; state is stored locally through `QSettings` and optional XLSX export.
- XLSX support is implemented directly through ZIP/XML parsing and writing. It is suitable for the app's supported templates, but it is not a full Excel engine.
- The app does not process Excel formulas, macros, or advanced workbook features.
- Financial terminology and formulas should be reviewed before using the app for formal accounting compliance.

---

## Recommended Future Improvements

- Add automated unit tests for calculation logic.
- Add sample XLSX templates to the repository.
- Add screenshots or GIFs to this README.
- Add CI builds for Windows, Linux, and macOS.
- Add release packaging scripts.
- Add optional database-backed multi-user mode.

---

## License

This project is licensed under the **GNU Affero General Public License v3.0 or later (AGPL-3.0-or-later)**.

See the included [`LICENSE`](LICENSE) file for the full license text.

The AGPL is a strong copyleft license. If the software is modified and made available to users over a network, the modified source code must also be made available under the same license terms.
