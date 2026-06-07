#include "SquadLoader.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

// ────────────────────────────────────────────────────────────────────────────

Models::PlayerStats SquadLoader::defaultStats() const
{
    Models::PlayerStats s;
    s.goals              = 0;
    s.assists            = 0;
    s.appearances        = 0;
    s.passAccuracy       = 75.0;
    s.keyPasses          = 0;
    s.dribblesSuccess    = 0;
    s.tackles            = 0;
    s.interceptions      = 0;
    s.aerialDuelsWon     = 0;
    s.saves              = 0;
    s.cleanSheets        = 0;
    s.savesPercentage    = 0.0;
    s.shotsOnTargetPct   = 0.0;
    s.rating             = 6.5;
    return s;
}

QString SquadLoader::positionFromCode(const QString &code) const
{
    if (code == "GK") return "Goalkeeper";
    if (code == "DF") return "Defender";
    if (code == "MF") return "Midfielder";
    if (code == "FW") return "Forward";
    return "Midfielder";
}

// ────────────────────────────────────────────────────────────────────────────

Models::Team SquadLoader::loadTeam(const QString &resourcePath) const
{
    Models::Team team;

    QFile f(resourcePath);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "SquadLoader: cannot open" << resourcePath;
        return team;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "SquadLoader: JSON parse error in" << resourcePath << err.errorString();
        return team;
    }

    QJsonObject root = doc.object();
    team.id          = root["id"].toInt();
    team.name        = root["name"].toString();
    team.country     = root["country"].toString();
    team.logoUrl     = "";

    QJsonArray players = root["players"].toArray();
    for (const QJsonValue &pv : players) {
        QJsonObject po = pv.toObject();

        Models::Player player;
        player.id          = po["id"].toInt();
        player.teamId      = team.id;
        player.name        = po["name"].toString();

        // Split name into first/last
        QStringList parts  = player.name.split(' ');
        player.firstname   = parts.isEmpty() ? "" : parts.first();
        player.lastname    = parts.size() > 1 ? parts.last() : "";

        player.age         = po["age"].toInt();
        player.nationality = team.country;
        player.position    = Models::positionFromString(positionFromCode(po["position"].toString()));
        player.jerseyNumber = po["jersey"].toInt();

        // Map caps/goals into stats as a useful proxy
        player.stats                  = defaultStats();
        player.stats.goals            = po["goals"].toInt();
        player.stats.appearances      = po["caps"].toInt();

        // Give forwards a slight shot accuracy bump, GKs a save% estimate
        QString pos = po["position"].toString();
        if (pos == "FW") {
            player.stats.shotsOnTargetPct = 40.0 + (player.stats.goals * 2.0);
            player.stats.dribblesSuccess  = 30 + player.stats.goals;
        } else if (pos == "GK") {
            player.stats.savesPercentage  = 68.0;
            player.stats.cleanSheets   = player.stats.appearances / 5;
        } else if (pos == "MF") {
            player.stats.keyPasses     = player.stats.appearances / 3;
            player.stats.passAccuracy  = 80.0;
        } else if (pos == "DF") {
            player.stats.tackles       = player.stats.appearances / 2;
            player.stats.interceptions = player.stats.appearances / 3;
            player.stats.aerialDuelsWon = player.stats.appearances / 4;
        }

        // Rating: rough proxy
        player.stats.rating = 6.5 + (player.stats.goals * 0.05)
                              + (player.stats.appearances * 0.01);
        player.stats.rating = std::min(player.stats.rating, 10.0);

        team.players.append(player);
    }

    return team;
}

// ────────────────────────────────────────────────────────────────────────────

QVector<Models::Team> SquadLoader::loadAll() const
{
    QVector<Models::Team> teams;

    // Read master index
    QFile indexFile(":/squads/teams_index.json");
    if (!indexFile.open(QIODevice::ReadOnly)) {
        qWarning() << "SquadLoader: cannot open :/squads/teams_index.json";
        return teams;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(indexFile.readAll(), &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "SquadLoader: index JSON error" << err.errorString();
        return teams;
    }

    QJsonArray index = doc.object()["teams"].toArray();
    teams.reserve(index.size());

    for (const QJsonValue &v : index) {
        QJsonObject entry = v.toObject();
        QString file      = entry["file"].toString();
        QString path      = ":/squads/" + file;
        Models::Team team = loadTeam(path);
        if (team.id > 0) {
            teams.append(team);
        }
    }

    return teams;
}
