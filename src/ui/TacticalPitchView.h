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

    // Returns the 11 snap positions for the given formation.
    // Positions are in scene coordinates.
    QVector<QPointF> formationPositions(const QString &formation,
                                        bool homeTeam) const;

    // Find the nearest snap position to a scene point
    QPointF nearestSnapPosition(const QPointF &scenePos, bool homeTeam) const;

    void clearTokens();

    // Add a single token at (or snapped to) a scene position.
    // Returns false if a token for this player is already on the pitch.
    bool addSingleToken(const Models::Player &player,
                        const QPointF        &scenePos,
                        bool                  homeTeam,
                        const QColor         &color);

    // Returns true if a token for playerId already exists on the pitch
    bool hasToken(int playerId) const;

    // Remove the token for playerId from the pitch. Returns true if found.
    bool removeToken(int playerId);

signals:
    void playerDroppedOnPitch   (int playerId, QPointF scenePos);
    void tokenRemovedFromPitch  (int playerId);

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

    // Drop a single player token at the nearest formation snap position.
    // homeTeam=true places in the bottom half (home).
    // Returns true if the token was placed (false if duplicate player already on pitch).
    bool addPlayerToken(const Models::Player &player,
                        const QPointF        &sceneDropPos,
                        bool                  homeTeam,
                        const QColor         &color);

    // Remove the token for playerId. Returns true if found.
    bool removePlayerToken(int playerId);

    void clearTokens();

    PitchScene *pitchScene() const { return m_scene; }

    // Render the current view to an image (for PNG export)
    QImage exportToImage() const;

signals:
    void formationChanged       (const QString &formation);
    void playerDroppedOnPitch   (int playerId, QPointF scenePos);
    void playerRemovedFromPitch (int playerId);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    PitchScene *m_scene;
};
