#pragma once

#include <QMainWindow>
#include <QVector>
#include <QMap>
#include <QColor>
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
class QTabWidget;

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
    void onTeamSelected          (QListWidgetItem *item);
    void onHomeFormationChanged  (const QString   &formation);
    void onAwayFormationChanged  (const QString   &formation);
    void onSuggestLineup         ();
    void onApplySuggestion       (const QVector<Models::SuggestedPlayer> &suggestions);
    void onCompareFormations     ();
    void onExportPng             ();
    void onOpenSettings          ();
    void onLoadingChanged        (bool loading);
    void onApiError              (const QString &error);
    void onTeamsReceived         (const QVector<Models::Team>   &teams);
    void onPlayersReceived       (int teamId, const QVector<Models::Player> &players);
    void onPlayerDroppedOnPitch  (int playerId, QPointF scenePos);
    void onPlayerRemovedFromPitch(int playerId);
    void onHomeColorPick         ();
    void onAwayColorPick         ();
    void updateSlotButtonStyles  ();

    // ── Data ──────────────────────────────────────────────────────────────────
    DatabaseManager        *m_db;
    ApiClient              *m_apiClient;
    LineupSuggestionEngine *m_engine;

    // ── UI components ─────────────────────────────────────────────────────────
    TacticalPitchView  *m_pitchView;
    PlayerRosterWidget *m_rosterWidget;      // home roster
    PlayerRosterWidget *m_awayRosterWidget;  // away roster
    SuggestionPanel    *m_suggestionPanel;
    QListWidget        *m_teamList;
    QComboBox          *m_formationCombo;    // home formation
    QComboBox          *m_awayFormationCombo; // away formation
    QProgressBar       *m_progressBar;
    QLabel             *m_statusLabel;
    QLabel             *m_homeTeamLabel;     // shows selected home team name
    QLabel             *m_awayTeamLabel;     // shows selected away team name
    QPushButton        *m_homeSlotBtn;       // sidebar toggle: assign to Home
    QPushButton        *m_awaySlotBtn;       // sidebar toggle: assign to Away
    QPushButton        *m_homeColorBtn;      // color swatch, opens QColorDialog
    QPushButton        *m_awayColorBtn;      // color swatch, opens QColorDialog
    QPushButton        *m_suggestBtn;
    QPushButton        *m_exportBtn;
    QPushButton        *m_settingsBtn;

    // ── State ─────────────────────────────────────────────────────────────────
    QString                  m_formation;     // home
    QString                  m_awayFormation; // away
    QVector<Models::Team>    m_teams;
    bool                     m_selectingHome = true;  // sidebar toggle state
    int                      m_homeTeamId    = -1;
    int                      m_awayTeamId    = -1;
    QVector<Models::Player>  m_homeTeamPlayers;
    QVector<Models::Player>  m_awayTeamPlayers;
    QMap<int, Models::Team>  m_teamIndex;
    QColor                   m_homeColor;    // default #1565C0 (blue)
    QColor                   m_awayColor;    // default #C62828 (red)

    // track whether next playersReceived is for home or away
    bool m_loadingForHome    = true;
    // track which team the current suggestion panel results are for
    bool m_suggestionForHome = true;

    void saveWindowState();
    void restoreWindowState();
};
