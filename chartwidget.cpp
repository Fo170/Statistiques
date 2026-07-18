#include "chartwidget.h"
#include "statsengine.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <cmath>
#include <algorithm>
#include <QToolTip>

ChartWidget::ChartWidget(QWidget* parent)
    : QWidget(parent), m_engine(nullptr), m_autoRange(true),
      m_dragging(false)
{
    setMouseTracking(true);
    setMinimumSize(400, 300);
    m_xMin = m_yMin = -10.0;
    m_xMax = m_yMax = 10.0;
    m_scaleX = m_scaleY = 1.0;
    m_offsetX = m_offsetY = 0.0;
}

void ChartWidget::setData(const QVector<QPointF>& points)
{
    m_points = points;
    if (m_autoRange) computeAutoRange();
    update();
}

void ChartWidget::setRegression(const StatsEngine* engine)
{
    m_engine = engine;
    if (m_autoRange && engine) {
        m_xMin = (double)engine->rg().xmin;
        m_xMax = (double)engine->rg().xmax;
        m_yMin = (double)engine->rg().ymin;
        m_yMax = (double)engine->rg().ymax;
        double dx = (m_xMax - m_xMin) * 0.1;
        double dy = (m_yMax - m_yMin) * 0.1;
        if (dx == 0) dx = 1.0;
        if (dy == 0) dy = 1.0;
        m_xMin -= dx; m_xMax += dx;
        m_yMin -= dy; m_yMax += dy;
    }
    update();
}

void ChartWidget::setAutoRange(bool enabled)
{
    m_autoRange = enabled;
    if (enabled) computeAutoRange();
    update();
}

void ChartWidget::resetView()
{
    m_offsetX = m_offsetY = 0.0;
    m_scaleX = m_scaleY = 1.0;
    if (m_autoRange) computeAutoRange();
    update();
}

void ChartWidget::computeAutoRange()
{
    if (m_points.isEmpty()) return;
    double xMin = m_points[0].x(), xMax = m_points[0].x();
    double yMin = m_points[0].y(), yMax = m_points[0].y();
    for (const auto& p : m_points) {
        xMin = std::min(xMin, p.x());
        xMax = std::max(xMax, p.x());
        yMin = std::min(yMin, p.y());
        yMax = std::max(yMax, p.y());
    }
    if (m_engine) {
        double fy;
        for (const auto& p : m_points) {
            fy = (double)m_engine->regFY(p.x());
            yMin = std::min(yMin, fy);
            yMax = std::max(yMax, fy);
        }
    }
    double dx = (xMax - xMin) * 0.1;
    double dy = (yMax - yMin) * 0.1;
    if (dx == 0) dx = 1.0;
    if (dy == 0) dy = 1.0;
    m_xMin = xMin - dx; m_xMax = xMax + dx;
    m_yMin = yMin - dy; m_yMax = yMax + dy;
}

void ChartWidget::worldToScreen(double wx, double wy, int& sx, int& sy) const
{
    int margin = 60;
    double w = width() - 2 * margin;
    double h = height() - 2 * margin;
    double rangeX = m_xMax - m_xMin;
    double rangeY = m_yMax - m_yMin;
    if (rangeX == 0) rangeX = 1;
    if (rangeY == 0) rangeY = 1;
    sx = margin + (int)((wx - m_xMin) / rangeX * w * m_scaleX + m_offsetX);
    sy = margin + (int)((1.0 - (wy - m_yMin) / rangeY) * h * m_scaleY + m_offsetY);
}

void ChartWidget::screenToWorld(int sx, int sy, double& wx, double& wy) const
{
    int margin = 60;
    double w = width() - 2 * margin;
    double h = height() - 2 * margin;
    double rangeX = m_xMax - m_xMin;
    double rangeY = m_yMax - m_yMin;
    if (m_scaleX != 0) sx = (int)((sx - m_offsetX) / m_scaleX);
    if (m_scaleY != 0) sy = (int)((sy - m_offsetY) / m_scaleY);
    wx = m_xMin + (double)(sx - margin) / w * rangeX;
    wy = m_yMin + (1.0 - (double)(sy - margin) / h) * rangeY;
}

void ChartWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    p.fillRect(rect(), Qt::white);

    drawGrid(p);
    drawAxes(p);
    drawRegressionCurve(p);
    drawDataPoints(p);
    drawInfo(p);
}

void ChartWidget::drawGrid(QPainter& p)
{
    p.setPen(QPen(QColor(220, 220, 220), 1, Qt::DotLine));

    double xStep = pow(10.0, floor(log10(m_xMax - m_xMin)) - 1);
    double yStep = pow(10.0, floor(log10(m_yMax - m_yMin)) - 1);
    xStep = std::max(xStep, 1e-15);
    yStep = std::max(yStep, 1e-15);

    for (double x = ceil(m_xMin / xStep) * xStep; x <= m_xMax; x += xStep) {
        int sx, sy1, sy2;
        worldToScreen(x, m_yMin, sx, sy1);
        worldToScreen(x, m_yMax, sx, sy2);
        p.drawLine(sx, sy1, sx, sy2);
    }
    for (double y = ceil(m_yMin / yStep) * yStep; y <= m_yMax; y += yStep) {
        int sx1, sy, sx2;
        worldToScreen(m_xMin, y, sx1, sy);
        worldToScreen(m_xMax, y, sx2, sy);
        p.drawLine(sx1, sy, sx2, sy);
    }
}

void ChartWidget::drawAxes(QPainter& p)
{
    p.setPen(QPen(Qt::black, 2));

    int sx0, sy0;
    worldToScreen(0, 0, sx0, sy0);

    int margin = 60;
    int x0 = margin;
    int x1 = width() - margin;
    int y0 = margin;
    int y1 = height() - margin;

    p.drawLine(x0, sy0, x1, sy0);
    p.drawLine(sx0, y0, sx0, y1);

    p.setPen(Qt::black);
    QFont f = font();
    f.setPointSize(8);
    p.setFont(f);

    double xStep = pow(10.0, floor(log10(m_xMax - m_xMin)) - 1);
    double yStep = pow(10.0, floor(log10(m_yMax - m_yMin)) - 1);
    xStep = std::max(xStep, 1e-15);
    yStep = std::max(yStep, 1e-15);

    for (double x = ceil(m_xMin / xStep) * xStep; x <= m_xMax; x += xStep) {
        int sx, sy;
        worldToScreen(x, 0, sx, sy);
        p.drawLine(sx, sy0 - 4, sx, sy0 + 4);
        p.drawText(sx - 20, sy0 + 16, 40, 16, Qt::AlignCenter,
                   QString::number(x, 'g', 4));
    }
    for (double y = ceil(m_yMin / yStep) * yStep; y <= m_yMax; y += yStep) {
        int sx, sy;
        worldToScreen(0, y, sx, sy);
        p.drawLine(sx0 - 4, sy, sx0 + 4, sy);
        p.drawText(sx0 - 50, sy - 8, 48, 16, Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(y, 'g', 4));
    }

    p.drawText(x1 - 16, sy0 - 8, "x");
    p.drawText(sx0 + 4, y0 + 12, "y");
}

void ChartWidget::drawDataPoints(QPainter& p)
{
    if (m_points.isEmpty()) return;
    p.setPen(QPen(Qt::blue, 1));
    p.setBrush(QBrush(Qt::blue));

    for (const auto& pt : m_points) {
        int sx, sy;
        worldToScreen(pt.x(), pt.y(), sx, sy);
        p.drawEllipse(QPoint(sx, sy), 3, 3);
    }
}

void ChartWidget::drawRegressionCurve(QPainter& p)
{
    if (!m_engine) return;
    p.setPen(QPen(Qt::red, 2));

    int steps = 500;
    double dx = (m_xMax - m_xMin) / steps;
    if (dx == 0) return;

    QPainterPath path;
    bool first = true;
    for (int i = 0; i <= steps; i++) {
        double wx = m_xMin + i * dx;
        double wy = (double)m_engine->regFY(wx);
        if (std::isnan(wy) || std::isinf(wy)) continue;
        int sx, sy;
        worldToScreen(wx, wy, sx, sy);
        if (sx < -1000 || sx > width() + 1000) continue;
        if (first) {
            path.moveTo(sx, sy);
            first = false;
        } else {
            path.lineTo(sx, sy);
        }
    }
    p.drawPath(path);
}

void ChartWidget::drawInfo(QPainter& p)
{
    if (!m_engine) return;
    p.setPen(Qt::black);
    QFont f = font();
    f.setPointSize(9);
    p.setFont(f);

    const auto& rg = m_engine->rg();
    QString formula;

    switch (rg.mode) {
    case 0: formula = QString("y=%1+%2*x").arg((double)rg.a, 0, 'g', 6)
                                           .arg((double)rg.b, 0, 'g', 6); break;
    case 1: formula = QString("y=%1+%2*ln(x+%3)").arg((double)rg.a, 0, 'g', 6)
                                                   .arg((double)rg.b, 0, 'g', 6)
                                                   .arg((double)rg.tx, 0, 'g', 4); break;
    case 2: formula = QString("y=%1*e^(%2*x)-%3").arg((double)rg.a, 0, 'g', 6)
                                                   .arg((double)rg.b, 0, 'g', 6)
                                                   .arg((double)rg.ty, 0, 'g', 4); break;
    case 3: formula = QString("y=%1*(x+%2)^%3-%4").arg((double)rg.a, 0, 'g', 6)
                                                     .arg((double)rg.tx, 0, 'g', 4)
                                                     .arg((double)rg.b, 0, 'g', 6)
                                                     .arg((double)rg.ty, 0, 'g', 4); break;
    case 4: formula = QString("y=%1*x^%2 (NLS)").arg((double)rg.a, 0, 'g', 6)
                                                  .arg((double)rg.b, 0, 'g', 6); break;
    }

    p.drawText(10, 26, QString("%1  r=%2  r^2=%3")
        .arg(formula).arg((double)rg.r, 0, 'g', 6).arg((double)rg.rcrit, 0, 'g', 6));
    p.drawText(10, 44, QString("N=%1  Tx=%2  Ty=%3")
        .arg((double)rg.n, 0, 'g', 4).arg((double)rg.tx, 0, 'g', 4).arg((double)rg.ty, 0, 'g', 4));
}

void ChartWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_lastPos = event->pos();
        m_lastOffX = m_offsetX;
        m_lastOffY = m_offsetY;
        setCursor(Qt::ClosedHandCursor);
    }
}

void ChartWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging) {
        QPoint delta = event->pos() - m_lastPos;
        m_offsetX = m_lastOffX + delta.x();
        m_offsetY = m_lastOffY + delta.y();
        update();
    } else {
        double wx, wy;
        screenToWorld(event->pos().x(), event->pos().y(), wx, wy);
        setToolTip(QString("X = %1\nY = %2").arg(wx, 0, 'g', 6).arg(wy, 0, 'g', 6));
    }
}

void ChartWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        setCursor(Qt::ArrowCursor);
    }
}

void ChartWidget::wheelEvent(QWheelEvent* event)
{
    double factor = (event->angleDelta().y() > 0) ? 1.15 : 0.87;
    m_scaleX *= factor;
    m_scaleY *= factor;
    update();
}

void ChartWidget::resizeEvent(QResizeEvent*)
{
    if (m_autoRange) computeAutoRange();
}
