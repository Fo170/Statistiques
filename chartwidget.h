#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QWidget>
#include <QVector>
#include <QPointF>

class StatsEngine;

class ChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit ChartWidget(QWidget* parent = nullptr);

    void setData(const QVector<QPointF>& points);
    void setRegression(const StatsEngine* engine);
    void setAutoRange(bool enabled);
    void resetView();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QVector<QPointF> m_points;
    const StatsEngine* m_engine;

    double m_xMin, m_xMax, m_yMin, m_yMax;
    double m_scaleX, m_scaleY;
    double m_offsetX, m_offsetY;
    bool m_autoRange;

    bool m_dragging;
    QPoint m_lastPos;
    double m_lastOffX, m_lastOffY;

    void computeAutoRange();
    void worldToScreen(double wx, double wy, int& sx, int& sy) const;
    void screenToWorld(int sx, int sy, double& wx, double& wy) const;
    void drawAxes(QPainter& p);
    void drawGrid(QPainter& p);
    void drawDataPoints(QPainter& p);
    void drawRegressionCurve(QPainter& p);
    void drawInfo(QPainter& p);
};

#endif
