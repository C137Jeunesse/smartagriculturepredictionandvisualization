// --- START OF FILE mainwindow.h ---

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSql/QSqlDatabase>
#include <QtCharts/QChartGlobal>
#include <memory>
#include "prediction.h"
#include<QDialog>

// Forward declarations
QT_BEGIN_NAMESPACE
class QAction;
class QWidget;
class QComboBox;
class QDateTimeEdit;
class QPushButton;
class QCheckBox;
class QSpinBox;
class QLabel;
class QTimer;
class QTextEdit;
class QSplitter;
class QChartView;
class QChart;
class QLineSeries;
class QDateTimeAxis;
class QValueAxis;
class QPieSeries;
QT_END_NAMESPACE

class SystemSuggestionDialog;
class GaugeWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // 公共方法，以便对话框可以调用
    void refreshSuggestion(SystemSuggestionDialog* dialog);

private slots:
    // UI Action Slots
    void onLoadDataClicked();
    void onRefreshClicked();
    void onAutoRefreshToggled(bool enabled);
    void onSystemSuggestionClicked();
    void onRetrainModelClicked(bool isFirstRun = false);
    void onExportDataClicked();
    void showAboutDialog();

    // Chart Interaction Slots
    void onSeriesHovered(const QPointF &point, bool state);

    // Internal Update Slots
    void updateDataAuto();
    void updateDashboard();
    void onStartTimeChanged(const QDateTime &dateTime);
    void onEndTimeChanged(const QDateTime &dateTime);

private:
    void initStyleSheet();
    void initMenu();
    bool initDatabase();
    void initUI();
    void initChart();
    void setupConnections();
    void populateCropAreaCombo();
    void initFertilizerPredictionSystem();

    void loadAndDisplayData();
    void createSampleData();
    QString getCurrentEnvironmentData();
    QMap<int, double> getLatestDataForCurrentArea();

    void updatePieChart(int cropAreaId, const QDateTime &startTime, const QDateTime &endTime);

    // --- UI Components ---
    QAction *m_retrainAction;
    QAction *m_exportAction;
    QAction *m_aboutAction;

    QWidget *m_centralWidget;
    QSplitter *m_mainSplitter;
    QSplitter *m_chartSplitter; // Re-add chart splitter

    GaugeWidget *m_tempGauge;
    GaugeWidget *m_humidityGauge;

    QComboBox *m_cropAreaCombo;
    QComboBox *m_dataTypeCombo;
    QDateTimeEdit *m_startTimeEdit;
    QDateTimeEdit *m_endTimeEdit;
    QPushButton *m_loadButton;
    QPushButton *m_refreshButton;
    QCheckBox *m_autoRefreshCheckBox;
    QSpinBox *m_refreshIntervalSpinBox;
    QPushButton *m_systemSuggestionButton;

    QLabel *m_chartTooltip;

    // Chart Views
    QChartView *m_chartView;
    QChartView *m_pieChartView; // Re-add pie chart view

    QChart *m_chart;
    QChart *m_pieChart;
    QLineSeries *m_series;
    QPieSeries *m_pieSeries;
    QDateTimeAxis *m_axisX;
    QValueAxis *m_axisY;

    // --- Backend Components ---
    QSqlDatabase m_db;
    QTimer *m_autoRefreshTimer;
    QTimer *m_dashboardUpdateTimer;
    std::unique_ptr<FertilizerPrediction::FertilizerPredictionSystem> m_predictionSystem;
    bool m_predictionSystemReady;
};


class SystemSuggestionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SystemSuggestionDialog(MainWindow* mainWindow, QWidget *parent = nullptr);
    void setSuggestionData(const std::vector<FertilizerPrediction::PredictionResult> &results, const QString &wateringTip);

signals:
    void refreshRequested(SystemSuggestionDialog* dialog);

private:
    QTextEdit *m_contentTextEdit;
    void setupUI();
};

#endif // MAINWINDOW_H
