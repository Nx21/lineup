#include "ui/PlayerToken.h"

#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QPropertyAnimation>
#include <QGraphicsScene>
#include <QFontMetrics>
#include <QCursor>

// ── Constructor ──────────────────────────────────────────────────────────────

PlayerToken::PlayerToken(const Models::Player &player,
                         const QColor         &color,
                         QGraphicsItem        *parent)
    : QGraphicsObject(parent)
    , m_player(player)
    , m_color(color)
{
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    setZValue(10);
    setCursor(QCursor(Qt::OpenHandCursor));
}

// ── Geometry ─────────────────────────────────────────────────────────────────

QRectF PlayerToken::boundingRect() const
{
    const qreal margin = 2.0;
    return QRectF(-RADIUS - margin, -RADIUS - margin,
                  (RADIUS + margin) * 2, (RADIUS + margin) * 2 + 14);
}

// ── Painting ─────────────────────────────────────────────────────────────────

void PlayerToken::paint(QPainter *painter,
                        const QStyleOptionGraphicsItem * /*option*/,
                        QWidget * /*widget*/)
{
    painter->setRenderHint(QPainter::Antialiasing);

    // Shadow
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 60));
    painter->drawEllipse(QRectF(-RADIUS + 2, -RADIUS + 4, RADIUS * 2, RADIUS * 2));

    // Circle fill
    const bool selected = isSelected();
    QRadialGradient grad(-RADIUS * 0.3, -RADIUS * 0.3, RADIUS * 1.2);
    grad.setColorAt(0, m_color.lighter(140));
    grad.setColorAt(1, m_color.darker(130));
    painter->setBrush(grad);
    painter->setPen(selected ? QPen(Qt::white, 3) : QPen(Qt::white, 1.5));
    painter->drawEllipse(QRectF(-RADIUS, -RADIUS, RADIUS * 2, RADIUS * 2));

    // Jersey number
    QFont numFont = painter->font();
    numFont.setBold(true);
    numFont.setPointSize(11);
    painter->setFont(numFont);
    painter->setPen(Qt::white);
    const QString num = QString::number(m_player.jerseyNumber > 0
                                        ? m_player.jerseyNumber
                                        : m_player.id % 99 + 1);
    painter->drawText(QRectF(-RADIUS, -RADIUS * 0.5, RADIUS * 2, RADIUS),
                      Qt::AlignCenter, num);

    // Name below number
    QFont nameFont = painter->font();
    nameFont.setBold(false);
    nameFont.setPointSize(7);
    painter->setFont(nameFont);
    painter->setPen(QColor(230, 230, 230));
    QString shortName = m_player.lastname.isEmpty()
                        ? m_player.name
                        : m_player.lastname;
    if (shortName.length() > 9)
        shortName = shortName.left(8) + QChar(0x2026); // ellipsis
    painter->drawText(QRectF(-RADIUS, RADIUS * 0.1, RADIUS * 2, RADIUS * 0.7),
                      Qt::AlignCenter, shortName);

    // Name label below the circle
    QFont labelFont;
    labelFont.setPointSize(7);
    painter->setFont(labelFont);
    painter->setPen(Qt::white);
    QString displayName = m_player.name;
    if (displayName.length() > 12)
        displayName = displayName.left(11) + QChar(0x2026);
    painter->drawText(QRectF(-RADIUS, RADIUS + 2, RADIUS * 2, 12),
                      Qt::AlignCenter, displayName);
}

// ── Color ─────────────────────────────────────────────────────────────────────

void PlayerToken::setColor(const QColor &c)
{
    if (m_color == c) return;
    m_color = c;
    emit colorChanged(c);
    update();
}

// ── Snap animation ───────────────────────────────────────────────────────────

void PlayerToken::snapTo(const QPointF &scenePos)
{
    auto *anim = new QPropertyAnimation(this, "pos", this);
    anim->setDuration(300);
    anim->setStartValue(pos());
    anim->setEndValue(scenePos);
    anim->setEasingCurve(QEasingCurve::OutBack);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

// ── Mouse events ─────────────────────────────────────────────────────────────

void PlayerToken::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    m_dragging  = true;
    m_dragStart = pos();
    setCursor(QCursor(Qt::ClosedHandCursor));
    setZValue(100);
    QGraphicsObject::mousePressEvent(event);
}

void PlayerToken::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsObject::mouseMoveEvent(event);
}

void PlayerToken::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    m_dragging = false;
    setCursor(QCursor(Qt::OpenHandCursor));
    setZValue(10);
    emit tokenMoved(m_player.id, scenePos());
    QGraphicsObject::mouseReleaseEvent(event);
}

QVariant PlayerToken::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged && scene()) {
        // Clamp token within scene rect
        const QRectF sceneR = scene()->sceneRect();
        QPointF p = value.toPointF();
        p.setX(qBound(sceneR.left()  + RADIUS, p.x(), sceneR.right()  - RADIUS));
        p.setY(qBound(sceneR.top()   + RADIUS, p.y(), sceneR.bottom() - RADIUS));
        return p;
    }
    return QGraphicsObject::itemChange(change, value);
}
