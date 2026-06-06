#include "engine/LineupSuggestionEngine.h"

#include <algorithm>
#include <cmath>

// ── Formation registry ───────────────────────────────────────────────────────

QVector<LineupSuggestionEngine::FormationSlots>
LineupSuggestionEngine::supportedFormations()
{
    return {
        { QStringLiteral("4-4-2"),   1, 4, 4, 2 },
        { QStringLiteral("4-3-3"),   1, 4, 3, 3 },
        { QStringLiteral("3-5-2"),   1, 3, 5, 2 },
        { QStringLiteral("4-2-3-1"), 1, 4, 5, 1 },
        { QStringLiteral("5-3-2"),   1, 5, 3, 2 },
    };
}

LineupSuggestionEngine::FormationSlots
LineupSuggestionEngine::parseFormation(const QString &name)
{
    for (const auto &f : supportedFormations())
        if (f.name == name) return f;

    // Fallback: parse "d-m-f" or "d-m1-m2-f"
    const QStringList parts = name.split(QLatin1Char('-'));
    FormationSlots fs;
    fs.name = name;
    if (parts.size() == 3) {
        fs.def = parts[0].toInt();
        fs.mid = parts[1].toInt();
        fs.fwd = parts[2].toInt();
    } else if (parts.size() == 4) {
        fs.def = parts[0].toInt();
        fs.mid = parts[1].toInt() + parts[2].toInt();
        fs.fwd = parts[3].toInt();
    }
    return fs;
}

// ── Constructor ──────────────────────────────────────────────────────────────

LineupSuggestionEngine::LineupSuggestionEngine(QObject *parent)
    : QObject(parent)
{}

// ── Scoring formulae ─────────────────────────────────────────────────────────

double LineupSuggestionEngine::scorePlayer(
    const Models::Player &player,
    Models::Position      role) const
{
    const Models::PlayerStats &s = player.stats;

    // Normalise raw integers to a 0-10 scale using soft caps so elite players
    // stand out without completely drowning weaker ones.
    auto norm = [](double val, double cap) -> double {
        return 10.0 * (1.0 - std::exp(-val / cap));
    };

    switch (role) {
    case Models::Position::Goalkeeper:
        // GK: saves%×0.5 + clean_sheets×0.3 + pass_accuracy×0.2
        return s.savesPercentage * 0.005          // 0-100 → 0-0.5
             + norm(s.cleanSheets, 5.0) * 0.3
             + s.passAccuracy * 0.002;            // 0-100 → 0-0.2

    case Models::Position::Defender:
        // DEF: tackles×0.4 + interceptions×0.3 + aerial_duels_won%×0.2 + pass_accuracy×0.1
        return norm(s.tackles,       30.0) * 0.4
             + norm(s.interceptions, 20.0) * 0.3
             + s.aerialDuelsWonPct   * 0.002      // 0-100 → 0-0.2
             + s.passAccuracy        * 0.001;     // 0-100 → 0-0.1

    case Models::Position::Midfielder:
        // MID: pass_accuracy×0.35 + key_passes×0.25 + dribbles_success%×0.2
        //     + goals×0.1 + assists×0.1
        return s.passAccuracy      * 0.0035       // → 0-0.35
             + norm(s.keyPasses, 40.0) * 0.25
             + s.dribbleSuccessPct * 0.002        // → 0-0.2
             + norm(s.goals,    15.0) * 0.1
             + norm(s.assists,  15.0) * 0.1;

    case Models::Position::Forward:
        // FWD: goals×0.4 + assists×0.2 + shots_on_target%×0.25 + dribbles%×0.15
        return norm(s.goals,   25.0) * 0.4
             + norm(s.assists, 15.0) * 0.2
             + s.shotsOnTargetPct   * 0.0025      // → 0-0.25
             + s.dribbleSuccessPct  * 0.0015;     // → 0-0.15

    default:
        return s.rating * 0.1;
    }
}

// ── Slot picking ─────────────────────────────────────────────────────────────

QVector<Models::SuggestedPlayer> LineupSuggestionEngine::pickSlots(
    const QString              &slotPrefix,
    int                         count,
    Models::Position            role,
    const QVector<Models::Player> &eligible,
    QVector<int>               &usedIds) const
{
    // Score each eligible, not yet used player
    QVector<QPair<double, Models::Player>> scored;
    scored.reserve(eligible.size());
    for (const Models::Player &p : eligible) {
        if (usedIds.contains(p.id)) continue;
        scored.append({ scorePlayer(p, role), p });
    }

    // Sort descending by score
    std::sort(scored.begin(), scored.end(),
              [](const auto &a, const auto &b){ return a.first > b.first; });

    QVector<Models::SuggestedPlayer> result;
    const int take = qMin(count, static_cast<int>(scored.size()));
    for (int i = 0; i < take; ++i) {
        Models::SuggestedPlayer sp;
        sp.slotLabel = (count == 1)
            ? slotPrefix
            : QString("%1-%2").arg(slotPrefix).arg(i + 1);
        sp.player = scored[i].second;
        sp.score  = scored[i].first;
        sp.player.overallScore = sp.score;
        usedIds.append(sp.player.id);
        result.append(sp);
    }
    return result;
}

// ── Public API ───────────────────────────────────────────────────────────────

QVector<Models::SuggestedPlayer> LineupSuggestionEngine::suggestLineup(
    const QString               &formation,
    const QVector<Models::Player> &squad) const
{
    const FormationSlots fs = parseFormation(formation);
    QVector<int> usedIds;
    QVector<Models::SuggestedPlayer> result;

    // Helper: filter squad by primary position, with fallback to Unknown
    auto byPos = [&](Models::Position pos) {
        QVector<Models::Player> out;
        for (const auto &p : squad)
            if (p.position == pos) out.append(p);
        if (out.isEmpty()) {
            // Fallback: all unpositioned players (Unknown)
            for (const auto &p : squad)
                if (p.position == Models::Position::Unknown) out.append(p);
        }
        return out;
    };

    // GK
    result += pickSlots(QStringLiteral("GK"),  fs.gk,  Models::Position::Goalkeeper,
                        byPos(Models::Position::Goalkeeper), usedIds);
    // DEF
    result += pickSlots(QStringLiteral("DEF"), fs.def, Models::Position::Defender,
                        byPos(Models::Position::Defender),   usedIds);
    // MID
    result += pickSlots(QStringLiteral("MID"), fs.mid, Models::Position::Midfielder,
                        byPos(Models::Position::Midfielder), usedIds);
    // FWD
    result += pickSlots(QStringLiteral("FWD"), fs.fwd, Models::Position::Forward,
                        byPos(Models::Position::Forward),    usedIds);

    return result;
}

QMap<QString, double> LineupSuggestionEngine::compareFormations(
    const QVector<Models::Player> &squad) const
{
    QMap<QString, double> scores;
    for (const auto &fs : supportedFormations()) {
        const auto suggestion = suggestLineup(fs.name, squad);
        double total = 0.0;
        for (const auto &sp : suggestion)
            total += sp.score;
        scores.insert(fs.name, total);
    }
    return scores;
}
