#pragma once

#include <QWidget>
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QVector>
#include "api/ApiModels.h"

// ── Data Model ────────────────────────────────────────────────────────────────

class PlayerRosterModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        ColName        = 0,
        ColPosition    = 1,
        ColAge         = 2,
        ColNationality = 3,
        ColRating      = 4,
        ColCount
    };

    explicit PlayerRosterModel(QObject *parent = nullptr);

    void setPlayers(const QVector<Models::Player> &players);
    const QVector<Models::Player> &players() const { return m_players; }

    Models::Player playerAt(int row) const;

    // QAbstractTableModel
    int           rowCount   (const QModelIndex &parent = {}) const override;
    int           columnCount(const QModelIndex &parent = {}) const override;
    QVariant      data       (const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant      headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags      (const QModelIndex &index) const override;

    // Drag
    Qt::DropActions supportedDragActions() const override;
    QStringList     mimeTypes()            const override;
    QMimeData      *mimeData(const QModelIndexList &indexes) const override;

private:
    QVector<Models::Player> m_players;
};

// ── Widget ────────────────────────────────────────────────────────────────────

class QTableView;
class QComboBox;
class QLineEdit;

class PlayerRosterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PlayerRosterWidget(QWidget *parent = nullptr);

    void setPlayers(const QVector<Models::Player> &players);
    const QVector<Models::Player> &players() const;

signals:
    void playerDoubleClicked(const Models::Player &player);

private:
    void setupUi();
    void applyFilter();

    PlayerRosterModel      *m_model;
    QSortFilterProxyModel  *m_posProxy;   // first stage: position filter
    QSortFilterProxyModel  *m_nameProxy;  // second stage: name search
    QTableView             *m_view;
    QComboBox              *m_posFilter;
    QLineEdit              *m_searchBox;
};
