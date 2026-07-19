# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

### Windows (Qt 6, MinGW)
```batch
set QTDIR=C:\Qt\6.11.0\mingw_64
.\Windows\build.bat
```
Binary: `Windows\build\release\Statistiques.exe`

### Linux (GCC)
```bash
chmod +x linux/build.sh
./linux/build.sh
```
Binary: `linux/build/Statistiques`

### Manual build (any platform)
```bash
mkdir build && cd build
qmake ../Statistiques.pro
make -j$(nproc)
```

## Project Structure

**Core Application:**
- `Statistiques.cpp` — Main Qt GUI: QMainWindow with menus (File, Data, Regression), QSplitter with chart + results pane
- `statsengine.h/.cpp` — Regression computation engine supporting 9 modes + auto-select best fit
- `chartwidget.h/.cpp` — Interactive chart widget: mouse drag to pan, wheel to zoom, displays fitted curve + data points

**Configuration:**
- `Statistiques.pro` — Qt project file (qmake), cross-platform
- `Windows/Statistiques.pro` — Windows-specific .pro (relative paths)
- `includes/` — Legacy C headers (DEF.HCL, FCT.HL, RGM.HL, STATS.HL) from original 1996 Borland C++ DOS program

**Data & Examples:**
- `exemples/` — 6 sample datasets (.txt files): capacitor discharge, pendulum, yeast growth, projectile, enzyme kinetics, allometry

## Code Architecture

### StatsEngine Class
Encapsulates all regression computation. Main methods:
- `compute(xd, yd)` — Run current regression mode on data
- `autoMode(xd, yd)` — Test all 9 modes, return index of best fit (highest r²)
- `regFY(x)` — Evaluate fitted curve at x
- `regFX(y)` — Inverse: evaluate x for given y (for reciprocal/logistic modes)

**Regression modes (0-8):**
- Modes 0-3: Log-linearization (MCO after transform) — linear, logarithmic, exponential, power
- Mode 4: Power NLS — Gauss-Newton, handles x=0
- Mode 5: Reciprocal — MCO on X'=1/x
- Mode 6: Polynomial deg 2 — Cramer 3×3 solve
- Modes 7-8: NLS Gauss-Newton 4/3 params — sinusoidal, logistic

**Key implementation notes:**
- Power (mode 3): tx=ty=0, excludes points with x≤0 or y≤0 from parameter estimation (they carry no power-law info), but includes them in r²/cov calculation
- Power NLS (mode 4): Initialized via log-linearization, then refines; naturally handles x=0
- Sinusoidal (mode 7): Initializes b by zero-crossing count, c by linear correlation of sin/cos projections
- Logistic (mode 8): 3-parameter fit, may diverge if data not sigmoid-shaped

### ChartWidget Class
Renders regression + data interactively:
- Curves drawn via `regFY()` sampling
- Pan (mouse drag), zoom (wheel), reset auto-range button
- Grid, axes, formula + stats overlay with sign display logic (no "+−" for negative c/d)

### Main GUI (Statistiques.cpp)
- Menu actions: manual data input → dialogs for n points → loop over X/Y entry
- File I/O: load .txt/.dat/.fch, parse "n_points \n x₁ y₁ \n ..." format
- Generate from math function: sin, cos, exp, log, sqrt, x², x³
- Auto-regression triggered after every data modification
- Export results to file (parameters, stats, fitted equation)

## Data Format

Text files with space/tab-separated values:
```
5                    # Number of points (first line)
1.0   2.3            # X Y pairs
2.0   4.1
3.0   5.9
4.0   8.2
5.0   9.8
```

## Development Notes

- **Qt Version:** Qt 6.11.0 (Windows), standard system Qt (Linux)
- **C++ Standard:** C++17
- **Math Library:** libm on Linux (`CONFIG += linux` in .pro)
- **No external dependencies:** Pure Qt + C++ standard library
- All data handling uses `std::vector<double>` internally, conversion to `QVector<QPointF>` for display
