#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QVector>
#include <QHash>
#include "api/ApiModels.h"

class DatabaseManager;

class ApiClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool    loading      READ isLoading      NOTIFY loadingChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage   NOTIFY errorMessageChanged)

public:
    explicit ApiClient(DatabaseManager *db, QObject *parent = nullptr);

    // ── State ────────────────────────────────────────────────────────────────
    bool    isLoading()     const { return m_pendingRequests > 0; }
    QString errorMessage()  const { return m_errorMessage; }

    // ── Configuration ────────────────────────────────────────────────────────
    void    setApiKey(const QString &key);
    QString apiKey()        const { return m_apiKey; }

    // ── Async API methods ────────────────────────────────────────────────────
    // Each method checks the SQLite cache first; only hits the network when
    // the cached entry is missing or older than 24 h.
    void fetchTeams();
    void fetchPlayers(int teamId);
    void fetchFixtures();

signals:
    void teamsReceived   (const QVector<Models::Team>    &teams);
    void playersReceived (int teamId, const QVector<Models::Player>   &players);
    void fixturesReceived(const QVector<Models::Fixture> &fixtures);

    void loadingChanged     (bool loading);
    void errorMessageChanged(const QString &message);
    void requestError       (const QString &error);

private:
    // Network helpers
    QNetworkRequest buildRequest(const QString &endpoint) const;
    void            incrementPending();
    void            decrementPending();
    void            setError(const QString &error);

    // Parsers
    QVector<Models::Team>    parseTeams   (const QByteArray &data) const;
    QVector<Models::Player>  parsePlayers (const QByteArray &data, int teamId) const;
    QVector<Models::Fixture> parseFixtures(const QByteArray &data) const;

    QNetworkAccessManager *m_nam;
    DatabaseManager       *m_db;         // non-owning
    QString                m_apiKey;
    int                    m_pendingRequests = 0;
    QString                m_errorMessage;

    static const QString BASE_URL;
    static const int     SEASON;
    static const int     LEAGUE_ID;
};
