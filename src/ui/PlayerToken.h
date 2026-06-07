#pragma once

#include <QGraphicsObject>
#include <QColor>
#include "api/ApiModels.h"

class PlayerToken : public QGraphicsObject
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

public:
    static constexpr qreal RADIUS = 28.0;

    explicit PlayerToken(const Models::Player &player,
                         const QColor         &color,
                         QGraphicsItem        *parent = nullptr);

    // QGraphicsItem interface
    QRectF boundingRect()                                                const override;
    void   paint(QPainter *painter,
                 const QStyleOptionGraphicsItem *option,
                 QWidget *widget = nullptr)                                     override;

    // Accessors
    const Models::Player &player() const { return m_player; }
    QColor                color()  const { return m_color;  }
    void                  setColor(const QColor &c);

    // Snap the token to the given scene position with a smooth animation
    void snapTo(const QPointF &scenePos);

signals:
    void colorChanged    (const QColor &color);
    void tokenMoved      (int playerId, QPointF newScenePos);
    void removeRequested (int playerId);   // emitted on "Remove from XI"

protected:
    void          mousePressEvent   (QGraphicsSceneMouseEvent *event)      override;
    void          mouseReleaseEvent (QGraphicsSceneMouseEvent *event)      override;
    void          mouseMoveEvent    (QGraphicsSceneMouseEvent *event)      override;
    void          contextMenuEvent  (QGraphicsSceneContextMenuEvent *event) override;
    QVariant      itemChange        (GraphicsItemChange change,
                                     const QVariant &value)                override;

private:
    Models::Player m_player;
    QColor         m_color;
    QPointF        m_dragStart;
    bool           m_dragging = false;
};
