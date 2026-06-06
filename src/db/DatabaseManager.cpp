#include "db/DatabaseManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QVariant>

int DatabaseManager::s_counter = 0;

// ── Constructor / Destructor ─────────────────────────────────────────────────

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{
    m_connectionName = QString("WorldCupDB_%1").arg(++s_counter);
}

DatabaseManager::~DatabaseManager()
{
    // Must destroy the QSqlDatabase member before calling removeDatabase,
    // otherwise Qt warns "connection still in use".
    m_db.close();
    m_db = QSqlDatabase(); // reset to invalid/default — releases the handle
    QSqlDatabase::removeDatabase(m_connectionName);
}

// ── Initialization ───────────────────────────────────────────────────────────

bool DatabaseManager::initialize(const QString &dbPath)
{
    QString path = dbPath;
    if (path.isEmpty()) {
        const QString dir = QStandardPaths::writableLocation(
                                QStandardPaths::AppDataLocation);
        QDir().mkpath(dir);
        path = dir + QStringLiteral("/worldcup_cache.db");
    }

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_db.setDatabaseName(path);

    if (!m_db.open()) {
        qWarning() << "DatabaseManager: failed to open database:" << m_db.lastError().text();
        return false;
    }

    // Enable WAL for better concurrent access
    QSqlQuery q(m_db);
    q.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    q.exec(QStringLiteral("PRAGMA foreign_keys=ON"));

    return createTables();
}

bool DatabaseManager::isValid() const
{
    return m_db.isOpen();
}

// ── Schema ───────────────────────────────────────────────────────────────────

bool DatabaseManager::createTables()
{
    QSqlQuery q(m_db);

    const QStringList ddl{
        R"(CREATE TABLE IF NOT EXISTS teams (
            id          INTEGER PRIMARY KEY,
            name        TEXT NOT NULL,
            short_name  TEXT,
            country     TEXT,
            logo_url    TEXT
        ))",

        R"(CREATE TABLE IF NOT EXISTS players (
            id                  INTEGER PRIMARY KEY,
            team_id             INTEGER NOT NULL,
            name                TEXT NOT NULL,
            firstname           TEXT,
            lastname            TEXT,
            age                 INTEGER,
            nationality         TEXT,
            position            TEXT,
            jersey_number       INTEGER,
            photo_url           TEXT,
            appearances         INTEGER DEFAULT 0,
            minutes_played      INTEGER DEFAULT 0,
            rating              REAL    DEFAULT 0,
            goals               INTEGER DEFAULT 0,
            assists             INTEGER DEFAULT 0,
            shots               INTEGER DEFAULT 0,
            shots_on_target     INTEGER DEFAULT 0,
            shots_on_target_pct REAL    DEFAULT 0,
            passes              INTEGER DEFAULT 0,
            key_passes          INTEGER DEFAULT 0,
            pass_accuracy       REAL    DEFAULT 0,
            dribbles_attempted  INTEGER DEFAULT 0,
            dribbles_success    INTEGER DEFAULT 0,
            dribble_success_pct REAL    DEFAULT 0,
            tackles             INTEGER DEFAULT 0,
            interceptions       INTEGER DEFAULT 0,
            aerial_duels_won    INTEGER DEFAULT 0,
            aerial_duels_total  INTEGER DEFAULT 0,
            aerial_duels_won_pct REAL   DEFAULT 0,
            saves               INTEGER DEFAULT 0,
            goals_conceded      INTEGER DEFAULT 0,
            saves_pct           REAL    DEFAULT 0,
            clean_sheets        INTEGER DEFAULT 0
        ))",

        R"(CREATE TABLE IF NOT EXISTS fixtures (
            id              INTEGER PRIMARY KEY,
            date            TEXT,
            home_team_id    INTEGER,
            away_team_id    INTEGER,
            home_team_name  TEXT,
            away_team_name  TEXT,
            home_score      INTEGER DEFAULT -1,
            away_score      INTEGER DEFAULT -1,
            status          TEXT,
            round           TEXT,
            league_id       INTEGER,
            season          INTEGER
        ))",

        R"(CREATE TABLE IF NOT EXISTS cache_meta (
            entity      TEXT PRIMARY KEY,
            fetched_at  TEXT NOT NULL
        ))"
    };

    for (const QString &stmt : ddl) {
        if (!q.exec(stmt)) {
            qWarning() << "DatabaseManager: DDL failed:" << q.lastError().text();
            return false;
        }
    }
    return true;
}

// ── TTL helpers ──────────────────────────────────────────────────────────────

bool DatabaseManager::isExpired(const QString &entity) const
{
    const QDateTime last = lastFetchTime(entity);
    if (!last.isValid()) return true;
    return last.secsTo(QDateTime::currentDateTimeUtc()) > CACHE_TTL_HOURS * 3600;
}

void DatabaseManager::updateFetchTime(const QString &entity)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO cache_meta (entity, fetched_at) VALUES (?, ?)"));
    q.addBindValue(entity);
    q.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    q.exec();
}

QDateTime DatabaseManager::lastFetchTime(const QString &entity) const
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT fetched_at FROM cache_meta WHERE entity = ?"));
    q.addBindValue(entity);
    if (q.exec() && q.next()) {
        return QDateTime::fromString(q.value(0).toString(), Qt::ISODate);
    }
    return {};
}

// ── Teams ────────────────────────────────────────────────────────────────────

bool DatabaseManager::cacheTeams(const QVector<Models::Team> &teams)
{
    if (!m_db.transaction()) return false;

    QSqlQuery del(m_db);
    del.exec(QStringLiteral("DELETE FROM teams"));

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO teams (id, name, short_name, country, logo_url) "
        "VALUES (?, ?, ?, ?, ?)"));

    for (const Models::Team &t : teams) {
        q.addBindValue(t.id);
        q.addBindValue(t.name);
        q.addBindValue(t.shortName);
        q.addBindValue(t.country);
        q.addBindValue(t.logoUrl);
        if (!q.exec()) {
            qWarning() << "cacheTeams insert failed:" << q.lastError().text();
            m_db.rollback();
            return false;
        }
    }

    if (!m_db.commit()) { m_db.rollback(); return false; }
    updateFetchTime(QStringLiteral("teams"));
    return true;
}

QVector<Models::Team> DatabaseManager::getCachedTeams(bool *expired) const
{
    if (expired) *expired = isExpired(QStringLiteral("teams"));

    QVector<Models::Team> teams;
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("SELECT id, name, short_name, country, logo_url FROM teams")))
        return teams;

    while (q.next()) {
        Models::Team t;
        t.id        = q.value(0).toInt();
        t.name      = q.value(1).toString();
        t.shortName = q.value(2).toString();
        t.country   = q.value(3).toString();
        t.logoUrl   = q.value(4).toString();
        teams.append(t);
    }
    return teams;
}

// ── Players ──────────────────────────────────────────────────────────────────

bool DatabaseManager::cachePlayers(int teamId, const QVector<Models::Player> &players)
{
    if (!m_db.transaction()) return false;

    QSqlQuery del(m_db);
    del.prepare(QStringLiteral("DELETE FROM players WHERE team_id = ?"));
    del.addBindValue(teamId);
    del.exec();

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO players ("
        " id, team_id, name, firstname, lastname, age, nationality, position,"
        " jersey_number, photo_url, appearances, minutes_played, rating,"
        " goals, assists, shots, shots_on_target, shots_on_target_pct,"
        " passes, key_passes, pass_accuracy,"
        " dribbles_attempted, dribbles_success, dribble_success_pct,"
        " tackles, interceptions,"
        " aerial_duels_won, aerial_duels_total, aerial_duels_won_pct,"
        " saves, goals_conceded, saves_pct, clean_sheets"
        ") VALUES ("
        " ?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?"
        ")"));

    for (const Models::Player &p : players) {
        const Models::PlayerStats &s = p.stats;
        q.addBindValue(p.id);
        q.addBindValue(p.teamId);
        q.addBindValue(p.name);
        q.addBindValue(p.firstname);
        q.addBindValue(p.lastname);
        q.addBindValue(p.age);
        q.addBindValue(p.nationality);
        q.addBindValue(Models::positionFullName(p.position));
        q.addBindValue(p.jerseyNumber);
        q.addBindValue(p.photoUrl);
        q.addBindValue(s.appearances);
        q.addBindValue(s.minutesPlayed);
        q.addBindValue(s.rating);
        q.addBindValue(s.goals);
        q.addBindValue(s.assists);
        q.addBindValue(s.shots);
        q.addBindValue(s.shotsOnTarget);
        q.addBindValue(s.shotsOnTargetPct);
        q.addBindValue(s.passes);
        q.addBindValue(s.keyPasses);
        q.addBindValue(s.passAccuracy);
        q.addBindValue(s.dribblesAttempted);
        q.addBindValue(s.dribblesSuccess);
        q.addBindValue(s.dribbleSuccessPct);
        q.addBindValue(s.tackles);
        q.addBindValue(s.interceptions);
        q.addBindValue(s.aerialDuelsWon);
        q.addBindValue(s.aerialDuelsTotal);
        q.addBindValue(s.aerialDuelsWonPct);
        q.addBindValue(s.saves);
        q.addBindValue(s.goalsConceded);
        q.addBindValue(s.savesPercentage);
        q.addBindValue(s.cleanSheets);

        if (!q.exec()) {
            qWarning() << "cachePlayers insert failed:" << q.lastError().text();
            m_db.rollback();
            return false;
        }
    }

    if (!m_db.commit()) { m_db.rollback(); return false; }
    updateFetchTime(QString("players_%1").arg(teamId));
    return true;
}

QVector<Models::Player> DatabaseManager::getCachedPlayers(int teamId, bool *expired) const
{
    if (expired) *expired = isExpired(QString("players_%1").arg(teamId));

    QVector<Models::Player> players;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT id, team_id, name, firstname, lastname, age, nationality, position,"
        " jersey_number, photo_url, appearances, minutes_played, rating,"
        " goals, assists, shots, shots_on_target, shots_on_target_pct,"
        " passes, key_passes, pass_accuracy,"
        " dribbles_attempted, dribbles_success, dribble_success_pct,"
        " tackles, interceptions,"
        " aerial_duels_won, aerial_duels_total, aerial_duels_won_pct,"
        " saves, goals_conceded, saves_pct, clean_sheets"
        " FROM players WHERE team_id = ?"));
    q.addBindValue(teamId);
    if (!q.exec()) return players;

    while (q.next()) {
        Models::Player p;
        int col = 0;
        p.id            = q.value(col++).toInt();
        p.teamId        = q.value(col++).toInt();
        p.name          = q.value(col++).toString();
        p.firstname     = q.value(col++).toString();
        p.lastname      = q.value(col++).toString();
        p.age           = q.value(col++).toInt();
        p.nationality   = q.value(col++).toString();
        p.position      = Models::positionFromString(q.value(col++).toString());
        p.jerseyNumber  = q.value(col++).toInt();
        p.photoUrl      = q.value(col++).toString();

        Models::PlayerStats &s = p.stats;
        s.appearances        = q.value(col++).toInt();
        s.minutesPlayed      = q.value(col++).toInt();
        s.rating             = q.value(col++).toDouble();
        s.goals              = q.value(col++).toInt();
        s.assists            = q.value(col++).toInt();
        s.shots              = q.value(col++).toInt();
        s.shotsOnTarget      = q.value(col++).toInt();
        s.shotsOnTargetPct   = q.value(col++).toDouble();
        s.passes             = q.value(col++).toInt();
        s.keyPasses          = q.value(col++).toInt();
        s.passAccuracy       = q.value(col++).toDouble();
        s.dribblesAttempted  = q.value(col++).toInt();
        s.dribblesSuccess    = q.value(col++).toInt();
        s.dribbleSuccessPct  = q.value(col++).toDouble();
        s.tackles            = q.value(col++).toInt();
        s.interceptions      = q.value(col++).toInt();
        s.aerialDuelsWon     = q.value(col++).toInt();
        s.aerialDuelsTotal   = q.value(col++).toInt();
        s.aerialDuelsWonPct  = q.value(col++).toDouble();
        s.saves              = q.value(col++).toInt();
        s.goalsConceded      = q.value(col++).toInt();
        s.savesPercentage    = q.value(col++).toDouble();
        s.cleanSheets        = q.value(col++).toInt();

        players.append(p);
    }
    return players;
}

// ── Fixtures ─────────────────────────────────────────────────────────────────

bool DatabaseManager::cacheFixtures(const QVector<Models::Fixture> &fixtures)
{
    if (!m_db.transaction()) return false;

    QSqlQuery del(m_db);
    del.exec(QStringLiteral("DELETE FROM fixtures"));

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO fixtures "
        "(id, date, home_team_id, away_team_id, home_team_name, away_team_name,"
        " home_score, away_score, status, round, league_id, season) "
        "VALUES (?,?,?,?,?,?,?,?,?,?,?,?)"));

    for (const Models::Fixture &f : fixtures) {
        q.addBindValue(f.id);
        q.addBindValue(f.date.toString(Qt::ISODate));
        q.addBindValue(f.homeTeamId);
        q.addBindValue(f.awayTeamId);
        q.addBindValue(f.homeTeamName);
        q.addBindValue(f.awayTeamName);
        q.addBindValue(f.homeScore);
        q.addBindValue(f.awayScore);
        q.addBindValue(f.status);
        q.addBindValue(f.round);
        q.addBindValue(f.leagueId);
        q.addBindValue(f.season);

        if (!q.exec()) {
            qWarning() << "cacheFixtures insert failed:" << q.lastError().text();
            m_db.rollback();
            return false;
        }
    }

    if (!m_db.commit()) { m_db.rollback(); return false; }
    updateFetchTime(QStringLiteral("fixtures"));
    return true;
}

QVector<Models::Fixture> DatabaseManager::getCachedFixtures(bool *expired) const
{
    if (expired) *expired = isExpired(QStringLiteral("fixtures"));

    QVector<Models::Fixture> fixtures;
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral(
            "SELECT id, date, home_team_id, away_team_id, home_team_name, away_team_name,"
            " home_score, away_score, status, round, league_id, season"
            " FROM fixtures")))
        return fixtures;

    while (q.next()) {
        Models::Fixture f;
        int col = 0;
        f.id           = q.value(col++).toInt();
        f.date         = QDateTime::fromString(q.value(col++).toString(), Qt::ISODate);
        f.homeTeamId   = q.value(col++).toInt();
        f.awayTeamId   = q.value(col++).toInt();
        f.homeTeamName = q.value(col++).toString();
        f.awayTeamName = q.value(col++).toString();
        f.homeScore    = q.value(col++).toInt();
        f.awayScore    = q.value(col++).toInt();
        f.status       = q.value(col++).toString();
        f.round        = q.value(col++).toString();
        f.leagueId     = q.value(col++).toInt();
        f.season       = q.value(col++).toInt();
        fixtures.append(f);
    }
    return fixtures;
}

// ── Cache management ─────────────────────────────────────────────────────────

void DatabaseManager::clearCache()
{
    QSqlQuery q(m_db);
    q.exec(QStringLiteral("DELETE FROM teams"));
    q.exec(QStringLiteral("DELETE FROM players"));
    q.exec(QStringLiteral("DELETE FROM fixtures"));
    q.exec(QStringLiteral("DELETE FROM cache_meta"));
}
