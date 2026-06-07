#include "ui/MainWindow.h"
#include <QApplication>
#include "api/ApiClient.h"
#include "api/ApiModels.h"
#include "db/DatabaseManager.h"
#include "engine/LineupSuggestionEngine.h"
#include "ui/TacticalPitchView.h"
#include "ui/PlayerRosterWidget.h"
#include "ui/SuggestionPanel.h"
#include "ui/SettingsDialog.h"
#include "data/SquadLoader.h"

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
#include <QTabWidget>
#include <QColorDialog>
#include <QSet>
#include <algorithm>

static constexpr QColor HOME_COLOR{0x15, 0x65, 0xC0};   // blue
static constexpr QColor AWAY_COLOR{0xC6, 0x28, 0x28};   // red

// ── Constructor / Destructor ─────────────────────────────────────────────────

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_db      (new DatabaseManager(this))
    , m_apiClient(nullptr)
    , m_engine  (new LineupSuggestionEngine(this))
    , m_pitchView       (new TacticalPitchView(this))
    , m_rosterWidget    (new PlayerRosterWidget(this))
    , m_awayRosterWidget(new PlayerRosterWidget(this))
    , m_suggestionPanel (new SuggestionPanel(this))
    , m_teamList        (new QListWidget(this))
    , m_formationCombo  (new QComboBox(this))
    , m_awayFormationCombo(new QComboBox(this))
    , m_progressBar     (new QProgressBar(this))
    , m_statusLabel     (new QLabel(this))
    , m_homeTeamLabel   (new QLabel(QStringLiteral("— none —"), this))
    , m_awayTeamLabel   (new QLabel(QStringLiteral("— none —"), this))
    , m_homeSlotBtn     (new QPushButton(QStringLiteral("🏠 Home"), this))
    , m_awaySlotBtn     (new QPushButton(QStringLiteral("✈ Away"), this))
    , m_homeColorBtn    (new QPushButton(this))
    , m_awayColorBtn    (new QPushButton(this))
    , m_suggestBtn      (new QPushButton(QStringLiteral("⚡ Suggest Lineup"), this))
    , m_exportBtn       (new QPushButton(QStringLiteral("📷 Export PNG"), this))
    , m_settingsBtn     (new QPushButton(QStringLiteral("⚙ Settings"), this))
    , m_formation       (QStringLiteral("4-4-2"))
    , m_awayFormation   (QStringLiteral("4-4-2"))
    , m_homeColor       (HOME_COLOR)
    , m_awayColor       (AWAY_COLOR)
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

    // Wire pitch drop / remove
    connect(m_pitchView, &TacticalPitchView::playerDroppedOnPitch,
            this, &MainWindow::onPlayerDroppedOnPitch);
    connect(m_pitchView, &TacticalPitchView::playerRemovedFromPitch,
            this, &MainWindow::onPlayerRemovedFromPitch);

    restoreWindowState();

    // Seed all 48 Wikipedia squads from Qt resources.
    // Only seeds teams that are not yet in the cache so re-runs are fast.
    {
        SquadLoader loader;
        const auto  allTeams  = loader.loadAll();
        bool        expired   = false;
        const auto  cached    = m_db->getCachedTeams(&expired);

        QSet<int> cachedIds;
        for (const auto &t : cached) cachedIds.insert(t.id);

        QVector<Models::Team> newTeams;
        for (const auto &t : allTeams)
            if (!cachedIds.contains(t.id)) newTeams.append(t);

        if (!newTeams.isEmpty()) {
            m_db->cacheTeams(newTeams);
            for (const auto &team : newTeams)
                m_db->cachePlayers(team.id, team.players);
        }
    }

    // Kick off initial team load (will hit the warm cache immediately)
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
    tb->setObjectName(QStringLiteral("MainToolbar"));
    tb->setMovable(false);
    tb->setIconSize(QSize(16, 16));
    tb->setStyleSheet(
        "QToolBar { background: #181825; border-bottom: 1px solid #313244; spacing: 6px; }"
        "QToolBar::separator { width: 1px; background: #45475a; margin: 4px 2px; }");

    const QString comboStyle =
        "QComboBox { background: #313244; color: #cdd6f4; "
        "  border: 1px solid #45475a; padding: 3px 8px; border-radius: 4px; min-width: 90px; }";

    // Home formation
    auto *formLabelH = new QLabel(QStringLiteral("  Formation(H): "), tb);
    formLabelH->setStyleSheet("color: #89b4fa; font-weight: bold;");
    tb->addWidget(formLabelH);

    const QStringList formations =
        { QStringLiteral("4-4-2"), QStringLiteral("4-3-3"),
          QStringLiteral("3-5-2"), QStringLiteral("4-2-3-1"),
          QStringLiteral("5-3-2") };
    m_formationCombo->addItems(formations);
    m_formationCombo->setCurrentText(m_formation);
    m_formationCombo->setStyleSheet(comboStyle);
    tb->addWidget(m_formationCombo);

    tb->addSeparator();

    // Away formation
    auto *formLabelA = new QLabel(QStringLiteral("  Formation(A): "), tb);
    formLabelA->setStyleSheet("color: #f38ba8; font-weight: bold;");
    tb->addWidget(formLabelA);

    m_awayFormationCombo->addItems(formations);
    m_awayFormationCombo->setCurrentText(m_awayFormation);
    m_awayFormationCombo->setStyleSheet(comboStyle);
    tb->addWidget(m_awayFormationCombo);

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

    connect(m_formationCombo,     &QComboBox::currentTextChanged,
            this, &MainWindow::onHomeFormationChanged);
    connect(m_awayFormationCombo, &QComboBox::currentTextChanged,
            this, &MainWindow::onAwayFormationChanged);
    connect(m_suggestBtn,  &QPushButton::clicked, this, &MainWindow::onSuggestLineup);
    connect(m_exportBtn,   &QPushButton::clicked, this, &MainWindow::onExportPng);
    connect(m_settingsBtn, &QPushButton::clicked, this, &MainWindow::onOpenSettings);
}

void MainWindow::setupSidebar()
{
    auto *sideWidget = new QWidget(this);
    auto *layout     = new QVBoxLayout(sideWidget);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    // ── Slot toggle buttons ─────────────────────────────────────────────────
    auto *slotRow = new QWidget(sideWidget);
    auto *slotLayout = new QHBoxLayout(slotRow);
    slotLayout->setContentsMargins(0, 0, 0, 0);
    slotLayout->setSpacing(4);
    slotLayout->addWidget(m_homeSlotBtn);
    slotLayout->addWidget(m_awaySlotBtn);
    layout->addWidget(slotRow);

    // ── Team info rows ──────────────────────────────────────────────────────
    auto swatchStyle = [](const QColor &c) {
        return QString("QPushButton { background:%1; border:2px solid #fff; "
                       "border-radius:3px; min-width:20px; max-width:20px; "
                       "min-height:20px; max-height:20px; }")
               .arg(c.name());
    };

    m_homeColorBtn->setFixedSize(20, 20);
    m_homeColorBtn->setStyleSheet(swatchStyle(m_homeColor));
    m_awayColorBtn->setFixedSize(20, 20);
    m_awayColorBtn->setStyleSheet(swatchStyle(m_awayColor));

    m_homeTeamLabel->setStyleSheet("color:#89b4fa; font-size:9pt;");
    m_homeTeamLabel->setTextFormat(Qt::PlainText);
    m_awayTeamLabel->setStyleSheet("color:#f38ba8; font-size:9pt;");
    m_awayTeamLabel->setTextFormat(Qt::PlainText);

    auto *homeRow = new QWidget(sideWidget);
    auto *homeRowLayout = new QHBoxLayout(homeRow);
    homeRowLayout->setContentsMargins(2, 0, 2, 0);
    homeRowLayout->setSpacing(4);
    auto *homeDot = new QLabel(QStringLiteral("🔵"), sideWidget);
    homeRowLayout->addWidget(homeDot);
    homeRowLayout->addWidget(m_homeColorBtn);
    homeRowLayout->addWidget(m_homeTeamLabel, 1);

    auto *awayRow = new QWidget(sideWidget);
    auto *awayRowLayout = new QHBoxLayout(awayRow);
    awayRowLayout->setContentsMargins(2, 0, 2, 0);
    awayRowLayout->setSpacing(4);
    auto *awayDot = new QLabel(QStringLiteral("🔴"), sideWidget);
    awayRowLayout->addWidget(awayDot);
    awayRowLayout->addWidget(m_awayColorBtn);
    awayRowLayout->addWidget(m_awayTeamLabel, 1);

    layout->addWidget(homeRow);
    layout->addWidget(awayRow);

    // ── Team list ───────────────────────────────────────────────────────────
    auto *hdr = new QLabel(QStringLiteral("🌍  Teams"), sideWidget);
    hdr->setStyleSheet(
        "font-weight: bold; color: #cba6f7; padding: 4px; font-size: 10pt;");
    layout->addWidget(hdr);

    m_teamList->setStyleSheet(
        "QListWidget { background: #181825; color: #cdd6f4; border: none; }"
        "QListWidget::item { padding: 6px 8px; border-bottom: 1px solid #313244; }"
        "QListWidget::item:selected { background: #45475a; }"
        "QListWidget::item:hover { background: #313244; }");
    layout->addWidget(m_teamList, 1);

    sideWidget->setStyleSheet("background: #1e1e2e;");
    sideWidget->setMinimumWidth(180);
    sideWidget->setMaximumWidth(260);

    auto *dock = new QDockWidget(QStringLiteral("Teams"), this);
    dock->setObjectName(QStringLiteral("TeamsDock"));
    dock->setWidget(sideWidget);
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    addDockWidget(Qt::LeftDockWidgetArea, dock);

    // Initial button styling
    updateSlotButtonStyles();

    connect(m_teamList, &QListWidget::itemClicked,
            this, &MainWindow::onTeamSelected);
    connect(m_homeSlotBtn, &QPushButton::clicked, this, [this]() {
        m_selectingHome = true;
        updateSlotButtonStyles();
    });
    connect(m_awaySlotBtn, &QPushButton::clicked, this, [this]() {
        m_selectingHome = false;
        updateSlotButtonStyles();
    });
    connect(m_homeColorBtn, &QPushButton::clicked, this, &MainWindow::onHomeColorPick);
    connect(m_awayColorBtn, &QPushButton::clicked, this, &MainWindow::onAwayColorPick);
}

void MainWindow::setupCentralWidget()
{
    // Pitch takes all the central area — no competition with roster
    setCentralWidget(m_pitchView);
}

void MainWindow::setupDocks()
{
    // Right dock: tabbed rosters on top, suggestions below
    auto *rosterTabs = new QTabWidget(this);
    rosterTabs->addTab(m_rosterWidget,     QStringLiteral("🏠 Home"));
    rosterTabs->addTab(m_awayRosterWidget, QStringLiteral("✈ Away"));
    rosterTabs->setStyleSheet(
        "QTabWidget::pane { border: none; }"
        "QTabBar::tab { background: #313244; color: #a6adc8; padding: 5px 12px; }"
        "QTabBar::tab:selected { background: #45475a; color: #cdd6f4; font-weight: bold; }");

    auto *rosterDock = new QDockWidget(QStringLiteral("Rosters"), this);
    rosterDock->setObjectName(QStringLiteral("RosterDock"));
    rosterDock->setWidget(rosterTabs);
    rosterDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    rosterDock->setMinimumWidth(280);
    addDockWidget(Qt::RightDockWidgetArea, rosterDock);

    m_suggestionPanel->setObjectName(QStringLiteral("SuggestionDock"));
    m_suggestionPanel->setMinimumWidth(280);
    addDockWidget(Qt::RightDockWidgetArea, m_suggestionPanel);

    splitDockWidget(rosterDock, m_suggestionPanel, Qt::Vertical);
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
        qobject_cast<QApplication*>(qApp)->setStyleSheet(
            "QMainWindow { background: #f5f5f5; }"
            "QWidget { background: #f5f5f5; color: #333333; }");
    } else {
        // Dark theme (Catppuccin Mocha palette)
        qobject_cast<QApplication*>(qApp)->setStyleSheet(
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

// ── Helper ─────────────────────────────────────────────────────────────────────

void MainWindow::updateSlotButtonStyles()
{
    const QString activeHomeStyle =
        "QPushButton { background:#1565C0; color:white; font-weight:bold; "
        "border:2px solid #42a5f5; border-radius:4px; padding:5px 12px; }";
    const QString activeAwayStyle =
        "QPushButton { background:#C62828; color:white; font-weight:bold; "
        "border:2px solid #ef9a9a; border-radius:4px; padding:5px 12px; }";
    const QString inactiveStyle =
        "QPushButton { background:#313244; color:#a6adc8; "
        "border:1px solid #45475a; border-radius:4px; padding:5px 12px; }";

    m_homeSlotBtn->setStyleSheet(m_selectingHome ? activeHomeStyle : inactiveStyle);
    m_awaySlotBtn->setStyleSheet(m_selectingHome ? inactiveStyle   : activeAwayStyle);
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
    std::sort(scored.begin(), scored.end(),
              [](const Models::Player &a, const Models::Player &b){
                  return a.overallScore > b.overallScore;
              });

    if (m_loadingForHome) {
        m_homeTeamPlayers = scored;
        m_rosterWidget->setPlayers(scored);
    } else {
        m_awayTeamPlayers = scored;
        m_awayRosterWidget->setPlayers(scored);
    }
    statusBar()->showMessage(
        QString("Loaded %1 players.").arg(scored.size()), 4000);
}

void MainWindow::onTeamSelected(QListWidgetItem *item)
{
    if (!item) return;
    const int teamId = item->data(Qt::UserRole).toInt();

    m_loadingForHome = m_selectingHome;
    if (m_selectingHome) {
        if (teamId == m_homeTeamId) return;
        m_homeTeamId = teamId;
        m_homeTeamPlayers.clear();
        m_rosterWidget->setPlayers({});
        m_homeTeamLabel->setText(item->text());
    } else {
        if (teamId == m_awayTeamId) return;
        m_awayTeamId = teamId;
        m_awayTeamPlayers.clear();
        m_awayRosterWidget->setPlayers({});
        m_awayTeamLabel->setText(item->text());
    }

    statusBar()->showMessage(
        QString("Loading players for %1…").arg(item->text()));
    m_apiClient->fetchPlayers(teamId);
}

void MainWindow::onHomeFormationChanged(const QString &formation)
{
    setFormation(formation);
}

void MainWindow::onAwayFormationChanged(const QString &formation)
{
    if (m_awayFormation == formation) return;
    m_awayFormation = formation;
    m_pitchView->setAwayFormation(formation);
}

void MainWindow::onSuggestLineup()
{
    m_suggestionForHome = m_selectingHome;
    const auto &players   = m_suggestionForHome ? m_homeTeamPlayers : m_awayTeamPlayers;
    const auto &formation = m_suggestionForHome ? m_formation       : m_awayFormation;
    const auto &teamLabel = m_suggestionForHome ? m_homeTeamLabel->text() : m_awayTeamLabel->text();

    if (players.isEmpty()) {
        statusBar()->showMessage(
            m_suggestionForHome ? QStringLiteral("Please select a home team first.")
                                : QStringLiteral("Please select an away team first."), 3000);
        return;
    }
    const auto suggestions = m_engine->suggestLineup(formation, players);
    m_suggestionPanel->setSuggestions(suggestions);
    statusBar()->showMessage(
        QString("Lineup suggestion generated for %1 (%2).").arg(teamLabel, formation), 3000);
}

void MainWindow::onApplySuggestion(
    const QVector<Models::SuggestedPlayer> &suggestions)
{
    QVector<Models::Player> xi;
    xi.reserve(suggestions.size());
    for (const auto &sp : suggestions)
        xi.append(sp.player);

    const bool   isHome = m_suggestionForHome;
    const QColor color  = isHome ? m_homeColor : m_awayColor;
    m_pitchView->setTeamPlayers(xi, isHome, color);
    statusBar()->showMessage(QStringLiteral("Lineup applied to pitch."), 3000);
}

void MainWindow::onCompareFormations()
{
    const auto &players = m_suggestionForHome ? m_homeTeamPlayers : m_awayTeamPlayers;
    if (players.isEmpty()) {
        statusBar()->showMessage(
            m_suggestionForHome ? QStringLiteral("Please select a home team first.")
                                : QStringLiteral("Please select an away team first."), 3000);
        return;
    }
    const QMap<QString, double> scores = m_engine->compareFormations(players);
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
    // Check home team first
    for (const Models::Player &p : m_homeTeamPlayers) {
        if (p.id == playerId) {
            const bool placed = m_pitchView->addPlayerToken(p, scenePos, true, m_homeColor);
            if (placed)
                statusBar()->showMessage(
                    QString("Placed %1 on pitch (home).").arg(p.name), 2000);
            else
                statusBar()->showMessage(
                    QString("%1 is already on the pitch.").arg(p.name), 2000);
            return;
        }
    }
    // Check away team
    for (const Models::Player &p : m_awayTeamPlayers) {
        if (p.id == playerId) {
            const bool placed = m_pitchView->addPlayerToken(p, scenePos, false, m_awayColor);
            if (placed)
                statusBar()->showMessage(
                    QString("Placed %1 on pitch (away).").arg(p.name), 2000);
            else
                statusBar()->showMessage(
                    QString("%1 is already on the pitch.").arg(p.name), 2000);
            return;
        }
    }
}

void MainWindow::onPlayerRemovedFromPitch(int playerId)
{
    for (const Models::Player &p : m_homeTeamPlayers) {
        if (p.id == playerId) {
            statusBar()->showMessage(
                QString("Removed %1 from XI — drag a replacement from the roster.")
                    .arg(p.name), 4000);
            return;
        }
    }
    for (const Models::Player &p : m_awayTeamPlayers) {
        if (p.id == playerId) {
            statusBar()->showMessage(
                QString("Removed %1 from XI — drag a replacement from the roster.")
                    .arg(p.name), 4000);
            return;
        }
    }
    statusBar()->showMessage(
        QStringLiteral("Player removed from XI — drag a replacement from the roster."),
        4000);
}

// ── Color pickers ─────────────────────────────────────────────────────────────

void MainWindow::onHomeColorPick()
{
    const QColor c = QColorDialog::getColor(m_homeColor, this,
                                            QStringLiteral("Home Team Color"));
    if (!c.isValid()) return;
    m_homeColor = c;
    m_homeColorBtn->setStyleSheet(
        QString("QPushButton { background:%1; border:2px solid #fff; "
                "border-radius:3px; min-width:20px; max-width:20px; "
                "min-height:20px; max-height:20px; }").arg(c.name()));
    m_pitchView->setHomeTeamColor(c);
}

void MainWindow::onAwayColorPick()
{
    const QColor c = QColorDialog::getColor(m_awayColor, this,
                                            QStringLiteral("Away Team Color"));
    if (!c.isValid()) return;
    m_awayColor = c;
    m_awayColorBtn->setStyleSheet(
        QString("QPushButton { background:%1; border:2px solid #fff; "
                "border-radius:3px; min-width:20px; max-width:20px; "
                "min-height:20px; max-height:20px; }").arg(c.name()));
    m_pitchView->setAwayTeamColor(c);
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

    // Ensure all docks are visible — a previously floating dock can get hidden
    // when features are changed to NoDockWidgetFeatures between sessions.
    for (auto *dock : findChildren<QDockWidget *>())
        dock->show();
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
