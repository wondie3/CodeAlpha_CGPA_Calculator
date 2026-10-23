# CodeAlpha CGPA Calculator (Qt 6 Widgets)

A desktop CGPA calculator written in C++17 with Qt 6 Widgets.

## Features
- Multiple semesters (tabs). Double-click a tab to rename it.
- Add / edit / delete courses in a table with live grade preview.
- Score -> letter grade -> grade points using the CodeAlpha scale (A+ 4.00 ... F 0.00).
- Per-semester GPA and overall CGPA (credit-weighted across all semesters).
- Explicit validation of student name, course name, semester name, score and credit hours.
- Local storage: JSON file, saved atomically (no half-written files), previous version kept as `.bak`,
  strict validation on load (corrupt files are rejected, never crash the app).
- CSV export for Microsoft Excel (UTF-8 BOM, CRLF, quoted fields, formula-injection protection).
- Unsaved-changes prompt on New / Open / Quit.

Data file (auto-loaded at startup, saved with Ctrl+S):
- Windows: `%APPDATA%/CodeAlpha/CGPA Calculator/transcript.json`
- Linux:   `~/.local/share/CodeAlpha/CGPA Calculator/transcript.json`
- macOS:   `~/Library/Application Support/CodeAlpha/CGPA Calculator/transcript.json`

## Project layout
```
CMakeLists.txt
src/
  Grading.*       grade scale, GPA/CGPA maths, input validation (no GUI code)
  Storage.*       JSON load/save and CSV export (no GUI code)
  CourseDialog.*  add/edit course dialog
  MainWindow.*    main window: tabs, tables, menus, summary
  Style.h         application stylesheet (QSS)
  main.cpp
console_version/main.cpp   original console program
```

## Build with Qt Creator
1. Install Qt 6 (Qt Online Installer -> Qt 6.x + a kit for your compiler; CMake and Ninja are included).
2. File -> Open File or Project -> select `CMakeLists.txt`.
3. Pick a Qt 6 kit -> Configure Project -> press Ctrl+R.

## Build with Visual Studio Code
1. Install Qt 6 (as above) and the extensions: **C/C++**, **CMake Tools**, **Qt Extension Pack** (recommended automatically).
2. Create `.vscode/settings.json` and point CMake at your Qt install (adjust version/compiler):
   ```json
   {
     "cmake.configureSettings": {
       "CMAKE_PREFIX_PATH": "C:/Qt/6.7.2/mingw_64"
     }
   }
   ```
   Linux example: `"/opt/Qt/6.7.2/gcc_64"`. On Ubuntu you can instead run `sudo apt install qt6-base-dev cmake ninja-build`
   and skip `CMAKE_PREFIX_PATH`.
3. Ctrl+Shift+P -> **CMake: Select a Kit** (MinGW kit must match the Qt build, e.g. "mingw_64" needs the Qt-bundled MinGW).
4. Ctrl+Shift+P -> **CMake: Configure**, then **CMake: Build**, then **CMake: Run Without Debugging** (Shift+F5).
5. Windows only: if the exe says a Qt DLL is missing, add `C:/Qt/6.7.2/mingw_64/bin` to PATH (or run `windeployqt` on the exe).

## Command line
```
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
cmake --build build
./build/CGPACalculator
```

## Submission checklist (CodeAlpha)
- Repo name: `CodeAlpha_CGPA_Calculator`
- Record a short video of: adding courses, switching semesters, saving/reopening, exporting CSV and opening it in Excel.
