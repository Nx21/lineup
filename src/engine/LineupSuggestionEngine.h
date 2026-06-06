#pragma once

#include <QObject>
#include <QVector>
#include <QMap>
#include <QString>
#include "api/ApiModels.h"

class LineupSuggestionEngine : public QObject
{
    Q_OBJECT

public:
    // Supported formations and their slot counts: {GK, DEF, MID, FWD}
    struct FormationSlots {
        QString name;
        int gk  = 1;
        int def = 0;
        int mid = 0;
        int fwd = 0;
    };

    static QVector<FormationSlots> supportedFormations();

    explicit LineupSuggestionEngine(QObject *parent = nullptr);

    // ── Scoring ───────────────────────────────────────────────────────────────
    double scorePlayer(const Models::Player &player, Models::Position role) const;

    // ── Suggestion ───────────────────────────────────────────────────────────
    // Returns exactly 11 SuggestedPlayer entries ordered GK → DEF → MID → FWD.
    QVector<Models::SuggestedPlayer> suggestLineup(
        const QString               &formation,
        const QVector<Models::Player> &squad) const;

    // Suggest for all formations and return a map formation → total squad score
    QMap<QString, double> compareFormations(
        const QVector<Models::Player> &squad) const;

private:
    static FormationSlots parseFormation(const QString &name);

    QVector<Models::SuggestedPlayer> pickSlots(
        const QString              &slotPrefix,
        int                         count,
        Models::Position            role,
        const QVector<Models::Player> &eligible,
        QVector<int>               &usedIds) const;
};
