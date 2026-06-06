#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QVector>
#include <QDateTime>
#include "api/ApiModels.h"

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager() override;

    bool initialize(const QString &dbPath = QString());
    bool isValid() const;

    // ── Teams ────────────────────────────────────────────────────────────────
    bool                   cacheTeams      (const QVector<Models::Team>    &teams);
    QVector<Models::Team>  getCachedTeams  (bool *expired = nullptr) const;

    // ── Players ──────────────────────────────────────────────────────────────
    bool                    cachePlayers     (int teamId, const QVector<Models::Player> &players);
    QVector<Models::Player> getCachedPlayers (int teamId, bool *expired = nullptr) const;

    // ── Fixtures ─────────────────────────────────────────────────────────────
    bool                      cacheFixtures     (const QVector<Models::Fixture> &fixtures);
    QVector<Models::Fixture>  getCachedFixtures (bool *expired = nullptr) const;

    // ── Cache management ─────────────────────────────────────────────────────
    void      clearCache();
    QDateTime lastFetchTime(const QString &entity) const;

    static constexpr int CACHE_TTL_HOURS = 24;

private:
    bool createTables();
    bool isExpired(const QString &entity) const;
    void updateFetchTime(const QString &entity);

    QSqlDatabase m_db;
    QString      m_connectionName;

    static int s_counter;
};
