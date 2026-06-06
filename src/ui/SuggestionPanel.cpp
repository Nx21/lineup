#include "ui/SuggestionPanel.h"

#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QListWidgetItem>
#include <QFont>

// ── Constructor ──────────────────────────────────────────────────────────────

SuggestionPanel::SuggestionPanel(QWidget *parent)
    : QDockWidget(QStringLiteral("Lineup Suggestions"), parent)
    , m_stack           (new QStackedWidget(this))
    , m_suggestionList  (new QListWidget)
    , m_applyBtn        (new QPushButton(QStringLiteral("✔  Apply Suggestion")))
    , m_compareBtn      (new QPushButton(QStringLiteral("⚖  Compare Formations")))
    , m_totalScoreLabel (new QLabel)
    , m_compareList     (new QListWidget)
    , m_backBtn         (new QPushButton(QStringLiteral("← Back")))
{
    setupUi();
}

void SuggestionPanel::setupUi()
{
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    setMinimumWidth(240);

    const QString panelStyle =
        "background: #1e1e2e; color: #cdd6f4;";
    const QString listStyle  =
        "QListWidget { background: #181825; border: none; color: #cdd6f4; }"
        "QListWidget::item { padding: 4px 6px; border-bottom: 1px solid #313244; }"
        "QListWidget::item:selected { background: #45475a; }";
    const QString btnStyle =
        "QPushButton { background: #313244; color: #cba6f7; border: 1px solid #45475a;"
        "  border-radius: 6px; padding: 6px 12px; font-weight: bold; }"
        "QPushButton:hover { background: #45475a; }"
        "QPushButton:pressed { background: #585b70; }";

    // ── Suggestion page ───────────────────────────────────────────────────────
    auto *sugPage   = new QWidget;
    auto *sugLayout = new QVBoxLayout(sugPage);
    sugLayout->setContentsMargins(6, 6, 6, 6);

    QFont hdrFont;
    hdrFont.setBold(true);
    hdrFont.setPointSize(9);

    auto *hdr = new QLabel(QStringLiteral("Suggested XI"));
    hdr->setFont(hdrFont);
    hdr->setAlignment(Qt::AlignCenter);
    hdr->setStyleSheet("color: #cba6f7; padding: 4px;");

    m_totalScoreLabel->setAlignment(Qt::AlignCenter);
    m_totalScoreLabel->setStyleSheet("color: #a6e3a1; font-size: 8pt;");

    m_suggestionList->setStyleSheet(listStyle);
    m_suggestionList->setDragDropMode(QAbstractItemView::NoDragDrop);

    m_applyBtn->setStyleSheet(btnStyle);
    m_compareBtn->setStyleSheet(btnStyle);

    auto *btnRow = new QHBoxLayout;
    btnRow->addWidget(m_applyBtn);
    btnRow->addWidget(m_compareBtn);

    sugLayout->addWidget(hdr);
    sugLayout->addWidget(m_totalScoreLabel);
    sugLayout->addWidget(m_suggestionList, 1);
    sugLayout->addLayout(btnRow);
    sugPage->setStyleSheet(panelStyle);

    // ── Compare page ──────────────────────────────────────────────────────────
    auto *cmpPage   = new QWidget;
    auto *cmpLayout = new QVBoxLayout(cmpPage);
    cmpLayout->setContentsMargins(6, 6, 6, 6);

    auto *cmpHdr = new QLabel(QStringLiteral("Formation Comparison"));
    cmpHdr->setFont(hdrFont);
    cmpHdr->setAlignment(Qt::AlignCenter);
    cmpHdr->setStyleSheet("color: #cba6f7; padding: 4px;");

    m_compareList->setStyleSheet(listStyle);
    m_backBtn->setStyleSheet(btnStyle);

    cmpLayout->addWidget(cmpHdr);
    cmpLayout->addWidget(m_compareList, 1);
    cmpLayout->addWidget(m_backBtn);
    cmpPage->setStyleSheet(panelStyle);

    // ── Stack ─────────────────────────────────────────────────────────────────
    m_stack->addWidget(sugPage);   // index 0
    m_stack->addWidget(cmpPage);   // index 1
    setWidget(m_stack);

    // ── Connections ───────────────────────────────────────────────────────────
    connect(m_applyBtn, &QPushButton::clicked, this, [this]() {
        if (!m_currentSuggestions.isEmpty())
            emit applySuggestionRequested(m_currentSuggestions);
    });

    connect(m_compareBtn, &QPushButton::clicked, this, [this]() {
        emit compareFormationsRequested();
    });

    connect(m_backBtn, &QPushButton::clicked,
            this, &SuggestionPanel::showSuggestionPage);
}

// ── Public API ────────────────────────────────────────────────────────────────

void SuggestionPanel::setSuggestions(
    const QVector<Models::SuggestedPlayer> &suggestions)
{
    m_currentSuggestions = suggestions;
    m_suggestionList->clear();

    double totalScore = 0.0;
    for (const Models::SuggestedPlayer &sp : suggestions) {
        totalScore += sp.score;

        const QString line = QString("[%1]  %2  —  %3")
            .arg(sp.slotLabel, -6)
            .arg(sp.player.name)
            .arg(QString::number(sp.score, 'f', 2));

        auto *item = new QListWidgetItem(line, m_suggestionList);

        // Colour-code by position
        switch (sp.player.position) {
        case Models::Position::Goalkeeper:
            item->setForeground(QColor(0xFFA500)); break;
        case Models::Position::Defender:
            item->setForeground(QColor(0x4FC3F7)); break;
        case Models::Position::Midfielder:
            item->setForeground(QColor(0xA5D6A7)); break;
        case Models::Position::Forward:
            item->setForeground(QColor(0xEF9A9A)); break;
        default:
            item->setForeground(QColor(Qt::white)); break;
        }
    }

    m_totalScoreLabel->setText(
        QString("Total squad score: %1").arg(QString::number(totalScore, 'f', 2)));

    showSuggestionPage();
}

void SuggestionPanel::setCompareData(const QMap<QString, double> &formationScores)
{
    m_compareList->clear();

    // Find max score for normalisation bar
    double maxScore = 0.0;
    for (double v : formationScores) maxScore = qMax(maxScore, v);

    // Sort descending
    QVector<QPair<double, QString>> sorted;
    for (auto it = formationScores.begin(); it != formationScores.end(); ++it)
        sorted.append({ it.value(), it.key() });
    std::sort(sorted.begin(), sorted.end(),
              [](const auto &a, const auto &b){ return a.first > b.first; });

    for (int i = 0; i < sorted.size(); ++i) {
        const QString bar = QString(
            int((sorted[i].first / (maxScore > 0 ? maxScore : 1)) * 18), QChar(0x2588));
        const QString line = QString("%1. %2  %3  (%4)")
            .arg(i + 1)
            .arg(sorted[i].second, -8)
            .arg(bar, -18)
            .arg(QString::number(sorted[i].first, 'f', 2));
        auto *item = new QListWidgetItem(line, m_compareList);
        if (i == 0)
            item->setForeground(QColor(0xF9E2AF)); // gold for best
    }

    showComparePage();
}

void SuggestionPanel::clearSuggestions()
{
    m_currentSuggestions.clear();
    m_suggestionList->clear();
    m_totalScoreLabel->clear();
    showSuggestionPage();
}

void SuggestionPanel::showSuggestionPage()
{
    m_stack->setCurrentIndex(0);
}

void SuggestionPanel::showComparePage()
{
    m_stack->setCurrentIndex(1);
}
