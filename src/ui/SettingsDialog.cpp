#include "ui/SettingsDialog.h"
#include "db/DatabaseManager.h"

#include <QSettings>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>

// ── Constructor ──────────────────────────────────────────────────────────────

SettingsDialog::SettingsDialog(DatabaseManager *db, QWidget *parent)
    : QDialog(parent)
    , m_apiKeyEdit    (new QLineEdit(this))
    , m_themeCombo    (new QComboBox(this))
    , m_lastFetchLabel(new QLabel(this))
    , m_clearBtn      (new QPushButton(QStringLiteral("Clear Cache"), this))
    , m_saveBtn       (new QPushButton(QStringLiteral("Save"), this))
    , m_cancelBtn     (new QPushButton(QStringLiteral("Cancel"), this))
    , m_db(db)
{
    setWindowTitle(QStringLiteral("Settings — WorldCup Analyst"));
    setModal(true);
    setMinimumWidth(420);
    setupUi();
    loadSettings();
}

void SettingsDialog::setupUi()
{
    const QString fieldStyle =
        "QLineEdit, QComboBox { background: #313244; color: #cdd6f4; "
        "  border: 1px solid #45475a; padding: 4px 8px; border-radius: 4px; }";
    const QString btnStyle =
        "QPushButton { background: #313244; color: #cba6f7; "
        "  border: 1px solid #45475a; border-radius: 6px; padding: 6px 16px; "
        "  font-weight: bold; }"
        "QPushButton:hover { background: #45475a; }";
    const QString dangerBtnStyle =
        "QPushButton { background: #313244; color: #f38ba8; "
        "  border: 1px solid #f38ba8; border-radius: 6px; padding: 6px 16px; }"
        "QPushButton:hover { background: #45475a; }";

    setStyleSheet("QDialog { background: #1e1e2e; color: #cdd6f4; }");

    // ── API section ───────────────────────────────────────────────────────────
    auto *apiGroup   = new QGroupBox(QStringLiteral("API-Football"), this);
    apiGroup->setStyleSheet(
        "QGroupBox { color: #cba6f7; font-weight: bold; "
        "  border: 1px solid #45475a; border-radius: 6px; margin-top: 8px; padding: 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; }");

    m_apiKeyEdit->setPlaceholderText(
        QStringLiteral("Paste your x-apisports-key here"));
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setStyleSheet(fieldStyle);

    auto *showKeyBtn = new QPushButton(QStringLiteral("👁"), this);
    showKeyBtn->setCheckable(true);
    showKeyBtn->setFixedWidth(32);
    showKeyBtn->setStyleSheet(btnStyle);
    connect(showKeyBtn, &QPushButton::toggled, this, [this](bool checked) {
        m_apiKeyEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
    });

    auto *keyRow = new QHBoxLayout;
    keyRow->addWidget(m_apiKeyEdit, 1);
    keyRow->addWidget(showKeyBtn);

    auto *apiForm = new QFormLayout(apiGroup);
    apiForm->addRow(QStringLiteral("API Key:"), keyRow);

    // ── Appearance section ────────────────────────────────────────────────────
    auto *appGroup = new QGroupBox(QStringLiteral("Appearance"), this);
    appGroup->setStyleSheet(apiGroup->styleSheet());

    m_themeCombo->addItem(QStringLiteral("Dark"),  QStringLiteral("dark"));
    m_themeCombo->addItem(QStringLiteral("Light"), QStringLiteral("light"));
    m_themeCombo->setStyleSheet(fieldStyle);

    auto *appForm = new QFormLayout(appGroup);
    appForm->addRow(QStringLiteral("Theme:"), m_themeCombo);

    // ── Cache section ─────────────────────────────────────────────────────────
    auto *cacheGroup = new QGroupBox(QStringLiteral("Cache"), this);
    cacheGroup->setStyleSheet(apiGroup->styleSheet());

    m_lastFetchLabel->setStyleSheet("color: #a6e3a1; font-size: 8pt;");
    m_clearBtn->setStyleSheet(dangerBtnStyle);

    auto *cacheForm = new QFormLayout(cacheGroup);
    cacheForm->addRow(QStringLiteral("Last fetch:"), m_lastFetchLabel);
    cacheForm->addRow(QString(), m_clearBtn);

    // ── Buttons ───────────────────────────────────────────────────────────────
    m_saveBtn->setDefault(true);
    m_saveBtn->setStyleSheet(btnStyle);
    m_cancelBtn->setStyleSheet(btnStyle);

    auto *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    btnRow->addWidget(m_cancelBtn);
    btnRow->addWidget(m_saveBtn);

    // ── Layout ────────────────────────────────────────────────────────────────
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->addWidget(apiGroup);
    mainLayout->addWidget(appGroup);
    mainLayout->addWidget(cacheGroup);
    mainLayout->addLayout(btnRow);

    // ── Connections ───────────────────────────────────────────────────────────
    connect(m_clearBtn,   &QPushButton::clicked, this, &SettingsDialog::onClearCache);
    connect(m_saveBtn,    &QPushButton::clicked, this, &SettingsDialog::onAccepted);
    connect(m_cancelBtn,  &QPushButton::clicked, this, &QDialog::reject);
}

// ── Settings persistence ──────────────────────────────────────────────────────

void SettingsDialog::loadSettings()
{
    QSettings cfg;
    m_apiKeyEdit->setText(cfg.value(QStringLiteral("api/key")).toString());

    const QString theme = cfg.value(QStringLiteral("ui/theme"),
                                    QStringLiteral("dark")).toString();
    const int idx = m_themeCombo->findData(theme);
    m_themeCombo->setCurrentIndex(idx >= 0 ? idx : 0);

    refreshLastFetch();
}

void SettingsDialog::saveSettings()
{
    QSettings cfg;
    cfg.setValue(QStringLiteral("api/key"),   m_apiKeyEdit->text().trimmed());
    cfg.setValue(QStringLiteral("ui/theme"),  m_themeCombo->currentData().toString());
}

QString SettingsDialog::apiKey() const
{
    return m_apiKeyEdit->text().trimmed();
}

QString SettingsDialog::theme() const
{
    return m_themeCombo->currentData().toString();
}

void SettingsDialog::refreshLastFetch()
{
    if (!m_db) { m_lastFetchLabel->setText(QStringLiteral("N/A")); return; }
    const QDateTime dt = m_db->lastFetchTime(QStringLiteral("teams"));
    m_lastFetchLabel->setText(dt.isValid()
        ? dt.toLocalTime().toString(QStringLiteral("dd MMM yyyy  hh:mm"))
        : QStringLiteral("Never"));
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void SettingsDialog::onClearCache()
{
    if (!m_db) return;
    const auto btn = QMessageBox::question(
        this,
        QStringLiteral("Clear Cache"),
        QStringLiteral("This will delete all locally cached team, player and fixture data.\n"
                       "Continue?"),
        QMessageBox::Yes | QMessageBox::No);

    if (btn == QMessageBox::Yes) {
        m_db->clearCache();
        refreshLastFetch();
        QMessageBox::information(this, QStringLiteral("Cache Cleared"),
                                 QStringLiteral("Cache cleared successfully."));
    }
}

void SettingsDialog::onAccepted()
{
    saveSettings();
    const QString newTheme = m_themeCombo->currentData().toString();
    emit themeChanged(newTheme);
    accept();
}
