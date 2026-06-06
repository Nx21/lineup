#include "api/ApiClient.h"
#include "db/DatabaseManager.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>

// ── Constants ────────────────────────────────────────────────────────────────

const QString ApiClient::BASE_URL  = QStringLiteral("https://v3.football.api-sports.io");
const int     ApiClient::SEASON    = 2026;
const int     ApiClient::LEAGUE_ID = 1;   // FIFA World Cup

// ── Constructor ──────────────────────────────────────────────────────────────

ApiClient::ApiClient(DatabaseManager *db, QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_db(db)
{}

// ── Configuration ────────────────────────────────────────────────────────────

void ApiClient::setApiKey(const QString &key)
{
    m_apiKey = key;
}

// ── Public API methods ───────────────────────────────────────────────────────

void ApiClient::fetchTeams()
{
    // Try cache first
    bool expired = false;
    const QVector<Models::Team> cached = m_db->getCachedTeams(&expired);
    if (!cached.isEmpty() && !expired) {
        emit teamsReceived(cached);
        return;
    }

    if (m_apiKey.isEmpty()) {
        setError(QStringLiteral("API key not set. Configure it in Settings."));
        // Still emit cached data (possibly stale) so the UI is not empty
        if (!cached.isEmpty()) emit teamsReceived(cached);
        return;
    }

    QUrl url(BASE_URL + QStringLiteral("/teams"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("league"), QString::number(LEAGUE_ID));
    q.addQueryItem(QStringLiteral("season"), QString::number(SEASON));
    url.setQuery(q);

    auto *reply = m_nam->get(buildRequest(url.toString()));
    incrementPending();

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        decrementPending();

        if (reply->error() != QNetworkReply::NoError) {
            setError(reply->errorString());
            return;
        }

        const QByteArray data = reply->readAll();
        const QVector<Models::Team> teams = parseTeams(data);
        if (!teams.isEmpty()) {
            m_db->cacheTeams(teams);
            emit teamsReceived(teams);
        } else {
            setError(QStringLiteral("Failed to parse teams response."));
        }
    });
}

void ApiClient::fetchPlayers(int teamId)
{
    bool expired = false;
    const QVector<Models::Player> cached = m_db->getCachedPlayers(teamId, &expired);
    if (!cached.isEmpty() && !expired) {
        emit playersReceived(teamId, cached);
        return;
    }

    if (m_apiKey.isEmpty()) {
        setError(QStringLiteral("API key not set. Configure it in Settings."));
        if (!cached.isEmpty()) emit playersReceived(teamId, cached);
        return;
    }

    QUrl url(BASE_URL + QStringLiteral("/players"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("team"),   QString::number(teamId));
    q.addQueryItem(QStringLiteral("season"), QString::number(SEASON));
    url.setQuery(q);

    auto *reply = m_nam->get(buildRequest(url.toString()));
    incrementPending();

    connect(reply, &QNetworkReply::finished, this, [this, reply, teamId]() {
        reply->deleteLater();
        decrementPending();

        if (reply->error() != QNetworkReply::NoError) {
            setError(reply->errorString());
            return;
        }

        const QByteArray data = reply->readAll();
        const QVector<Models::Player> players = parsePlayers(data, teamId);
        if (!players.isEmpty()) {
            m_db->cachePlayers(teamId, players);
            emit playersReceived(teamId, players);
        } else {
            setError(QString("Failed to parse players for team %1.").arg(teamId));
        }
    });
}

void ApiClient::fetchFixtures()
{
    bool expired = false;
    const QVector<Models::Fixture> cached = m_db->getCachedFixtures(&expired);
    if (!cached.isEmpty() && !expired) {
        emit fixturesReceived(cached);
        return;
    }

    if (m_apiKey.isEmpty()) {
        setError(QStringLiteral("API key not set. Configure it in Settings."));
        if (!cached.isEmpty()) emit fixturesReceived(cached);
        return;
    }

    QUrl url(BASE_URL + QStringLiteral("/fixtures"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("league"), QString::number(LEAGUE_ID));
    q.addQueryItem(QStringLiteral("season"), QString::number(SEASON));
    url.setQuery(q);

    auto *reply = m_nam->get(buildRequest(url.toString()));
    incrementPending();

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        decrementPending();

        if (reply->error() != QNetworkReply::NoError) {
            setError(reply->errorString());
            return;
        }

        const QByteArray data = reply->readAll();
        const QVector<Models::Fixture> fixtures = parseFixtures(data);
        if (!fixtures.isEmpty()) {
            m_db->cacheFixtures(fixtures);
            emit fixturesReceived(fixtures);
        } else {
            setError(QStringLiteral("Failed to parse fixtures response."));
        }
    });
}

// ── Private helpers ──────────────────────────────────────────────────────────

QNetworkRequest ApiClient::buildRequest(const QString &url) const
{
    QNetworkRequest req{QUrl(url)};
    req.setRawHeader("x-apisports-key", m_apiKey.toUtf8());
    req.setRawHeader("Accept", "application/json");
    return req;
}

void ApiClient::incrementPending()
{
    if (++m_pendingRequests == 1)
        emit loadingChanged(true);
}

void ApiClient::decrementPending()
{
    if (m_pendingRequests > 0 && --m_pendingRequests == 0)
        emit loadingChanged(false);
}

void ApiClient::setError(const QString &error)
{
    m_errorMessage = error;
    emit errorMessageChanged(error);
    emit requestError(error);
}

// ── JSON Parsers ─────────────────────────────────────────────────────────────

QVector<Models::Team> ApiClient::parseTeams(const QByteArray &data) const
{
    QVector<Models::Team> teams;
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) return teams;

    const QJsonArray response = doc.object().value(QStringLiteral("response")).toArray();
    teams.reserve(response.size());

    for (const QJsonValue &v : response) {
        const QJsonObject obj  = v.toObject();
        const QJsonObject team = obj.value(QStringLiteral("team")).toObject();

        Models::Team t;
        t.id        = team.value(QStringLiteral("id")).toInt();
        t.name      = team.value(QStringLiteral("name")).toString();
        t.shortName = team.value(QStringLiteral("code")).toString();
        t.country   = team.value(QStringLiteral("country")).toString();
        t.logoUrl   = team.value(QStringLiteral("logo")).toString();
        teams.append(t);
    }
    return teams;
}

QVector<Models::Player> ApiClient::parsePlayers(const QByteArray &data, int teamId) const
{
    QVector<Models::Player> players;
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) return players;

    const QJsonArray response = doc.object().value(QStringLiteral("response")).toArray();
    players.reserve(response.size());

    for (const QJsonValue &v : response) {
        const QJsonObject obj        = v.toObject();
        const QJsonObject playerObj  = obj.value(QStringLiteral("player")).toObject();
        const QJsonArray  statistics = obj.value(QStringLiteral("statistics")).toArray();

        Models::Player p;
        p.id          = playerObj.value(QStringLiteral("id")).toInt();
        p.teamId      = teamId;
        p.name        = playerObj.value(QStringLiteral("name")).toString();
        p.firstname   = playerObj.value(QStringLiteral("firstname")).toString();
        p.lastname    = playerObj.value(QStringLiteral("lastname")).toString();
        p.age         = playerObj.value(QStringLiteral("age")).toInt();
        p.nationality = playerObj.value(QStringLiteral("nationality")).toString();
        p.photoUrl    = playerObj.value(QStringLiteral("photo")).toString();

        if (!statistics.isEmpty()) {
            const QJsonObject stat   = statistics.first().toObject();
            const QJsonObject games  = stat.value(QStringLiteral("games")).toObject();
            const QJsonObject goals  = stat.value(QStringLiteral("goals")).toObject();
            const QJsonObject passes = stat.value(QStringLiteral("passes")).toObject();
            const QJsonObject dribbles = stat.value(QStringLiteral("dribbles")).toObject();
            const QJsonObject tackles  = stat.value(QStringLiteral("tackles")).toObject();
            const QJsonObject duels    = stat.value(QStringLiteral("duels")).toObject();
            const QJsonObject shots    = stat.value(QStringLiteral("shots")).toObject();
            // Position from API (games.position)
            p.position = Models::positionFromString(
                games.value(QStringLiteral("position")).toString());
            p.jerseyNumber = games.value(QStringLiteral("number")).toInt();

            Models::PlayerStats &s = p.stats;
            s.appearances   = games.value(QStringLiteral("appearences")).toInt();
            s.minutesPlayed = games.value(QStringLiteral("minutes")).toInt();
            const QJsonValue ratingVal = games.value(QStringLiteral("rating"));
            s.rating        = ratingVal.isNull() ? 0.0 : ratingVal.toString("0").toDouble();

            s.goals         = goals.value(QStringLiteral("total")).toInt();
            s.assists       = goals.value(QStringLiteral("assists")).toInt();

            s.shots         = shots.value(QStringLiteral("total")).toInt();
            s.shotsOnTarget = shots.value(QStringLiteral("on")).toInt();
            s.shotsOnTargetPct = (s.shots > 0)
                ? 100.0 * s.shotsOnTarget / s.shots : 0.0;

            s.passes        = passes.value(QStringLiteral("total")).toInt();
            s.keyPasses     = passes.value(QStringLiteral("key")).toInt();
            const QJsonValue accVal = passes.value(QStringLiteral("accuracy"));
            s.passAccuracy  = accVal.isNull() ? 0.0 : accVal.toString("0").toDouble();

            s.dribblesAttempted = dribbles.value(QStringLiteral("attempts")).toInt();
            s.dribblesSuccess   = dribbles.value(QStringLiteral("success")).toInt();
            s.dribbleSuccessPct = (s.dribblesAttempted > 0)
                ? 100.0 * s.dribblesSuccess / s.dribblesAttempted : 0.0;

            s.tackles       = tackles.value(QStringLiteral("total")).toInt();
            s.interceptions = tackles.value(QStringLiteral("interceptions")).toInt();

            s.aerialDuelsWon   = duels.value(QStringLiteral("won")).toInt();
            s.aerialDuelsTotal = duels.value(QStringLiteral("total")).toInt();
            s.aerialDuelsWonPct = (s.aerialDuelsTotal > 0)
                ? 100.0 * s.aerialDuelsWon / s.aerialDuelsTotal : 0.0;

            // Goalkeeper (API-Football v3: saves/conceded live inside "goals" object for GKs)
            s.saves         = goals.value(QStringLiteral("saves")).toInt();
            s.goalsConceded = goals.value(QStringLiteral("conceded")).toInt();
            s.cleanSheets   = 0; // not provided per-player in free tier
            s.savesPercentage = (s.saves + s.goalsConceded > 0)
                ? 100.0 * s.saves / (s.saves + s.goalsConceded) : 0.0;
        }

        players.append(p);
    }
    return players;
}

QVector<Models::Fixture> ApiClient::parseFixtures(const QByteArray &data) const
{
    QVector<Models::Fixture> fixtures;
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) return fixtures;

    const QJsonArray response = doc.object().value(QStringLiteral("response")).toArray();
    fixtures.reserve(response.size());

    for (const QJsonValue &v : response) {
        const QJsonObject obj     = v.toObject();
        const QJsonObject fixture = obj.value(QStringLiteral("fixture")).toObject();
        const QJsonObject league  = obj.value(QStringLiteral("league")).toObject();
        const QJsonObject teams   = obj.value(QStringLiteral("teams")).toObject();
        const QJsonObject goals   = obj.value(QStringLiteral("goals")).toObject();

        Models::Fixture f;
        f.id           = fixture.value(QStringLiteral("id")).toInt();
        f.date         = QDateTime::fromString(
                             fixture.value(QStringLiteral("date")).toString(),
                             Qt::ISODate);
        f.status       = fixture.value(QStringLiteral("status"))
                                 .toObject()
                                 .value(QStringLiteral("short")).toString();
        f.round        = league.value(QStringLiteral("round")).toString();
        f.leagueId     = league.value(QStringLiteral("id")).toInt();

        const QJsonObject home = teams.value(QStringLiteral("home")).toObject();
        const QJsonObject away = teams.value(QStringLiteral("away")).toObject();
        f.homeTeamId   = home.value(QStringLiteral("id")).toInt();
        f.homeTeamName = home.value(QStringLiteral("name")).toString();
        f.awayTeamId   = away.value(QStringLiteral("id")).toInt();
        f.awayTeamName = away.value(QStringLiteral("name")).toString();

        const QJsonValue hg = goals.value(QStringLiteral("home"));
        const QJsonValue ag = goals.value(QStringLiteral("away"));
        f.homeScore    = hg.isNull() ? -1 : hg.toInt();
        f.awayScore    = ag.isNull() ? -1 : ag.toInt();

        fixtures.append(f);
    }
    return fixtures;
}
