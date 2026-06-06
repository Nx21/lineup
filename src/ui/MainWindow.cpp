#include "ui/MainWindow.h"
#include "api/ApiClient.h"
#include "api/ApiModels.h"
#include "db/DatabaseManager.h"
#include "engine/LineupSuggestionEngine.h"
#include "ui/TacticalPitchView.h"
#include "ui/PlayerRosterWidget.h"
#include "ui/SuggestionPanel.h"
#include "ui/SettingsDialog.h"

#include <QToolBar>
#include <QListWidget>
#include <QListWidgetItem>
#include <QComboBox>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QDockWidget>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStatusBar>
#include <QFileDialog>
#include <QSettings>
#include <QImageWriter>
#include <QResizeEvent>
#include <QCloseEvent>
#include <QAction>
#include <QMessageBox>
#include <algorithm>

// ── Constructor / Destructor ─────────────────────────────────────────────────

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_db      (new DatabaseManager(this))
    , m_apiClient(nullptr)
    , m_engine  (new LineupSuggestionEngine(this))
    , m_pitchView      (new TacticalPitchView(this))
    , m_rosterWidget   (new PlayerRosterWidget(this))
    , m_suggestionPanel(new SuggestionPanel(this))
    , m_teamList       (new QListWidget(this))
    , m_formationCombo (new QComboBox(this))
    , m_progressBar    (new QProgressBar(this))
    , m_statusLabel    (new QLabel(this))
    , m_suggestBtn     (new QPushButton(QStringLiteral("⚡ Suggest Lineup"), this))
    , m_exportBtn      (new QPushButton(QStringLiteral("📷 Export PNG"), this))
    , m_settingsBtn    (new QPushButton(QStringLiteral("⚙ Settings"), this))
    , m_formation      (QStringLiteral("4-4-2"))
{
    setWindowTitle(QStringLiteral("WorldCup Analyst — 2026 FIFA World Cup"));
    setMinimumSize(1200, 750);

    // Initialise database
    m_db->initialize();

    // Create ApiClient after DB is ready
    m_apiClient = new ApiClient(m_db, this);

    // Load saved API key
    QSettings cfg;
    m_apiClient->setApiKey(cfg.value(QStringLiteral("api/key")).toString());

    setupToolbar();
    setupSidebar();
    setupCentralWidget();
    setupDocks();
    setupStatusBar();

    // Apply saved theme
    applyTheme(cfg.value(QStringLiteral("ui/theme"),
                         QStringLiteral("dark")).toString());

    // Wire API signals
    connect(m_apiClient, &ApiClient::teamsReceived,
            this, &MainWindow::onTeamsReceived);
    connect(m_apiClient, &ApiClient::playersReceived,
            this, &MainWindow::onPlayersReceived);
    connect(m_apiClient, &ApiClient::loadingChanged,
            this, &MainWindow::onLoadingChanged);
    connect(m_apiClient, &ApiClient::requestError,
            this, &MainWindow::onApiError);

    // Wire suggestion panel
    connect(m_suggestionPanel, &SuggestionPanel::applySuggestionRequested,
            this, &MainWindow::onApplySuggestion);
    connect(m_suggestionPanel, &SuggestionPanel::compareFormationsRequested,
            this, &MainWindow::onCompareFormations);

    // Wire pitch drop
    connect(m_pitchView, &TacticalPitchView::playerDroppedOnPitch,
            this, &MainWindow::onPlayerDroppedOnPitch);

    restoreWindowState();

    // Kick off initial team load
    m_apiClient->fetchTeams();
}

MainWindow::~MainWindow()
{
    saveWindowState();
}

// ── UI setup ─────────────────────────────────────────────────────────────────

void MainWindow::setupToolbar()
{
    auto *tb = addToolBar(QStringLiteral("Main Toolbar"));
    tb->setMovable(false);
    tb->setIconSize(QSize(16, 16));
    tb->setStyleSheet(
        "QToolBar { background: #181825; border-bottom: 1px solid #313244; spacing: 6px; }"
        "QToolBar::separator { width: 1px; background: #45475a; margin: 4px 2px; }");

    // Formation
    auto *formLabel = new QLabel(QStringLiteral("  Formation: "), tb);
    formLabel->setStyleSheet("color: #cdd6f4; font-weight: bold;");
    tb->addWidget(formLabel);

    const QStringList formations =
        { QStringLiteral("4-4-2"), QStringLiteral("4-3-3"),
          QStringLiteral("3-5-2"), QStringLiteral("4-2-3-1"),
          QStringLiteral("5-3-2") };
    m_formationCombo->addItems(formations);
    m_formationCombo->setCurrentText(m_formation);
    m_formationCombo->setStyleSheet(
        "QComboBox { background: #313244; color: #cdd6f4; "
        "  border: 1px solid #45475a; padding: 3px 8px; border-radius: 4px; min-width: 90px; }");
    tb->addWidget(m_formationCombo);

    tb->addSeparator();

    const QString btnStyle =
        "QPushButton { background: #313244; color: #cba6f7; "
        "  border: 1px solid #45475a; border-radius: 5px; padding: 5px 12px; "
        "  font-weight: bold; }"
        "QPushButton:hover { background: #45475a; }"
        "QPushButton:pressed { background: #585b70; }";

    m_suggestBtn->setStyleSheet(btnStyle);
    m_exportBtn->setStyleSheet(btnStyle);
    m_settingsBtn->setStyleSheet(btnStyle);

    tb->addWidget(m_suggestBtn);
    tb->addWidget(m_exportBtn);
    tb->addSeparator();
    tb->addWidget(m_settingsBtn);

    connect(m_formationCombo, &QComboBox::currentTextChanged,
            this, &MainWindow::onFormationChanged);
    connect(m_suggestBtn,  &QPushButton::clicked, this, &MainWindow::onSuggestLineup);
    connect(m_exportBtn,   &QPushButton::clicked, this, &MainWindow::onExportPng);
    connect(m_settingsBtn, &QPushButton::clicked, this, &MainWindow::onOpenSettings);
}

void MainWindow::setupSidebar()
{
    auto *sideWidget = new QWidget(this);
    auto *layout     = new QVBoxLayout(sideWidget);
    layout->setContentsMargins(4, 4, 4, 4);

    auto *hdr = new QLabel(QStringLiteral("🌍  Teams"), sideWidget);
    hdr->setStyleSheet(
        "font-weight: bold; color: #cba6f7; padding: 4px; font-size: 10pt;");

    m_teamList->setStyleSheet(
        "QListWidget { background: #181825; color: #cdd6f4; border: none; }"
        "QListWidget::item { padding: 6px 8px; border-bottom: 1px solid #313244; }"
        "QListWidget::item:selected { background: #45475a; }"
        "QListWidget::item:hover { background: #313244; }");

    layout->addWidget(hdr);
    layout->addWidget(m_teamList, 1);
    sideWidget->setStyleSheet("background: #1e1e2e;");
    sideWidget->setMinimumWidth(180);
    sideWidget->setMaximumWidth(260);

    auto *dock = new QDockWidget(QStringLiteral("Teams"), this);
    dock->setWidget(sideWidget);
    dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::LeftDockWidgetArea, dock);

    connect(m_teamList, &QListWidget::itemClicked,
            this, &MainWindow::onTeamSelected);
}

void MainWindow::setupCentralWidget()
{
    // Central: pitch + roster stacked vertically on right
    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setStyleSheet("QSplitter::handle { background: #313244; width: 3px; }");

    splitter->addWidget(m_pitchView);
    splitter->addWidget(m_rosterWidget);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);

    setCentralWidget(splitter);
}

void MainWindow::setupDocks()
{
    addDockWidget(Qt::RightDockWidgetArea, m_suggestionPanel);
    m_suggestionPanel->setMinimumWidth(240);
}

void MainWindow::setupStatusBar()
{
    m_progressBar->setRange(0, 0);    // indeterminate
    m_progressBar->setVisible(false);
    m_progressBar->setMaximumWidth(120);
    m_progressBar->setMaximumHeight(14);
    m_progressBar->setStyleSheet(
        "QProgressBar { border: 1px solid #45475a; border-radius: 3px; background: #313244; }"
        "QProgressBar::chunk { background: #cba6f7; }");

    m_statusLabel->setStyleSheet("color: #a6adc8; padding: 0 6px;");

    statusBar()->setStyleSheet("QStatusBar { background: #181825; color: #a6adc8; }");
    statusBar()->addPermanentWidget(m_progressBar);
    statusBar()->addWidget(m_statusLabel);
    statusBar()->showMessage(QStringLiteral("Ready — select a team to load squad data"));
}

// ── Theme ─────────────────────────────────────────────────────────────────────

void MainWindow::applyTheme(const QString &theme)
{
    if (theme == QStringLiteral("light")) {
        qApp->setStyleSheet(
            "QMainWindow { background: #f5f5f5; }"
            "QWidget { background: #f5f5f5; color: #333333; }");
    } else {
        // Dark theme (Catppuccin Mocha palette)
        qApp->setStyleSheet(
            "QMainWindow { background: #1e1e2e; }"
            "QWidget { color: #cdd6f4; }"
            "QScrollBar:vertical { background: #181825; width: 8px; }"
            "QScrollBar::handle:vertical { background: #45475a; border-radius: 4px; }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
            "QScrollBar:horizontal { background: #181825; height: 8px; }"
            "QScrollBar::handle:horizontal { background: #45475a; border-radius: 4px; }"
            "QToolTip { background: #313244; color: #cdd6f4; border: 1px solid #45475a; }");
    }
}

// ── Formation ─────────────────────────────────────────────────────────────────

void MainWindow::setFormation(const QString &f)
{
    if (m_formation == f) return;
    m_formation = f;
    m_pitchView->setFormation(f);
    emit formationChanged(f);
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void MainWindow::onTeamsReceived(const QVector<Models::Team> &teams)
{
    m_teams = teams;
    m_teamIndex.clear();
    m_teamList->clear();

    for (const Models::Team &t : teams) {
        m_teamIndex.insert(t.id, t);
        auto *item = new QListWidgetItem(
            QString("%1  %2").arg(t.country, t.name), m_teamList);
        item->setData(Qt::UserRole, t.id);
    }

    statusBar()->showMessage(
        QString("Loaded %1 teams.").arg(teams.size()), 4000);
}

void MainWindow::onPlayersReceived(int /*teamId*/,
                                    const QVector<Models::Player> &players)
{
    // Score players before displaying
    QVector<Models::Player> scored = players;
    for (Models::Player &p : scored) {
        p.overallScore = m_engine->scorePlayer(p, p.position);
    }
    // Sort descending by score
    std::sort(scored.begin(), scored.end(),
              [](const Models::Player &a, const Models::Player &b){
                  return a.overallScore > b.overallScore;
              });

    m_currentPlayers = scored;
    m_rosterWidget->setPlayers(scored);
    statusBar()->showMessage(
        QString("Loaded %1 players.").arg(scored.size()), 4000);
}

void MainWindow::onTeamSelected(QListWidgetItem *item)
{
    if (!item) return;
    const int teamId = item->data(Qt::UserRole).toInt();
    if (teamId == m_currentTeamId) return;
    m_currentTeamId = teamId;

    m_currentPlayers.clear();
    m_rosterWidget->setPlayers({});
    m_pitchView->clearTokens();
    m_suggestionPanel->clearSuggestions();

    statusBar()->showMessage(
        QString("Loading players for %1…").arg(item->text()));
    m_apiClient->fetchPlayers(teamId);
}

void MainWindow::onFormationChanged(const QString &formation)
{
    setFormation(formation);
    // Re-apply tokens if a team is already loaded
    if (!m_currentPlayers.isEmpty()) {
        m_pitchView->setTeamPlayers(m_currentPlayers, true,
                                    QColor(0x1565C0)); // home blue
    }
}

void MainWindow::onSuggestLineup()
{
    if (m_currentPlayers.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("Please select a team first."), 3000);
        return;
    }
    const auto suggestions = m_engine->suggestLineup(m_formation, m_currentPlayers);
    m_suggestionPanel->setSuggestions(suggestions);
    statusBar()->showMessage(
        QString("Lineup suggestion generated for %1.").arg(m_formation), 3000);
}

void MainWindow::onApplySuggestion(
    const QVector<Models::SuggestedPlayer> &suggestions)
{
    QVector<Models::Player> xi;
    xi.reserve(suggestions.size());
    for (const auto &sp : suggestions)
        xi.append(sp.player);

    m_pitchView->setTeamPlayers(xi, true, QColor(0x1565C0));
    statusBar()->showMessage(QStringLiteral("Lineup applied to pitch."), 3000);
}

void MainWindow::onCompareFormations()
{
    if (m_currentPlayers.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("Please select a team first."), 3000);
        return;
    }
    const QMap<QString, double> scores =
        m_engine->compareFormations(m_currentPlayers);
    m_suggestionPanel->setCompareData(scores);
}

void MainWindow::onExportPng()
{
    const QString path = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Export Lineup as PNG"),
        QStringLiteral("lineup.png"),
        QStringLiteral("PNG Images (*.png)"));

    if (path.isEmpty()) return;

    const QImage img = m_pitchView->exportToImage();
    if (!img.save(path))
        QMessageBox::warning(this, QStringLiteral("Export Failed"),
                             QStringLiteral("Could not save image to: ") + path);
    else
        statusBar()->showMessage(QStringLiteral("Exported to ") + path, 4000);
}

void MainWindow::onOpenSettings()
{
    SettingsDialog dlg(m_db, this);
    connect(&dlg, &SettingsDialog::themeChanged,
            this, &MainWindow::applyTheme);
    if (dlg.exec() == QDialog::Accepted) {
        m_apiClient->setApiKey(dlg.apiKey());
    }
}

void MainWindow::onLoadingChanged(bool loading)
{
    m_progressBar->setVisible(loading);
    if (!loading)
        m_progressBar->setVisible(false);
}

void MainWindow::onApiError(const QString &error)
{
    statusBar()->showMessage(QStringLiteral("⚠  ") + error, 6000);
    m_progressBar->setVisible(false);
}

void MainWindow::onPlayerDroppedOnPitch(int playerId, QPointF scenePos)
{
    Q_UNUSED(scenePos)
    // Find the player in the current roster
    for (const Models::Player &p : m_currentPlayers) {
        if (p.id == playerId) {
            statusBar()->showMessage(
                QString("Placed %1 on pitch.").arg(p.name), 2000);
            return;
        }
    }
}

// ── Window state ─────────────────────────────────────────────────────────────

void MainWindow::saveWindowState()
{
    QSettings cfg;
    cfg.setValue(QStringLiteral("window/geometry"), saveGeometry());
    cfg.setValue(QStringLiteral("window/state"),    saveState());
}

void MainWindow::restoreWindowState()
{
    QSettings cfg;
    if (cfg.contains(QStringLiteral("window/geometry")))
        restoreGeometry(cfg.value(QStringLiteral("window/geometry")).toByteArray());
    if (cfg.contains(QStringLiteral("window/state")))
        restoreState(cfg.value(QStringLiteral("window/state")).toByteArray());
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveWindowState();
    QMainWindow::closeEvent(event);
}
