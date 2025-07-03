#ifndef GAUGE_WIDGET_H
#define GAUGE_WIDGET_H

#include <QWidget>
#include <QColor>
#include <QString>

class GaugeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GaugeWidget(const QString &title, const QString &unit, QWidget *parent = nullptr);
    ~GaugeWidget() override;
    void setRange(double min, double max);
    void setValue(double value);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_title;
    QString m_unit;
    double m_minValue;
    double m_maxValue;
    double m_currentValue;
    QColor m_arcColor;
};

#endif // GAUGE_WIDGET_H
