#pragma once

#include <QMainWindow>
#include <QVector>
#include <QMap>
#include "api/ApiModels.h"

class ApiClient;
class DatabaseManager;
class LineupSuggestionEngine;
class TacticalPitchView;
class PlayerRosterWidget;
class SuggestionPanel;
class QListWidget;
class QListWidgetItem;
class QComboBox;
class QStatusBar;
class QProgressBar;
class QLabel;
class QPushButton;
class QDockWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    Q_PROPERTY(QString formation READ formation WRITE setFormation NOTIFY formationChanged)

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    QString formation() const { return m_formation; }
    void    setFormation(const QString &f);

signals:
    void formationChanged(const QString &formation);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent (QCloseEvent  *event) override;

private:
    // ── UI setup ──────────────────────────────────────────────────────────────
    void setupToolbar();
    void setupSidebar();
    void setupCentralWidget();
    void setupDocks();
    void setupStatusBar();

    // ── Theme ─────────────────────────────────────────────────────────────────
    void applyTheme(const QString &theme);

    // ── Slots ─────────────────────────────────────────────────────────────────
    void onTeamSelected      (QListWidgetItem *item);
    void onFormationChanged  (const QString   &formation);
    void onSuggestLineup     ();
    void onApplySuggestion   (const QVector<Models::SuggestedPlayer> &suggestions);
    void onCompareFormations ();
    void onExportPng         ();
    void onOpenSettings      ();
    void onLoadingChanged    (bool loading);
    void onApiError          (const QString &error);
    void onTeamsReceived     (const QVector<Models::Team>   &teams);
    void onPlayersReceived   (int teamId, const QVector<Models::Player> &players);
    void onPlayerDroppedOnPitch(int playerId, QPointF scenePos);

    // ── Data ──────────────────────────────────────────────────────────────────
    DatabaseManager        *m_db;
    ApiClient              *m_apiClient;
    LineupSuggestionEngine *m_engine;

    // ── UI components ─────────────────────────────────────────────────────────
    TacticalPitchView  *m_pitchView;
    PlayerRosterWidget *m_rosterWidget;
    SuggestionPanel    *m_suggestionPanel;
    QListWidget        *m_teamList;
    QComboBox          *m_formationCombo;
    QProgressBar       *m_progressBar;
    QLabel             *m_statusLabel;
    QPushButton        *m_suggestBtn;
    QPushButton        *m_exportBtn;
    QPushButton        *m_settingsBtn;

    // ── State ─────────────────────────────────────────────────────────────────
    QString                  m_formation;
    QVector<Models::Team>    m_teams;
    int                      m_currentTeamId = -1;
    QVector<Models::Player>  m_currentPlayers;
    QMap<int, Models::Team>  m_teamIndex;       // id → Team

    void saveWindowState();
    void restoreWindowState();
};
