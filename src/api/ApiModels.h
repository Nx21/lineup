#pragma once

#include <QString>
#include <QVector>
#include <QDateTime>

namespace Models {

// ── Position ─────────────────────────────────────────────────────────────────

enum class Position {
    Unknown,
    Goalkeeper,
    Defender,
    Midfielder,
    Forward
};

inline Position positionFromString(const QString &s)
{
    const QString lc = s.toLower();
    if (lc == "goalkeeper" || lc == "g")  return Position::Goalkeeper;
    if (lc == "defender"   || lc == "d")  return Position::Defender;
    if (lc == "midfielder" || lc == "m")  return Position::Midfielder;
    if (lc == "attacker"   || lc == "f"
        || lc == "forward" || lc == "a")  return Position::Forward;
    return Position::Unknown;
}

inline QString positionToString(Position p)
{
    switch (p) {
    case Position::Goalkeeper: return QStringLiteral("GK");
    case Position::Defender:   return QStringLiteral("DEF");
    case Position::Midfielder: return QStringLiteral("MID");
    case Position::Forward:    return QStringLiteral("FWD");
    default:                   return QStringLiteral("UNK");
    }
}

inline QString positionFullName(Position p)
{
    switch (p) {
    case Position::Goalkeeper: return QStringLiteral("Goalkeeper");
    case Position::Defender:   return QStringLiteral("Defender");
    case Position::Midfielder: return QStringLiteral("Midfielder");
    case Position::Forward:    return QStringLiteral("Forward");
    default:                   return QStringLiteral("Unknown");
    }
}

// ── PlayerStats ──────────────────────────────────────────────────────────────

struct PlayerStats {
    // General
    int    appearances       = 0;
    int    minutesPlayed     = 0;
    double rating            = 0.0;   // 0-10 average

    // Attacking
    int    goals             = 0;
    int    assists           = 0;
    int    shots             = 0;
    int    shotsOnTarget     = 0;
    double shotsOnTargetPct  = 0.0;   // 0-100

    // Passing
    int    passes            = 0;
    int    keyPasses         = 0;
    double passAccuracy      = 0.0;   // 0-100

    // Dribbling
    int    dribblesAttempted = 0;
    int    dribblesSuccess   = 0;
    double dribbleSuccessPct = 0.0;   // 0-100

    // Defending
    int    tackles           = 0;
    int    interceptions     = 0;
    int    aerialDuelsWon    = 0;
    int    aerialDuelsTotal  = 0;
    double aerialDuelsWonPct = 0.0;   // 0-100

    // Goalkeeping
    int    saves             = 0;
    int    goalsConceded     = 0;
    double savesPercentage   = 0.0;   // 0-100
    int    cleanSheets       = 0;
};

// ── Player ───────────────────────────────────────────────────────────────────

struct Player {
    int        id            = 0;
    int        teamId        = 0;
    QString    name;
    QString    firstname;
    QString    lastname;
    int        age           = 0;
    QString    nationality;
    Position   position      = Position::Unknown;
    int        jerseyNumber  = 0;
    QString    photoUrl;
    PlayerStats stats;
    double     overallScore  = 0.0;   // computed by LineupSuggestionEngine
};

// ── Team ─────────────────────────────────────────────────────────────────────

struct Team {
    int             id        = 0;
    QString         name;
    QString         shortName;
    QString         country;
    QString         logoUrl;
    QVector<Player> players;
};

// ── Fixture ──────────────────────────────────────────────────────────────────

struct Fixture {
    int      id           = 0;
    QDateTime date;
    int      homeTeamId   = 0;
    int      awayTeamId   = 0;
    QString  homeTeamName;
    QString  awayTeamName;
    int      homeScore    = -1;
    int      awayScore    = -1;
    QString  status;
    QString  round;
    int      leagueId     = 0;
    int      season       = 2026;
};

// ── SuggestedPlayer ──────────────────────────────────────────────────────────

struct SuggestedPlayer {
    QString  slotLabel;   // e.g. "GK", "DEF-1", "MID-3"
    Player   player;
    double   score = 0.0;
};

} // namespace Models
