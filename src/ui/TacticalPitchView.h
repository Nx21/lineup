#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMap>
#include <QVector>
#include <QPointF>
#include <QColor>
#include "api/ApiModels.h"

class PlayerToken;

// ── PitchScene ───────────────────────────────────────────────────────────────

class PitchScene : public QGraphicsScene
{
    Q_OBJECT

public:
    // Virtual pitch dimensions (scene units)
    static constexpr qreal PITCH_W = 680.0;
    static constexpr qreal PITCH_H = 1050.0;

    explicit PitchScene(QObject *parent = nullptr);

    void setFormation(const QString &formation);
    QString formation() const { return m_formation; }

    // Place tokens for a team. homeTeam=true → bottom half.
    void setTeamPlayers(const QVector<Models::Player> &players,
                        bool                           homeTeam,
                        const QColor                  &color);

    void clearTokens();

    // Returns the 11 snap positions for the given formation.
    // Positions are in scene coordinates.
    QVector<QPointF> formationPositions(const QString &formation,
                                        bool homeTeam) const;

    // Find the nearest snap position to a scene point
    QPointF nearestSnapPosition(const QPointF &scenePos, bool homeTeam) const;

signals:
    void playerDroppedOnPitch(int playerId, QPointF scenePos);

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void dragEnterEvent (QGraphicsSceneDragDropEvent *event) override;
    void dragMoveEvent  (QGraphicsSceneDragDropEvent *event) override;
    void dropEvent      (QGraphicsSceneDragDropEvent *event) override;

private:
    void drawPitch(QPainter *painter, const QRectF &rect);
    void initFormationMap();

    // Map: formation name → list of (xFraction, yFraction) for HOME team
    // Away team mirrors Y: yAway = 1 - yHome
    // xFraction: 0=left, 1=right
    // yFraction: 0=top of pitch, 1=bottom of pitch
    using FractionList = QVector<QPair<double, double>>;
    QMap<QString, FractionList> m_formationMap;

    QString               m_formation;
    QVector<PlayerToken*> m_homeTokens;
    QVector<PlayerToken*> m_awayTokens;
};

// ── TacticalPitchView ────────────────────────────────────────────────────────

class TacticalPitchView : public QGraphicsView
{
    Q_OBJECT
    Q_PROPERTY(QString formation READ formation WRITE setFormation NOTIFY formationChanged)

public:
    explicit TacticalPitchView(QWidget *parent = nullptr);

    QString formation() const;
    void    setFormation(const QString &formation);

    void setTeamPlayers(const QVector<Models::Player> &players,
                        bool                           homeTeam,
                        const QColor                  &color);

    void clearTokens();

    // Render the current view to an image (for PNG export)
    QImage exportToImage() const;

signals:
    void formationChanged(const QString &formation);
    void playerDroppedOnPitch(int playerId, QPointF scenePos);

private:
    PitchScene *m_scene;
};
