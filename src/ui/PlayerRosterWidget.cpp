#include "ui/PlayerRosterWidget.h"

#include <QTableView>
#include <QComboBox>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QMimeData>
#include <QFont>
#include <QColor>

// ── PlayerRosterModel ────────────────────────────────────────────────────────

PlayerRosterModel::PlayerRosterModel(QObject *parent)
    : QAbstractTableModel(parent)
{}

void PlayerRosterModel::setPlayers(const QVector<Models::Player> &players)
{
    beginResetModel();
    m_players = players;
    endResetModel();
}

Models::Player PlayerRosterModel::playerAt(int row) const
{
    if (row < 0 || row >= m_players.size()) return {};
    return m_players[row];
}

int PlayerRosterModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_players.size();
}

int PlayerRosterModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColCount;
}

QVariant PlayerRosterModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_players.size())
        return {};

    const Models::Player &p = m_players[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColName:        return p.name;
        case ColPosition:    return Models::positionToString(p.position);
        case ColAge:         return p.age > 0 ? QString::number(p.age) : QStringLiteral("-");
        case ColNationality: return p.nationality;
        case ColRating:      return QString::number(p.overallScore, 'f', 2);
        default: break;
        }
    }

    if (role == Qt::TextAlignmentRole) {
        if (index.column() == ColAge || index.column() == ColRating)
            return int(Qt::AlignCenter);
    }

    if (role == Qt::ForegroundRole) {
        switch (p.position) {
        case Models::Position::Goalkeeper: return QColor(0xFFA500); // orange
        case Models::Position::Defender:   return QColor(0x4FC3F7); // sky blue
        case Models::Position::Midfielder: return QColor(0xA5D6A7); // green
        case Models::Position::Forward:    return QColor(0xEF9A9A); // red
        default:                           return QColor(Qt::white);
        }
    }

    // Expose player id for drag
    if (role == Qt::UserRole)
        return p.id;

    return {};
}

QVariant PlayerRosterModel::headerData(int section,
                                        Qt::Orientation orientation,
                                        int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractTableModel::headerData(section, orientation, role);

    switch (section) {
    case ColName:        return QStringLiteral("Name");
    case ColPosition:    return QStringLiteral("Pos");
    case ColAge:         return QStringLiteral("Age");
    case ColNationality: return QStringLiteral("Nationality");
    case ColRating:      return QStringLiteral("Score");
    default: return {};
    }
}

Qt::ItemFlags PlayerRosterModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f = QAbstractTableModel::flags(index);
    if (index.isValid())
        f |= Qt::ItemIsDragEnabled;
    return f;
}

Qt::DropActions PlayerRosterModel::supportedDragActions() const
{
    return Qt::CopyAction;
}

QStringList PlayerRosterModel::mimeTypes() const
{
    return { QStringLiteral("application/x-player-id") };
}

QMimeData *PlayerRosterModel::mimeData(const QModelIndexList &indexes) const
{
    if (indexes.isEmpty()) return nullptr;

    const int row = indexes.first().row();
    if (row < 0 || row >= m_players.size()) return nullptr;

    auto *mime = new QMimeData;
    mime->setData(QStringLiteral("application/x-player-id"),
                  QByteArray::number(m_players[row].id));
    return mime;
}

// ── PlayerRosterWidget ────────────────────────────────────────────────────────

PlayerRosterWidget::PlayerRosterWidget(QWidget *parent)
    : QWidget(parent)
    , m_model    (new PlayerRosterModel(this))
    , m_posProxy (new QSortFilterProxyModel(this))
    , m_nameProxy(new QSortFilterProxyModel(this))
    , m_view     (new QTableView(this))
    , m_posFilter(new QComboBox(this))
    , m_searchBox(new QLineEdit(this))
{
    setupUi();
}

void PlayerRosterWidget::setupUi()
{
    // Chain: model → pos proxy → name proxy → view
    m_posProxy->setSourceModel(m_model);
    m_posProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_posProxy->setFilterKeyColumn(PlayerRosterModel::ColPosition);

    m_nameProxy->setSourceModel(m_posProxy);
    m_nameProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_nameProxy->setFilterKeyColumn(PlayerRosterModel::ColName);

    m_view->setModel(m_nameProxy);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->setDragEnabled(true);
    m_view->setDragDropMode(QAbstractItemView::DragOnly);
    m_view->setSortingEnabled(true);
    m_view->setAlternatingRowColors(true);
    m_view->verticalHeader()->setVisible(false);
    m_view->horizontalHeader()->setStretchLastSection(false);
    m_view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_view->horizontalHeader()->setSectionResizeMode(
        PlayerRosterModel::ColName, QHeaderView::Stretch);
    m_view->setShowGrid(false);

    // Styling
    m_view->setStyleSheet(
        "QTableView { background: #1e1e2e; color: #cdd6f4; border: none; }"
        "QTableView::item:selected { background: #45475a; }"
        "QTableView::item:alternate { background: #181825; }"
        "QHeaderView::section { background: #313244; color: #cba6f7; "
        "  border: none; padding: 4px; font-weight: bold; }");

    // Position filter
    m_posFilter->addItem(QStringLiteral("All"), QStringLiteral(""));
    m_posFilter->addItem(QStringLiteral("GK"),  QStringLiteral("GK"));
    m_posFilter->addItem(QStringLiteral("DEF"), QStringLiteral("DEF"));
    m_posFilter->addItem(QStringLiteral("MID"), QStringLiteral("MID"));
    m_posFilter->addItem(QStringLiteral("FWD"), QStringLiteral("FWD"));
    m_posFilter->setStyleSheet(
        "QComboBox { background: #313244; color: #cdd6f4; border: 1px solid #45475a; "
        "  padding: 2px 6px; border-radius: 4px; }");

    m_searchBox->setPlaceholderText(QStringLiteral("Search player…"));
    m_searchBox->setStyleSheet(
        "QLineEdit { background: #313244; color: #cdd6f4; border: 1px solid #45475a; "
        "  padding: 2px 6px; border-radius: 4px; }");

    // Toolbar row
    auto *filterRow = new QHBoxLayout;
    filterRow->setContentsMargins(0, 0, 0, 0);
    filterRow->addWidget(new QLabel(QStringLiteral("Filter:"), this));
    filterRow->addWidget(m_posFilter);
    filterRow->addWidget(m_searchBox, 1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addLayout(filterRow);
    layout->addWidget(m_view, 1);

    // Connections
    connect(m_posFilter, &QComboBox::currentIndexChanged,
            this, &PlayerRosterWidget::applyFilter);

    connect(m_searchBox, &QLineEdit::textChanged,
            this, &PlayerRosterWidget::applyFilter);

    connect(m_view, &QTableView::doubleClicked,
            this, [this](const QModelIndex &idx) {
        // Map through both proxy layers back to source
        const QModelIndex nameIdx = m_nameProxy->mapToSource(idx);
        const QModelIndex srcIdx  = m_posProxy->mapToSource(nameIdx);
        emit playerDoubleClicked(m_model->playerAt(srcIdx.row()));
    });
}

void PlayerRosterWidget::setPlayers(const QVector<Models::Player> &players)
{
    m_model->setPlayers(players);
    m_view->sortByColumn(PlayerRosterModel::ColRating, Qt::DescendingOrder);
}

const QVector<Models::Player> &PlayerRosterWidget::players() const
{
    return m_model->players();
}

void PlayerRosterWidget::applyFilter()
{
    // Stage 1: position filter — empty string means "show all"
    const QString posCode = m_posFilter->currentData().toString();
    m_posProxy->setFilterFixedString(posCode);

    // Stage 2: name search on top of position-filtered results
    m_nameProxy->setFilterFixedString(m_searchBox->text());
}
