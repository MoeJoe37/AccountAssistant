# Account Assistant

A bilingual (English / Arabic) Qt 6 accounting data-entry application with
interactive pie and candlestick charts and PDF report export.

---

## Features

| Section | Fields |
|---------|--------|
| **Sales** | 12 monthly figures |
| **Sales Return** | 12 monthly figures |
| **Purchases** | Supplier Purchases · Supplier Payments |
| **Expenses** | Dynamic table – Account Name · Month · Amount |
| **Inventory** | 12 × First Period + 12 × Last Period |

Each section has **Pie Chart** and **Candle Chart** checkboxes to control which
charts are rendered on the right panel.

### Calculations
```
Cost of Goods Sold  = Σ First Period  +  Supplier Purchases  +  Σ Last Period
Net Sales           = Σ Sales  −  Σ Sales Return
Profit Margin       = Cost of Goods Sold  −  Net Sales
```

### Other features
- **Language switching** — Settings button → English / Arabic (full RTL layout)
- **PDF export** — exports summary figures + all visible charts to a PDF file

---

## Prerequisites

| Tool | Version |
|------|---------|
| CMake | ≥ 3.20 |
| vcpkg | latest |
| Qt 6 | ≥ 6.4 (via vcpkg or system) |
| C++ compiler | MSVC 2022 / GCC 12+ / Clang 15+ |

---

## Build

### 1. Install dependencies via vcpkg
```bash
# Clone vcpkg if you don't have it
git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh   # Linux/macOS
# or
.\vcpkg\bootstrap-vcpkg.bat  # Windows

# Install Qt packages (this may take a while)
./vcpkg/vcpkg install qtbase qtcharts qtsvg
```

### 2. Configure & build
```bash
cmake -B build \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

> **Windows (Visual Studio)**  
> ```bat
> cmake -B build -G "Visual Studio 17 2022" -A x64 ^
>   -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
> cmake --build build --config Release
> ```

### 3. Run
```bash
./build/AccountAssistant          # Linux/macOS
build\Release\AccountAssistant.exe  # Windows
```

---

## Project structure

```
AccountAssistant/
├── CMakeLists.txt
├── vcpkg.json
└── src/
    ├── main.cpp
    ├── mainwindow.h / .cpp      — main window, data entry tabs
    ├── chartswidget.h / .cpp    — pie & candlestick chart panel
    ├── pdfexporter.h / .cpp     — PDF export via QPrinter
    ├── settingsdialog.h / .cpp  — language settings dialog
    ├── translations.h           — bilingual string helper (T macro)
    └── appdata.h                — shared data structures + calculations
```

---

## Troubleshooting

**Qt Charts not found**  
Make sure `qtcharts` is installed in vcpkg and the toolchain file is set.

**Arabic text not rendering**  
Install an Arabic system font (e.g. *Tahoma*, *Arial*, *Segoe UI*). Qt uses the
system font stack for Unicode shaping.

**windeployqt not found**  
Run `windeployqt.exe AccountAssistant.exe` manually from your Qt bin directory
after building.
