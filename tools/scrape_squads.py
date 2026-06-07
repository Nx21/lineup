#!/usr/bin/env python3
"""
scrape_squads.py
────────────────
Fetches the official 2026 FIFA World Cup squads from Wikipedia and
writes one JSON file per team into resources/squads/.

Usage:
    python3 tools/scrape_squads.py

Output:
    resources/squads/<team_name>.json   (one per team, 48 total)
    resources/squads/teams_index.json   (master index listing every team)

JSON schema per team file:
{
  "id":        <integer unique id>,
  "name":      "Brazil",
  "short":     "BRA",
  "country":   "Brazil",
  "group":     "C",
  "coach":     "Carlo Ancelotti",
  "players": [
    {
      "id":           <integer>,
      "jersey":       <integer or 0>,
      "position":     "GK" | "DF" | "MF" | "FW",
      "name":         "Vinicius Jr",
      "dob":          "1900-07-12",
      "age":          24,
      "caps":         <integer>,
      "goals":        <integer>,
      "club":         "Real Madrid",
      "club_country": "Spain"
    },
    ...
  ]
}
"""

import re
import json
import time
import hashlib
import os
import unicodedata
from pathlib import Path

import requests
from bs4 import BeautifulSoup, Tag

# ── Config ────────────────────────────────────────────────────────────────────

WIKI_URL   = "https://en.wikipedia.org/wiki/2026_FIFA_World_Cup_squads"
OUT_DIR    = Path(__file__).parent.parent / "resources" / "squads"
HEADERS    = {"User-Agent": "WorldCupAnalyst/1.0 (scrape_squads.py; educational project)"}

# ── Helpers ───────────────────────────────────────────────────────────────────

def slugify(text: str) -> str:
    """Lower-case ASCII slug suitable as a filename."""
    text = unicodedata.normalize("NFKD", text)
    text = text.encode("ascii", "ignore").decode("ascii")
    text = re.sub(r"[^a-z0-9]+", "_", text.lower()).strip("_")
    return text

def stable_id(team_slug: str, player_name: str, index: int) -> int:
    """Deterministic integer id derived from team + player name."""
    raw = f"{team_slug}:{player_name}:{index}"
    return int(hashlib.md5(raw.encode()).hexdigest()[:8], 16) % 100000 + 1

def clean(text: str) -> str:
    """Strip footnote markers and extra whitespace."""
    text = re.sub(r"\[.*?\]", "", text)   # [1], [note 2] etc.
    text = re.sub(r"\s+", " ", text)
    return text.strip()

def parse_dob(cell_text: str):
    """
    Wikipedia DoB cells look like:  "(1994-11-05)5 November 1994"
    or just "5 November 1994". Extract ISO date and age.
    """
    iso_match = re.search(r"\((\d{4}-\d{2}-\d{2})\)", cell_text)
    age_match = re.search(r"aged?\s*(\d{1,3})", cell_text, re.IGNORECASE)
    age_match2 = re.search(r"\((\d{1,3})\)", cell_text)

    dob = iso_match.group(1) if iso_match else ""
    age = 0
    if age_match:
        age = int(age_match.group(1))
    elif age_match2 and not iso_match:
        age = int(age_match2.group(1))
    elif dob:
        # estimate from birth year
        from datetime import date
        try:
            birth_year = int(dob[:4])
            age = 2026 - birth_year
        except Exception:
            pass
    return dob, age

def parse_pos(raw: str) -> str:
    raw = re.sub(r"^\d+", "", raw).strip().upper()  # strip leading jersey number
    mapping = {
        "GK": "GK", "G": "GK",
        "DF": "DF", "D": "DF", "CB": "DF", "RB": "DF", "LB": "DF",
        "MF": "MF", "M": "MF", "CM": "MF", "AM": "MF", "DM": "MF",
        "FW": "FW", "F": "FW", "CF": "FW", "ST": "FW", "SS": "FW",
        "RW": "FW", "LW": "FW",
    }
    return mapping.get(raw, "MF")   # default mid if unknown

def pos_order(pos: str) -> int:
    return {"GK": 0, "DF": 1, "MF": 2, "FW": 3}.get(pos, 4)

# ── Scrape ────────────────────────────────────────────────────────────────────

def fetch_page() -> BeautifulSoup:
    print(f"Fetching {WIKI_URL} …")
    resp = requests.get(WIKI_URL, headers=HEADERS, timeout=30)
    resp.raise_for_status()
    return BeautifulSoup(resp.text, "lxml")

def parse_squads(soup: BeautifulSoup) -> list[dict]:
    """
    Walk the page structure:
      h2  → Group letter (e.g. "Group A")
      h3  → Team name
      table.wikitable → squad table
    """
    teams = []
    current_group = ""
    team_counter   = 1

    content = soup.find("div", {"id": "mw-content-text"})
    if not content:
        raise RuntimeError("Could not find page content div")

    # We iterate over all headings and tables in document order
    elements = content.find_all(["h2", "h3", "table"])

    i = 0
    while i < len(elements):
        el = elements[i]

        # ── Group heading ─────────────────────────────────────────────────────
        if el.name == "h2":
            txt = clean(el.get_text())
            m = re.match(r"Group\s+([A-Z])", txt, re.IGNORECASE)
            if m:
                current_group = m.group(1).upper()
            i += 1
            continue

        # ── Team heading ──────────────────────────────────────────────────────
        if el.name == "h3":
            team_name = clean(el.get_text())
            # Skip non-team h3s (e.g. "Notes", "References")
            if not current_group or not re.search(r"[A-Z]", team_name):
                i += 1
                continue

            # Look ahead for a wikitable that's a squad table
            table = None
            j = i + 1
            while j < len(elements):
                candidate = elements[j]
                if candidate.name == "h3":
                    break   # next team, no table found
                if candidate.name == "h2":
                    break
                if candidate.name == "table":
                    # Confirm it looks like a squad table (has "Pos" header)
                    header_text = candidate.get_text()
                    if re.search(r"\bPos\b", header_text, re.IGNORECASE):
                        table = candidate
                        break
                j += 1

            if table is None:
                i += 1
                continue

            # ── Extract coach from the paragraph between h3 and the table ──
            coach = ""
            for sib in el.find_next_siblings():
                if sib == table:
                    break
                if hasattr(sib, "get_text"):
                    txt = sib.get_text()
                    m_coach = re.search(r"Coach[:\s]+([^\n\[]+)", txt)
                    if m_coach:
                        coach = clean(m_coach.group(1))
                        break

            # ── Parse squad table ─────────────────────────────────────────────
            players = parse_squad_table(table, team_name, team_counter)

            # ── Derive short name ─────────────────────────────────────────────
            short = slugify(team_name)[:3].upper()

            team = {
                "id":      team_counter,
                "name":    team_name,
                "short":   short,
                "country": team_name,
                "group":   current_group,
                "coach":   coach,
                "players": players,
            }
            teams.append(team)
            team_counter += 1

        i += 1

    return teams


def parse_squad_table(table: Tag, team_name: str, team_id: int) -> list[dict]:
    """Parse a wikitable squad table into a list of player dicts."""
    team_slug = slugify(team_name)
    players   = []

    rows = table.find_all("tr")
    # Find header row to map column indices
    header_row = None
    for row in rows:
        cells = row.find_all(["th", "td"])
        texts = [clean(c.get_text()).upper() for c in cells]
        if any("POS" in t for t in texts):
            header_row = texts
            break

    if not header_row:
        return players

    def col(names):
        for name in names:
            for idx, h in enumerate(header_row):
                if name in h:
                    return idx
        return None

    idx_no    = col(["NO.", "#", "NO"])
    idx_pos   = col(["POS"])
    idx_name  = col(["NAME", "PLAYER"])
    idx_dob   = col(["DATE", "DOB", "BIRTH"])
    idx_caps  = col(["CAPS", "CAP"])
    idx_goals = col(["GOALS", "GLS"])
    idx_club  = col(["CLUB"])

    if idx_pos is None or idx_name is None:
        return players

    player_idx = 0
    for row in rows:
        cells = row.find_all(["th", "td"])
        if len(cells) < 2:
            continue
        texts = [clean(c.get_text()) for c in cells]

        # Skip header rows
        if any("Pos" in t or "POS" in t for t in texts):
            continue

        def get(idx):
            if idx is None or idx >= len(texts):
                return ""
            return texts[idx].strip()

        pos_raw = get(idx_pos)
        if not pos_raw or pos_raw.upper() in ("POS", "POSITION"):
            continue

        pos      = parse_pos(pos_raw)
        name_raw = get(idx_name)
        if not name_raw or name_raw.lower() in ("name", "player"):
            continue

        # Remove (c) captain marker and trailing spaces
        name = re.sub(r"\s*\(c\)\s*", "", name_raw).strip()
        name = re.sub(r"\s+", " ", name)

        dob_raw      = get(idx_dob)
        dob, age     = parse_dob(dob_raw)

        try:
            jersey = int(get(idx_no))
        except (ValueError, TypeError):
            jersey = 0

        try:
            caps = int(re.sub(r"\D", "", get(idx_caps))) if idx_caps else 0
        except (ValueError, TypeError):
            caps = 0

        try:
            goals = int(re.sub(r"\D", "", get(idx_goals))) if idx_goals else 0
        except (ValueError, TypeError):
            goals = 0

        # Club — may have "Club (Country)" format
        club_raw     = get(idx_club)
        club_match   = re.match(r"^(.+?)\s*\(([^)]+)\)\s*$", club_raw)
        if club_match:
            club         = club_match.group(1).strip()
            club_country = club_match.group(2).strip()
        else:
            club         = club_raw
            club_country = ""

        player_id = team_id * 1000 + player_idx + 1

        players.append({
            "id":           player_id,
            "jersey":       jersey,
            "position":     pos,
            "name":         name,
            "dob":          dob,
            "age":          age,
            "caps":         caps,
            "goals":        goals,
            "club":         club,
            "club_country": club_country,
        })
        player_idx += 1

    # Sort by position order then jersey
    players.sort(key=lambda p: (pos_order(p["position"]), p["jersey"] or 99))
    return players


# ── Write output ──────────────────────────────────────────────────────────────

def write_output(teams: list[dict]):
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    index = []
    for team in teams:
        slug     = slugify(team["name"])
        filename = f"{slug}.json"
        filepath = OUT_DIR / filename

        with open(filepath, "w", encoding="utf-8") as f:
            json.dump(team, f, ensure_ascii=False, indent=2)

        print(f"  ✓  {team['name']:30s}  {len(team['players']):2d} players  →  {filename}")

        index.append({
            "id":    team["id"],
            "name":  team["name"],
            "short": team["short"],
            "group": team["group"],
            "file":  filename,
        })

    index_path = OUT_DIR / "teams_index.json"
    with open(index_path, "w", encoding="utf-8") as f:
        json.dump({"teams": index}, f, ensure_ascii=False, indent=2)
    print(f"\n  ✓  Index written → teams_index.json  ({len(index)} teams)")


# ── Entry point ───────────────────────────────────────────────────────────────

if __name__ == "__main__":
    soup  = fetch_page()
    teams = parse_squads(soup)
    if not teams:
        print("ERROR: No teams parsed. Wikipedia page structure may have changed.")
        raise SystemExit(1)
    print(f"\nParsed {len(teams)} teams:\n")
    write_output(teams)
    print("\nDone.")
