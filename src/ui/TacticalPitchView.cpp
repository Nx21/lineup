#include "ui/TacticalPitchView.h"
#include "ui/PlayerToken.h"

#include <QPainter>
#include <QGraphicsSceneDragDropEvent>
#include <QMimeData>
#include <QLinearGradient>
#include <QResizeEvent>

// ── Formation map ─────────────────────────────────────────────────────────────
// Positions as (xFrac, yFrac) for HOME team.
// Order: GK first, then DEF left→right, MID left→right, FWD left→right.
// yFrac = 0 means top of pitch; 1 means bottom.
// Home team occupies yFrac ∈ [0.52, 1.0]; away mirrors (1 - yFrac) ∈ [0, 0.48].

void PitchScene::initFormationMap()
{
    // 4-4-2
    m_formationMap[QStringLiteral("4-4-2")] = {
        {0.50, 0.94},                                             // GK
        {0.15,0.82},{0.38,0.82},{0.62,0.82},{0.85,0.82},         // DEF ×4
        {0.15,0.65},{0.38,0.65},{0.62,0.65},{0.85,0.65},         // MID ×4
        {0.35,0.50},{0.65,0.50}                                   // FWD ×2
    };

    // 4-3-3
    m_formationMap[QStringLiteral("4-3-3")] = {
        {0.50, 0.94},
        {0.15,0.82},{0.38,0.82},{0.62,0.82},{0.85,0.82},
        {0.25,0.65},{0.50,0.65},{0.75,0.65},
        {0.20,0.50},{0.50,0.50},{0.80,0.50}
    };

    // 3-5-2
    m_formationMap[QStringLiteral("3-5-2")] = {
        {0.50, 0.94},
        {0.25,0.82},{0.50,0.82},{0.75,0.82},
        {0.10,0.65},{0.30,0.65},{0.50,0.65},{0.70,0.65},{0.90,0.65},
        {0.35,0.50},{0.65,0.50}
    };

    // 4-2-3-1  (2 CDMs + 3 CAMs + 1 ST = 6 MID slots total)
    m_formationMap[QStringLiteral("4-2-3-1")] = {
        {0.50, 0.94},
        {0.15,0.82},{0.38,0.82},{0.62,0.82},{0.85,0.82},
        {0.35,0.70},{0.65,0.70},         // CDM pair
        {0.15,0.57},{0.50,0.57},{0.85,0.57},  // CAM trio
        {0.50,0.50}                       // ST
    };

    // 5-3-2
    m_formationMap[QStringLiteral("5-3-2")] = {
        {0.50, 0.94},
        {0.10,0.82},{0.28,0.82},{0.50,0.82},{0.72,0.82},{0.90,0.82},
        {0.25,0.65},{0.50,0.65},{0.75,0.65},
        {0.35,0.50},{0.65,0.50}
    };
}

// ── PitchScene ───────────────────────────────────────────────────────────────

PitchScene::PitchScene(QObject *parent)
    : QGraphicsScene(parent)
{
    setSceneRect(0, 0, PITCH_W, PITCH_H);
    setBackgroundBrush(Qt::NoBrush);
    initFormationMap();
    m_homeFormation = QStringLiteral("4-4-2");
    m_awayFormation = QStringLiteral("4-4-2");
}

void PitchScene::setFormation(const QString &formation)
{
    m_homeFormation = formation;
    update();
}

void PitchScene::setHomeFormation(const QString &formation)
{
    m_homeFormation = formation;
    update();
}

void PitchScene::setAwayFormation(const QString &formation)
{
    m_awayFormation = formation;
    update();
}

// ── Formation positions ───────────────────────────────────────────────────────

QVector<QPointF> PitchScene::formationPositions(
    const QString &formation, bool homeTeam) const
{
    const auto it = m_formationMap.find(formation);
    if (it == m_formationMap.end())
        return {};

    QVector<QPointF> pts;
    pts.reserve(it->size());
    for (const auto &frac : *it) {
        const double x = frac.first  * PITCH_W;
        const double y = homeTeam
            ? frac.second * PITCH_H
            : (1.0 - frac.second) * PITCH_H;
        pts.append({x, y});
    }
    return pts;
}

QPointF PitchScene::nearestSnapPosition(const QPointF &scenePos, bool homeTeam) const
{
    const QString &f = homeTeam ? m_homeFormation : m_awayFormation;
    const QVector<QPointF> pts = formationPositions(f, homeTeam);
    if (pts.isEmpty()) return scenePos;

    QPointF best = pts.first();
    qreal   bestDist = QLineF(scenePos, best).length();
    for (const QPointF &p : pts) {
        const qreal d = QLineF(scenePos, p).length();
        if (d < bestDist) { bestDist = d; best = p; }
    }
    return best;
}

// ── Token management ─────────────────────────────────────────────────────────

void PitchScene::setTeamPlayers(const QVector<Models::Player> &players,
                                 bool                           homeTeam,
                                 const QColor                  &color)
{
    // Remove old tokens for this team
    auto &tokens = homeTeam ? m_homeTokens : m_awayTokens;
    for (PlayerToken *t : tokens) { removeItem(t); delete t; }
    tokens.clear();

    const QVector<QPointF> positions = formationPositions(
        homeTeam ? m_homeFormation : m_awayFormation, homeTeam);
    const int count = qMin(players.size(), positions.size());

    for (int i = 0; i < count; ++i) {
        auto *token = new PlayerToken(players[i], color);
        addItem(token);
        token->setPos(positions[i]);
        connect(token, &PlayerToken::tokenMoved,      this, &PitchScene::playerDroppedOnPitch);
        connect(token, &PlayerToken::removeRequested, this, &PitchScene::removeToken);
        tokens.append(token);
    }
}

void PitchScene::clearTokens()
{
    for (PlayerToken *t : m_homeTokens) { removeItem(t); delete t; }
    for (PlayerToken *t : m_awayTokens) { removeItem(t); delete t; }
    m_homeTokens.clear();
    m_awayTokens.clear();
}

bool PitchScene::hasToken(int playerId) const
{
    for (const PlayerToken *t : m_homeTokens)
        if (t->player().id == playerId) return true;
    for (const PlayerToken *t : m_awayTokens)
        if (t->player().id == playerId) return true;
    return false;
}

bool PitchScene::removeToken(int playerId)
{
    auto removeFrom = [&](QVector<PlayerToken*> &tokens) -> bool {
        for (int i = 0; i < tokens.size(); ++i) {
            if (tokens[i]->player().id == playerId) {
                PlayerToken *t = tokens.takeAt(i);
                removeItem(t);
                t->deleteLater();
                emit tokenRemovedFromPitch(playerId);
                return true;
            }
        }
        return false;
    };
    return removeFrom(m_homeTokens) || removeFrom(m_awayTokens);
}

void PitchScene::setTeamColor(bool homeTeam, const QColor &color)
{
    auto &tokens = homeTeam ? m_homeTokens : m_awayTokens;
    for (PlayerToken *t : tokens)
        t->setColor(color);
}

bool PitchScene::addSingleToken(const Models::Player &player,
                                 const QPointF        &scenePos,
                                 bool                  homeTeam,
                                 const QColor         &color)
{
    if (hasToken(player.id)) return false;   // already on pitch

    // Snap to nearest empty formation slot
    const QVector<QPointF> snapPts = formationPositions(
        homeTeam ? m_homeFormation : m_awayFormation, homeTeam);
    auto &tokens = homeTeam ? m_homeTokens : m_awayTokens;

    // Find nearest slot not already occupied by another token
    QPointF bestPos = scenePos;
    qreal   bestDist = 1e9;
    for (const QPointF &sp : snapPts) {
        // Check if this slot is already taken
        bool taken = false;
        for (const PlayerToken *t : tokens)
            if (QLineF(t->pos(), sp).length() < 5.0) { taken = true; break; }
        if (taken) continue;

        const qreal d = QLineF(scenePos, sp).length();
        if (d < bestDist) { bestDist = d; bestPos = sp; }
    }

    auto *token = new PlayerToken(player, color);
    addItem(token);
    token->setPos(bestPos);
    connect(token, &PlayerToken::tokenMoved,      this, &PitchScene::playerDroppedOnPitch);
    connect(token, &PlayerToken::removeRequested, this, &PitchScene::removeToken);
    tokens.append(token);
    return true;
}

// ── Drag & drop (from roster) ─────────────────────────────────────────────────

void PitchScene::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    if (event->mimeData()->hasFormat(QStringLiteral("application/x-player-id")))
        event->acceptProposedAction();
    else
        event->ignore();
}

void PitchScene::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    if (event->mimeData()->hasFormat(QStringLiteral("application/x-player-id")))
        event->acceptProposedAction();
    else
        event->ignore();
}

void PitchScene::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    const QByteArray data =
        event->mimeData()->data(QStringLiteral("application/x-player-id"));
    bool ok = false;
    const int playerId = data.toInt(&ok);
    if (ok) {
        emit playerDroppedOnPitch(playerId, event->scenePos());
        event->acceptProposedAction();
    }
}

// ── Pitch drawing ─────────────────────────────────────────────────────────────

void PitchScene::drawBackground(QPainter *painter, const QRectF &rect)
{
    Q_UNUSED(rect)
    drawPitch(painter, sceneRect());
}

void PitchScene::drawPitch(QPainter *painter, const QRectF &r)
{
    const qreal W  = r.width();
    const qreal H  = r.height();
    const qreal ox = r.left();
    const qreal oy = r.top();

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // ── Grass gradient ────────────────────────────────────────────────────────
    QLinearGradient grassGrad(ox, oy, ox + W, oy + H);
    grassGrad.setColorAt(0.0, QColor(34, 120, 34));
    grassGrad.setColorAt(0.5, QColor(46, 140, 46));
    grassGrad.setColorAt(1.0, QColor(34, 120, 34));
    painter->fillRect(r, grassGrad);

    // Alternating stripe pattern
    painter->setOpacity(0.07);
    const int stripes = 8;
    const qreal stripeW = W / stripes;
    for (int i = 0; i < stripes; i += 2) {
        painter->fillRect(QRectF(ox + i * stripeW, oy, stripeW, H), Qt::white);
    }
    painter->setOpacity(1.0);

    // ── Line style ────────────────────────────────────────────────────────────
    QPen linePen(Qt::white, 2.5);
    linePen.setJoinStyle(Qt::MiterJoin);
    painter->setPen(linePen);
    painter->setBrush(Qt::NoBrush);

    // Outer boundary
    const qreal margin = 20.0;
    const QRectF field(ox + margin, oy + margin,
                       W - 2 * margin, H - 2 * margin);
    painter->drawRect(field);

    // Centre line
    painter->drawLine(QLineF(field.left(), field.center().y(),
                             field.right(), field.center().y()));

    // Centre circle (radius ~9.15 m; scale: 1m ≈ 6.8 px)
    const qreal ccR = 60.0;
    painter->drawEllipse(field.center(), ccR, ccR);

    // Centre spot
    painter->setBrush(Qt::white);
    painter->drawEllipse(field.center(), 3.5, 3.5);
    painter->setBrush(Qt::NoBrush);

    // ── Penalty areas ─────────────────────────────────────────────────────────
    // Standard: penalty area 40.32 m wide × 16.5 m deep
    const qreal paW = field.width() * 0.593;
    const qreal paH = field.height() * 0.099;
    const qreal paX = field.left() + (field.width() - paW) / 2.0;

    // Top penalty area
    painter->drawRect(QRectF(paX, field.top(), paW, paH));

    // Bottom penalty area
    painter->drawRect(QRectF(paX, field.bottom() - paH, paW, paH));

    // ── Goal areas ────────────────────────────────────────────────────────────
    const qreal gaW = field.width() * 0.278;
    const qreal gaH = field.height() * 0.038;
    const qreal gaX = field.left() + (field.width() - gaW) / 2.0;

    painter->drawRect(QRectF(gaX, field.top(), gaW, gaH));
    painter->drawRect(QRectF(gaX, field.bottom() - gaH, gaW, gaH));

    // ── Goals ─────────────────────────────────────────────────────────────────
    const qreal gW  = field.width() * 0.110;
    const qreal gD  = 12.0;
    const qreal gX  = field.left() + (field.width() - gW) / 2.0;
    painter->drawRect(QRectF(gX, field.top()    - gD, gW, gD));
    painter->drawRect(QRectF(gX, field.bottom(), gW, gD));

    // ── Penalty spots ─────────────────────────────────────────────────────────
    const qreal psY = field.height() * 0.0686; // 11m from goal line
    painter->setBrush(Qt::white);
    painter->drawEllipse(QPointF(field.center().x(), field.top()    + psY), 3.5, 3.5);
    painter->drawEllipse(QPointF(field.center().x(), field.bottom() - psY), 3.5, 3.5);
    painter->setBrush(Qt::NoBrush);

    // ── Penalty arcs ─────────────────────────────────────────────────────────
    const qreal arcR   = 60.0;
    const qreal arcTop = field.top() + paH;
    const qreal arcBot = field.bottom() - paH;
    const qreal cx     = field.center().x();

    painter->drawArc(QRectF(cx - arcR, arcTop - arcR, arcR * 2, arcR * 2),
                     -60 * 16, -60 * 16); // bottom arc of top area
    painter->drawArc(QRectF(cx - arcR, arcBot - arcR, arcR * 2, arcR * 2),
                     120 * 16, -60 * 16); // top arc of bottom area

    // ── Corner arcs ───────────────────────────────────────────────────────────
    const qreal cArcR = 10.0;
    painter->drawArc(QRectF(field.left()  - cArcR, field.top()    - cArcR,
                            cArcR * 2, cArcR * 2), 270 * 16,  90 * 16);
    painter->drawArc(QRectF(field.right() - cArcR, field.top()    - cArcR,
                            cArcR * 2, cArcR * 2), 180 * 16,  90 * 16);
    painter->drawArc(QRectF(field.left()  - cArcR, field.bottom() - cArcR,
                            cArcR * 2, cArcR * 2),   0 * 16,  90 * 16);
    painter->drawArc(QRectF(field.right() - cArcR, field.bottom() - cArcR,
                            cArcR * 2, cArcR * 2),  90 * 16,  90 * 16);

    painter->restore();
}

// ── TacticalPitchView ─────────────────────────────────────────────────────────

TacticalPitchView::TacticalPitchView(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(new PitchScene(this))
{
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setDragMode(QGraphicsView::NoDrag);
    setAcceptDrops(true);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setStyleSheet("background: transparent; border: none;");

    connect(m_scene, &PitchScene::playerDroppedOnPitch,
            this,    &TacticalPitchView::playerDroppedOnPitch);
    connect(m_scene, &PitchScene::tokenRemovedFromPitch,
            this,    &TacticalPitchView::playerRemovedFromPitch);
}

void TacticalPitchView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}

QString TacticalPitchView::formation() const     { return m_scene->homeFormation(); }
QString TacticalPitchView::homeFormation() const { return m_scene->homeFormation(); }
QString TacticalPitchView::awayFormation() const { return m_scene->awayFormation(); }

void TacticalPitchView::setFormation(const QString &formation)
{
    setHomeFormation(formation);
}

void TacticalPitchView::setHomeFormation(const QString &formation)
{
    if (m_scene->homeFormation() == formation) return;
    m_scene->setHomeFormation(formation);
    emit formationChanged(formation);
}

void TacticalPitchView::setAwayFormation(const QString &formation)
{
    if (m_scene->awayFormation() == formation) return;
    m_scene->setAwayFormation(formation);
}

void TacticalPitchView::setTeamPlayers(const QVector<Models::Player> &players,
                                        bool                           homeTeam,
                                        const QColor                  &color)
{
    m_scene->setTeamPlayers(players, homeTeam, color);
}

void TacticalPitchView::clearTokens()
{
    m_scene->clearTokens();
}

bool TacticalPitchView::addPlayerToken(const Models::Player &player,
                                        const QPointF        &sceneDropPos,
                                        bool                  homeTeam,
                                        const QColor         &color)
{
    return m_scene->addSingleToken(player, sceneDropPos, homeTeam, color);
}

bool TacticalPitchView::removePlayerToken(int playerId)
{
    return m_scene->removeToken(playerId);
}

void TacticalPitchView::setHomeTeamColor(const QColor &color)
{
    m_scene->setTeamColor(true, color);
}

void TacticalPitchView::setAwayTeamColor(const QColor &color)
{
    m_scene->setTeamColor(false, color);
}

QImage TacticalPitchView::exportToImage() const
{
    QImage img(m_scene->sceneRect().size().toSize(), QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter p(&img);
    m_scene->render(&p);
    return img;
}
