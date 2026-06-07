# ⚽ WorldCup Analyst

A desktop application for building tactical lineups and analysing squads for the **2026 FIFA World Cup**. All 48 qualified nations, 26 real players each — scraped directly from Wikipedia.

---

## Features

| Feature | Description |
|---|---|
| 🌍 **48 Teams** | All 2026 FIFA World Cup squads with real players (Wikipedia data) |
| 🎯 **Drag & Drop** | Drag players from the roster panel onto the tactical pitch |
| 🔄 **Remove & Replace** | Right-click any token → *Remove from XI* → drag in a replacement |
| 🤖 **AI Lineup Suggester** | Weighted scoring algorithm picks the best XI for any formation |
| 📊 **Formation Comparison** | Side-by-side squad score for all 5 supported formations |
| 🏟️ **5 Formations** | 4-4-2 · 4-3-3 · 3-5-2 · 4-2-3-1 · 5-3-2 |
| 📷 **Export PNG** | Render your lineup as a high-res image |
| 🎨 **Dark / Light theme** | Switchable in Settings |
| 💾 **SQLite cache** | Player data cached locally with 24-hour TTL |
| 🔑 **API-Football** | Optional live data via API-Football v3 key (free tier) |

---

## Requirements

### Build dependencies

| Tool | Minimum version |
|---|---|
| Qt 6 (Core, Widgets, Network, Sql, Gui, Svg) | 6.4+ |
| CMake | 3.20+ |
| C++ compiler with C++17 | GCC 11+ / Clang 14+ / MSVC 2022 |
| Python 3 | 3.9+ *(squad scraper only)* |

### Python dependencies *(squad scraper only)*

Listed in [`requirements.txt`](requirements.txt):

```
requests>=2.28.0
beautifulsoup4>=4.12.0
lxml>=4.9.0
```

---

## Quick start

### 1 — Install dependencies

**Linux (Ubuntu/Debian)**
```bash
./setup.sh
```

**macOS (Homebrew)**
```bash
./setup.sh
# Then follow the PATH export instructions it prints
```

**Windows**
1. Download the **Qt 6** online installer from https://www.qt.io/download-qt-installer  
   Select: *Qt 6.x → Desktop → MinGW 64-bit* (or MSVC 2022)
2. Install **CMake** from https://cmake.org/download/
3. Install **Python 3** from https://www.python.org/downloads/

Manual Qt package names by distro:

| Distro | Packages |
|---|---|
| Ubuntu/Debian | `qt6-base-dev` `qt6-tools-dev` `libqt6svg6-dev` `libqt6sql6-sqlite` |
| Fedora | `qt6-qtbase-devel` `qt6-qtsvg-devel` `qt6-qttools-devel` |
| Arch | `qt6-base` `qt6-svg` `qt6-tools` |

### 2 — Clone the repository

```bash
git clone https://github.com/Nx21/lineup.git
cd lineup
```

### 3 — Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

> **Note (Homebrew Qt on macOS):**
> ```bash
> cmake -B build -DCMAKE_BUILD_TYPE=Release \
>       -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
> ```

### 4 — Run

```bash
./build/WorldCupAnalyst        # Linux / macOS
build\WorldCupAnalyst.exe      # Windows
```

---

## Project structure

```
lineup/
├── CMakeLists.txt               # Build definition (Qt6, SQLite, tests)
├── setup.sh                     # One-command dependency installer
├── requirements.txt             # Python deps (squad scraper)
│
├── src/
│   ├── main.cpp
│   ├── api/
│   │   ├── ApiClient.h/.cpp     # Async REST wrapper (API-Football v3)
│   │   └── ApiModels.h          # Team, Player, Fixture, SuggestedPlayer structs
│   ├── db/
│   │   └── DatabaseManager.h/.cpp  # SQLite cache (24-hour TTL)
│   ├── data/
│   │   └── SquadLoader.h/.cpp   # Loads squad JSON from Qt resources
│   ├── engine/
│   │   └── LineupSuggestionEngine.h/.cpp  # Weighted scoring algorithm
│   └── ui/
│       ├── MainWindow.h/.cpp
│       ├── TacticalPitchView.h/.cpp  # QPainter pitch + drag-drop scene
│       ├── PlayerToken.h/.cpp        # Draggable player circle token
│       ├── PlayerRosterWidget.h/.cpp # Filterable table view
│       ├── SuggestionPanel.h/.cpp    # Suggested XI dock widget
│       └── SettingsDialog.h/.cpp     # API key, theme, cache settings
│
├── resources/
│   ├── resources.qrc            # Qt resource manifest
│   ├── images/pitch.svg
│   └── squads/                  # 48 × JSON squad files + teams_index.json
│
├── tests/
│   └── test_suggestion_engine.cpp  # 17 Qt Test unit tests
│
└── tools/
    └── scrape_squads.py         # Wikipedia squad scraper
```

---

## How it works

### Lineup Suggestion Engine

For each formation slot the engine scores eligible players using a weighted formula:

| Role | Formula |
|---|---|
| **GK** | `saves% × 0.5 + clean_sheets × 0.3 + pass_accuracy × 0.2` |
| **DEF** | `tackles × 0.4 + interceptions × 0.3 + aerial_duels × 0.2 + pass_accuracy × 0.1` |
| **MID** | `pass_accuracy × 0.35 + key_passes × 0.25 + dribbles × 0.2 + goals × 0.1 + assists × 0.1` |
| **FWD** | `goals × 0.4 + assists × 0.2 + shots_on_target% × 0.25 + dribbles × 0.15` |

Players are scored with exponential normalisation, picked without duplication, and ranked in the Suggestion Panel.

### Squad data pipeline

```
Wikipedia HTML  →  tools/scrape_squads.py  →  resources/squads/*.json
                                                      ↓
                                           Qt resources (compiled in)
                                                      ↓
                                           src/data/SquadLoader.cpp
                                                      ↓
                                           SQLite cache (DatabaseManager)
                                                      ↓
                                           UI (roster, pitch, suggestions)
```

To refresh squad data from Wikipedia:
```bash
pip install -r requirements.txt       # first time only
python3 tools/scrape_squads.py
cmake --build build --parallel        # recompile resources
```

---

## Optional: Live API data

The app works fully offline using the bundled Wikipedia squads. To enable live data from **API-Football**:

1. Register for a free key at https://dashboard.api-sports.io/register  
   *(free tier: 100 requests/day)*
2. Launch the app → click **⚙ Settings** → paste your key → OK
3. Live squads are fetched and cached for 24 hours

---

## Running the tests

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
cd build && ctest --output-on-failure
```

Expected: **17/17 tests pass**

---

## Usage tips

| Action | How |
|---|---|
| Select a team | Click team name in left sidebar |
| Change formation | Use the dropdown in the toolbar |
| Add player to pitch | Drag from the roster panel → drop onto the pitch |
| Remove player from XI | Right-click token → *✕ Remove from XI* |
| Replace a player | Remove → drag a new player from the roster |
| Auto-suggest XI | Click **⚡ Suggest Lineup** → **Apply** |
| Compare formations | Click **Compare Formations** in the suggestion panel |
| Export lineup | Click **📷 Export PNG** |

---

## License

MIT © 2026
