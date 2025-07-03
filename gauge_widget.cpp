#include "gauge_widget.h"
#include <QPainter>
#include <QPainterPath>
#include <cmath>

GaugeWidget::GaugeWidget(const QString &title, const QString &unit, QWidget *parent)
    : QWidget(parent),
    m_title(title),
    m_unit(unit),
    m_minValue(0.0),
    m_maxValue(100.0),
    m_currentValue(0.0),
    m_arcColor(Qt::green)
{
    setMinimumHeight(120);
}

void GaugeWidget::setRange(double min, double max)
{
    m_minValue = min;
    m_maxValue = max;
    update();
}

void GaugeWidget::setValue(double value)
{
    m_currentValue = qBound(m_minValue, value, m_maxValue);

    // 根据数值改变颜色
    double percentage = (m_currentValue - m_minValue) / (m_maxValue - m_minValue);
    if (percentage < 0.33) m_arcColor = QColor(52, 152, 219); // 蓝色
    else if (percentage < 0.66) m_arcColor = QColor(241, 196, 15); // 黄色
    else m_arcColor = QColor(231, 76, 60); // 红色

    update();
}

void GaugeWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int side = qMin(width(), height());
    painter.translate(width() / 2.0, height() / 2.0);
    painter.scale(side / 200.0, side / 200.0);

    // 绘制背景弧
    painter.setPen(QPen(QColor(220, 220, 220), 12, Qt::SolidLine, Qt::RoundCap));
    painter.drawArc(-80, -80, 160, 160, -45 * 16, 270 * 16);

    // 绘制前景弧
    double percentage = (m_currentValue - m_minValue) / (m_maxValue - m_minValue);
    int angle = static_cast<int>(percentage * 270.0);
    painter.setPen(QPen(m_arcColor, 12, Qt::SolidLine, Qt::RoundCap));
    painter.drawArc(-80, -80, 160, 160, -45 * 16, angle * 16);

    // 绘制中心文字
    painter.setPen(Qt::black);
    QFont font = painter.font();
    font.setPixelSize(36);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(QRectF(-50, -25, 100, 50), Qt::AlignCenter, QString::number(m_currentValue, 'f', 1));

    // 绘制单位
    font.setPixelSize(14);
    font.setBold(false);
    painter.setFont(font);
    painter.drawText(QRectF(-50, 15, 100, 20), Qt::AlignCenter, m_unit);

    // 绘制标题
    font.setPixelSize(16);
    painter.setFont(font);
    painter.drawText(QRectF(-100, 70, 200, 30), Qt::AlignCenter, m_title);
}

GaugeWidget::~GaugeWidget() {}
