// --- START OF FILE mainwindow.cpp (v1.3 with Pie Chart and Suggestion Refresh) ---

#include "mainwindow.h"
#include "gauge_widget.h"

// Qt Core and Widgets
#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include <QDateTime>
#include <cmath>

// Qt UI Components
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSplitter>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QDateTimeEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QProgressDialog>
#include <QIcon>
#include <QFileDialog>
#include <QTextStream>

// Qt SQL and Charts
#include <QtSql>
#include <QtCharts>


// =================================================================
// SystemSuggestionDialog Implementation (with Refresh Button)
// =================================================================

/**
 * @brief 构造函数，初始化系统建议对话框。
 * @param mainWindow 主窗口指针，用于连接刷新信号。
 * @param parent 父窗口部件。
 */
SystemSuggestionDialog::SystemSuggestionDialog(MainWindow* mainWindow, QWidget *parent)
    : QDialog(parent)
{
    setupUI();
    // 将对话框的刷新请求信号连接到主窗口的刷新槽函数
    connect(this, &SystemSuggestionDialog::refreshRequested, mainWindow, &MainWindow::refreshSuggestion);
}

/**
 * @brief 设置对话框的UI布局和控件。
 */
void SystemSuggestionDialog::setupUI()
{
    setWindowTitle("系统智能建议");
    setMinimumSize(480, 420);

    auto layout = new QVBoxLayout(this);
    m_contentTextEdit = new QTextEdit(this);
    m_contentTextEdit->setReadOnly(true); // 内容设为只读

    layout->addWidget(new QLabel("<b>根据当前环境数据分析，系统建议如下：</b>"));
    layout->addWidget(m_contentTextEdit);

    // 底部按钮布局
    auto buttonLayout = new QHBoxLayout();
    auto refreshButton = new QPushButton(QIcon::fromTheme("view-refresh"), " 刷新建议");
    auto closeButton = new QPushButton(QIcon::fromTheme("dialog-close"), " 关闭");

    buttonLayout->addStretch();
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addWidget(closeButton);
    layout->addLayout(buttonLayout);

    // 连接按钮信号
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(refreshButton, &QPushButton::clicked, this, [this]() {
        // 当点击刷新按钮时，发射 refreshRequested 信号，并传递自身指针
        emit refreshRequested(this);
    });
}

/**
 * @brief 将预测结果和浇水建议格式化为HTML并显示在文本框中。
 * @param results 肥料预测结果列表。
 * @param wateringTip 浇水建议字符串。
 */
void SystemSuggestionDialog::setSuggestionData(const std::vector<FertilizerPrediction::PredictionResult> &results, const QString &wateringTip)
{
    QString htmlContent;

    // --- 肥料推荐部分 ---
    htmlContent += "<h3><font color='#2c3e50'>肥料推荐</font></h3>";
    htmlContent += "<table width='100%' cellspacing='0' cellpadding='5' style='background-color: #ffffff; border: 1px solid #ddd; border-radius: 4px;'>";
    htmlContent += "<tr style='background-color: #3498db; color: white;'><th>肥料类型</th><th>置信度</th></tr>";

    if (results.empty()) {
        htmlContent += "<tr><td colspan='2' align='center'>无可用肥料推荐。</td></tr>";
    } else {
        for (const auto &result : results) {
            htmlContent += QString("<tr><td>%1</td><td>%2%</td></tr>")
            .arg(QString::fromStdString(result.fertilizer))
                .arg(QString::number(result.confidence * 100, 'f', 2));
        }
    }
    htmlContent += "</table>";

    // --- 浇水建议部分 ---
    htmlContent += "<h3><font color='#2c3e50'>浇水建议</font></h3>";
    // 根据建议内容（包含"高"、"低"）选择不同的颜色以示提醒
    QString tipColor = wateringTip.contains("低") ? "#e74c3c" : (wateringTip.contains("高") ? "#f39c12" : "#2ecc71");
    htmlContent += QString("<p style='background-color: #ecf0f1; padding: 10px; border-radius: 5px;'><b><font color='%1'>%2</font></b></p>")
                       .arg(tipColor)
                       .arg(wateringTip);

    m_contentTextEdit->setHtml(htmlContent);
}


// =================================================================
// MainWindow Implementation
// =================================================================

/**
 * @brief MainWindow构造函数，应用程序的主入口。
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_predictionSystemReady(false), m_chartTooltip(nullptr)
{
    // --- 1. 基本窗口设置 ---
    setWindowTitle("智慧农业数据可视化与决策系统");
    setMinimumSize(1360, 800);
    setWindowIcon(QIcon::fromTheme("applications-science"));

    // --- 2. 初始化 ---
    initStyleSheet();        // 加载全局样式表
    initMenu();              // 初始化菜单栏
    initUI();                // 初始化主界面布局
    initChart();             // 初始化图表
    setupConnections();      // 设置所有信号和槽的连接

    // --- 3. 数据库和数据 ---
    if (!initDatabase()) {
        QMessageBox::critical(this, "数据库错误", "无法连接到数据库。应用程序功能将受限。");
    } else {
        populateCropAreaCombo(); // 从数据库加载作物区域
    }

    // 设置默认时间范围（过去7天）
    m_endTimeEdit->setDateTime(QDateTime::currentDateTime());
    m_startTimeEdit->setDateTime(QDateTime::currentDateTime().addDays(-7));

    // --- 4. 定时器 ---
    // 自动刷新定时器（默认关闭）
    m_autoRefreshTimer = new QTimer(this);
    connect(m_autoRefreshTimer, &QTimer::timeout, this, &MainWindow::updateDataAuto);

    // 仪表盘定时器（每5秒更新一次）
    m_dashboardUpdateTimer = new QTimer(this);
    connect(m_dashboardUpdateTimer, &QTimer::timeout, this, &MainWindow::updateDashboard);
    m_dashboardUpdateTimer->start(5000);

    // --- 5. 核心功能 ---
    initFertilizerPredictionSystem(); // 初始化肥料预测系统

    // --- 6. 初始加载 ---
    if (m_cropAreaCombo->count() > 0) {
        onLoadDataClicked(); // 如果有数据，则加载初始图表
    } else {
        m_chart->setTitle("数据库中无作物区数据。");
    }

    statusBar()->showMessage("系统准备就绪。");
}

/**
 * @brief MainWindow析构函数。
 */
MainWindow::~MainWindow()
{
    // 确保数据库连接在程序退出时被关闭
    if (m_db.isOpen()) {
        m_db.close();
    }
}

/**
 * @brief 初始化应用程序的全局样式表 (CSS)。
 */
void MainWindow::initStyleSheet()
{
    qApp->setStyleSheet(
        "QMainWindow { background-color: #f4f6f8; }"
        "QGroupBox { font-weight: bold; background-color: #ffffff; border: 1px solid #dfe4ea; border-radius: 8px; margin-top: 1ex; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top center; padding: 0 10px; background-color: #ffffff; }"
        "QPushButton { background-color: #3498db; color: white; border: none; padding: 8px 16px; border-radius: 4px; font-size: 10pt; }"
        "QPushButton:hover { background-color: #2980b9; }"
        "QPushButton:pressed { background-color: #1f618d; }"
        "QPushButton:disabled { background-color: #bdc3c7; color: #7f8c8d; }"
        "QComboBox, QDateTimeEdit, QSpinBox { padding: 5px; border: 1px solid #ced4da; border-radius: 4px; background-color: white; }"
        "QComboBox::drop-down { border: none; }"
        "QLabel { color: #495057; }"
        "QChartView { border: 1px solid #dfe4ea; border-radius: 8px; background-color: white; }"
        "QMenuBar { background-color: #ffffff; border-bottom: 1px solid #dfe4ea; }"
        "QStatusBar { font-size: 9pt; color: #6c757d; }"
        "#chartTooltip { background: rgba(0, 0, 0, 0.7); color: white; padding: 5px; border-radius: 3px; border: 1px solid white; }"
        );
}

/**
 * @brief 初始化菜单栏（文件、帮助）。
 */
void MainWindow::initMenu()
{
    // 文件菜单
    auto fileMenu = menuBar()->addMenu("文件(&F)");
    m_exportAction = new QAction(QIcon::fromTheme("document-save"), "导出当前图表数据(&E)...", this);
    m_retrainAction = new QAction(QIcon::fromTheme("system-run"), "重新训练模型(&R)...", this);

    fileMenu->addAction(m_exportAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_retrainAction);
    fileMenu->addSeparator();
    fileMenu->addAction(QIcon::fromTheme("application-exit"), "退出(&X)", this, &QWidget::close);

    // 帮助菜单
    auto helpMenu = menuBar()->addMenu("帮助(&H)");
    m_aboutAction = new QAction(QIcon::fromTheme("help-about"), "关于(&A)", this);
    helpMenu->addAction(m_aboutAction);
}

/**
 * @brief 初始化主窗口UI布局。
 */
void MainWindow::initUI()
{
    // --- 1. 创建主布局和分割器 ---
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    m_mainSplitter = new QSplitter(Qt::Horizontal, m_centralWidget);

    // --- 2. 创建左侧面板 (仪表盘和控制) ---
    auto leftPanel = new QWidget();
    leftPanel->setMaximumWidth(340);
    leftPanel->setMinimumWidth(300);

    // 实时仪表盘
    auto dashboardGroup = new QGroupBox("实时仪表盘");
    auto dashboardLayout = new QHBoxLayout(dashboardGroup);
    m_tempGauge = new GaugeWidget("温度", "°C");
    m_tempGauge->setRange(0, 50);
    m_humidityGauge = new GaugeWidget("土壤湿度", "%");
    m_humidityGauge->setRange(0, 100);
    dashboardLayout->addWidget(m_tempGauge);
    dashboardLayout->addWidget(m_humidityGauge);

    // 数据控制面板
    auto controlGroup = new QGroupBox("数据控制");
    auto controlLayout = new QGridLayout(controlGroup);
    controlLayout->setSpacing(10);
    controlLayout->addWidget(new QLabel("作物区域:"), 0, 0);
    m_cropAreaCombo = new QComboBox();
    controlLayout->addWidget(m_cropAreaCombo, 0, 1);
    controlLayout->addWidget(new QLabel("数据类型:"), 1, 0);
    m_dataTypeCombo = new QComboBox();
    m_dataTypeCombo->addItems({"温度 (°C)", "空气湿度 (%)", "土壤湿度 (%)", "氮 (N) mg/kg", "磷 (P) mg/kg", "钾 (K) mg/kg"});
    controlLayout->addWidget(m_dataTypeCombo, 1, 1);
    controlLayout->addWidget(new QLabel("开始时间:"), 2, 0);
    m_startTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime());
    controlLayout->addWidget(m_startTimeEdit, 2, 1);
    controlLayout->addWidget(new QLabel("结束时间:"), 3, 0);
    m_endTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime());
    controlLayout->addWidget(m_endTimeEdit, 3, 1);
    m_startTimeEdit->setDisplayFormat("yyyy-MM-dd hh:mm");
    m_endTimeEdit->setDisplayFormat("yyyy-MM-dd hh:mm");

    // 操作按钮
    m_loadButton = new QPushButton(QIcon::fromTheme("document-open"), " 查询数据");
    controlLayout->addWidget(m_loadButton, 4, 0, 1, 2);
    m_refreshButton = new QPushButton(QIcon::fromTheme("view-refresh"), " 刷新视图");
    controlLayout->addWidget(m_refreshButton, 5, 0, 1, 2);

    // 自动刷新控件
    m_autoRefreshCheckBox = new QCheckBox("自动刷新");
    m_refreshIntervalSpinBox = new QSpinBox();
    m_refreshIntervalSpinBox->setRange(5, 300);
    m_refreshIntervalSpinBox->setValue(30);
    m_refreshIntervalSpinBox->setSuffix(" 秒");
    m_refreshIntervalSpinBox->setEnabled(false);
    controlLayout->addWidget(m_autoRefreshCheckBox, 6, 0);
    controlLayout->addWidget(m_refreshIntervalSpinBox, 6, 1);

    // 智能建议按钮
    m_systemSuggestionButton = new QPushButton(QIcon::fromTheme("system-help"), " 系统智能建议");

    // 组合左侧面板布局
    auto leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->addWidget(dashboardGroup);
    leftLayout->addWidget(controlGroup);
    leftLayout->addStretch();
    leftLayout->addWidget(m_systemSuggestionButton);

    // --- 3. 创建右侧面板 (图表) ---
    // 使用垂直分割器来容纳折线图和饼图
    m_chartSplitter = new QSplitter(Qt::Vertical);
    m_chartView = new QChartView();     // 用于显示折线图
    m_pieChartView = new QChartView();  // 用于显示饼图
    m_chartSplitter->addWidget(m_chartView);
    m_chartSplitter->addWidget(m_pieChartView);
    // 设置初始大小比例，折线图占3份，饼图占2份
    m_chartSplitter->setStretchFactor(0, 3);
    m_chartSplitter->setStretchFactor(1, 2);

    // --- 4. 组合主分割器 ---
    m_mainSplitter->addWidget(leftPanel);
    m_mainSplitter->addWidget(m_chartSplitter);
    m_mainSplitter->setStretchFactor(1, 1); // 让右侧图表区域随窗口拉伸

    auto mainLayout = new QHBoxLayout(m_centralWidget);
    mainLayout->addWidget(m_mainSplitter);
}


/**
 * @brief 初始化图表对象（折线图和饼图）。
 */
void MainWindow::initChart()
{
    // --- 折线图 (Line Chart) ---
    m_chart = new QChart();
    m_series = new QLineSeries();
    m_series->setName("传感器数据");
    m_chart->addSeries(m_series);
    m_chart->setAnimationOptions(QChart::SeriesAnimations);
    m_chart->legend()->hide();

    // X轴 (时间)
    m_axisX = new QDateTimeAxis;
    m_axisX->setFormat("MM-dd hh:mm");
    m_axisX->setTitleText("时间");
    m_chart->addAxis(m_axisX, Qt::AlignBottom);

    // Y轴 (数值)
    m_axisY = new QValueAxis;
    m_chart->addAxis(m_axisY, Qt::AlignLeft);

    m_series->attachAxis(m_axisX);
    m_series->attachAxis(m_axisY);

    m_chartView->setChart(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing); // 开启抗锯齿
    m_chartView->setRubberBand(QChartView::RectangleRubberBand); // 允许矩形缩放
    m_chart->setAcceptHoverEvents(true); // 允许悬停事件以显示提示

    // 自定义悬停提示标签
    m_chartTooltip = new QLabel(m_chartView);
    m_chartTooltip->setObjectName("chartTooltip");
    m_chartTooltip->hide();

    // --- 饼图 (Pie Chart) ---
    m_pieChart = new QChart();
    m_pieSeries = new QPieSeries();
    m_pieSeries->setHoleSize(0.35); // 创建一个环形图（甜甜圈图）
    m_pieChart->addSeries(m_pieSeries);
    m_pieChart->setAnimationOptions(QChart::AllAnimations);
    m_pieChartView->setChart(m_pieChart);
    m_pieChartView->setRenderHint(QPainter::Antialiasing);
}

/**
 * @brief 集中设置所有信号和槽的连接。
 */
void MainWindow::setupConnections()
{
    // 控件信号
    connect(m_loadButton, &QPushButton::clicked, this, &MainWindow::onLoadDataClicked);
    connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::onRefreshClicked);
    connect(m_autoRefreshCheckBox, &QCheckBox::toggled, this, &MainWindow::onAutoRefreshToggled);
    connect(m_systemSuggestionButton, &QPushButton::clicked, this, &MainWindow::onSystemSuggestionClicked);

    // 当选择项改变时，自动重新加载数据
    connect(m_cropAreaCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onLoadDataClicked);
    connect(m_dataTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onLoadDataClicked);

    // 时间范围限制
    connect(m_startTimeEdit, &QDateTimeEdit::dateTimeChanged, this, &MainWindow::onStartTimeChanged);
    connect(m_endTimeEdit, &QDateTimeEdit::dateTimeChanged, this, &MainWindow::onEndTimeChanged);

    // 菜单动作信号
    connect(m_retrainAction, &QAction::triggered, this, [this](){ onRetrainModelClicked(false); });
    connect(m_exportAction, &QAction::triggered, this, &MainWindow::onExportDataClicked);
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);

    // 图表交互信号
    connect(m_series, &QLineSeries::hovered, this, &MainWindow::onSeriesHovered);
}

/**
 * @brief 初始化SQLite数据库连接。
 * @return 如果连接成功返回 true，否则返回 false。
 */
bool MainWindow::initDatabase()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE", "agriculture_connection");
    m_db.setDatabaseName(QApplication::applicationDirPath() + "/agriculture.db");

    if (!m_db.open()) {
        return false;
    }

    // 创建数据表（如果不存在）
    QSqlQuery q(m_db);
    q.exec("CREATE TABLE IF NOT EXISTS sensor_data (id INTEGER PRIMARY KEY, crop_area_id INT, timestamp DATETIME, data_type INT, value REAL)");

    // 如果表是空的，则创建一些示例数据
    q.exec("SELECT COUNT(*) FROM sensor_data");
    if (q.next() && q.value(0).toInt() == 0) {
        createSampleData();
    }
    return true;
}

/**
 * @brief 初始化肥料预测系统。
 *        它会尝试加载现有模型，如果失败则尝试从CSV文件训练新模型。
 */
void MainWindow::initFertilizerPredictionSystem()
{
    m_predictionSystem = std::make_unique<FertilizerPrediction::FertilizerPredictionSystem>();
    QString modelPath = QApplication::applicationDirPath() + "/fertilizer_model.rf";
    QString csvPath = QApplication::applicationDirPath() + "/fertilizer_data.csv";

    // 检查预训练模型是否存在
    if (QFile::exists(modelPath)) {
        if (m_predictionSystem->loadModel(modelPath.toStdString())) {
            m_predictionSystemReady = true;
            statusBar()->showMessage("肥料预测模型加载成功。", 5000);
        } else {
            QMessageBox::critical(this, "模型加载失败", "找到模型文件但无法加载。请尝试删除该文件并重启以重新训练。");
            m_predictionSystemReady = false;
        }
    } else {
        // 如果模型不存在，检查训练数据是否存在
        if (!QFile::exists(csvPath)) {
            QMessageBox::critical(this, "严重错误", "未找到模型文件，也未找到训练数据 'fertilizer_data.csv'！预测功能将不可用。");
            m_predictionSystemReady = false;
        } else {
            // 从CSV文件进行首次训练
            onRetrainModelClicked(true);
        }
    }
    // 根据模型是否就绪来启用或禁用相关功能
    m_systemSuggestionButton->setEnabled(m_predictionSystemReady);
    m_retrainAction->setEnabled(QFile::exists(csvPath));
}

/**
 * @brief 当用户点击“重新训练模型”或首次启动时调用。
 * @param isFirstRun 如果是首次启动时自动调用，则为 true。
 */
void MainWindow::onRetrainModelClicked(bool isFirstRun)
{
    QString csvPath = QApplication::applicationDirPath() + "/fertilizer_data.csv";
    if (!QFile::exists(csvPath)) {
        QMessageBox::critical(this, "错误", "找不到训练数据 fertilizer_data.csv！");
        return;
    }

    // 如果不是首次运行，需要用户确认
    if (!isFirstRun) {
        auto reply = QMessageBox::question(this, "确认重新训练",
                                           "这将使用 'fertilizer_data.csv' 重新训练并覆盖现有模型。过程可能需要一些时间。是否继续？",
                                           QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No) {
            return;
        }
    } else {
        QMessageBox::information(this, "首次运行", "未找到预训练模型。将使用 'fertilizer_data.csv' 进行一次性训练，请稍候...");
    }

    // 显示进度对话框，因为训练可能耗时
    QProgressDialog progress("正在训练模型...", "取消", 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
    qApp->processEvents(); // 强制UI刷新

    if (!m_predictionSystem->loadDataFromCSV(csvPath.toStdString())) {
        QMessageBox::critical(this, "错误", "无法从CSV文件加载数据。");
        progress.close();
        return;
    }

    m_predictionSystem->trainModel();

    QString modelPath = QApplication::applicationDirPath() + "/fertilizer_model.rf";
    m_predictionSystem->saveModel(modelPath.toStdString());

    m_predictionSystemReady = m_predictionSystem->isModelReady();
    m_systemSuggestionButton->setEnabled(m_predictionSystemReady);

    progress.close();
    statusBar()->showMessage("模型已成功训练并保存。", 5000);
}

// ... Slot implementations ...

void MainWindow::onLoadDataClicked() { if(!m_db.isOpen()) return; loadAndDisplayData(); updateDashboard(); }
void MainWindow::onRefreshClicked() { onLoadDataClicked(); }
void MainWindow::onAutoRefreshToggled(bool enabled) { m_refreshIntervalSpinBox->setEnabled(enabled); if(enabled) m_autoRefreshTimer->start(m_refreshIntervalSpinBox->value()*1000); else m_autoRefreshTimer->stop(); }
void MainWindow::updateDataAuto() { m_endTimeEdit->setDateTime(QDateTime::currentDateTime()); onLoadDataClicked(); }
void MainWindow::onStartTimeChanged(const QDateTime &dt) { if(m_endTimeEdit) m_endTimeEdit->setMinimumDateTime(dt); }
void MainWindow::onEndTimeChanged(const QDateTime &dt) { if(m_startTimeEdit) m_startTimeEdit->setMaximumDateTime(dt); }

/**
 * @brief 处理折线图上的鼠标悬停事件，以显示数据点工具提示。
 * @param point 悬停点的数据坐标 (x=timestamp, y=value)。
 * @param state true表示鼠标进入点，false表示离开。
 */
void MainWindow::onSeriesHovered(const QPointF &point, bool state)
{
    if (state && m_chartTooltip) {
        QPoint curPos = QCursor::pos();
        QPoint viewPos = m_chartView->mapFromGlobal(curPos); // 将全局坐标转换为视图坐标
        m_chartTooltip->setText(QString("时间: %1\n数值: %2")
                                    .arg(QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(point.x())).toString("yyyy-MM-dd hh:mm"))
                                    .arg(point.y()));
        m_chartTooltip->move(viewPos.x() + 15, viewPos.y()); // 在光标右侧显示
        m_chartTooltip->adjustSize();
        m_chartTooltip->show();
    } else if (m_chartTooltip) {
        m_chartTooltip->hide();
    }
}

/**
 * @brief 当点击“系统智能建议”按钮时调用。
 */
void MainWindow::onSystemSuggestionClicked()
{
    if (!m_predictionSystemReady) {
        QMessageBox::critical(this, "错误", "预测模型未准备就绪。");
        return;
    }

    // 创建并显示对话框
    auto dialog = new SystemSuggestionDialog(this, this);
    refreshSuggestion(dialog); // 立即生成第一次建议
    dialog->exec();            // 以模态方式显示对话框
    delete dialog;             // 对话框关闭后释放内存
}

/**
 * @brief 刷新建议对话框中的内容。
 * @param dialog 需要刷新内容的对话框指针。
 */
void MainWindow::refreshSuggestion(SystemSuggestionDialog* dialog)
{
    if (!dialog) return;

    // 获取当前环境数据
    QString inputData = getCurrentEnvironmentData();
    if (inputData.isEmpty()) {
        QMessageBox::warning(this, "数据不足", "无法获取当前所选作物区域的完整环境数据进行预测。");
        return;
    }

    // 调用预测系统
    auto prediction = m_predictionSystem->predict(inputData.toStdString());

    // 更新对话框内容
    dialog->setSuggestionData(prediction.first, QString::fromStdString(prediction.second));
    statusBar()->showMessage("建议已刷新。", 3000);
}

/**
 * @brief 导出当前折线图中的数据到CSV文件。
 */
void MainWindow::onExportDataClicked()
{
    if (m_series->points().isEmpty()) {
        QMessageBox::warning(this, "无数据", "图表中没有可导出的数据。");
        return;
    }

    // 生成默认文件名
    QString defaultFileName = QString("export_%1_%2.csv")
                                  .arg(m_cropAreaCombo->currentText().replace(" ", "_"))
                                  .arg(QDate::currentDate().toString("yyyyMMdd"));

    QString fileName = QFileDialog::getSaveFileName(this, "导出为CSV", defaultFileName, "CSV 文件 (*.csv)");
    if (fileName.isEmpty()) return; // 用户取消

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "文件错误", "无法写入文件: " + file.errorString());
        return;
    }

    // 写入CSV数据
    QTextStream out(&file);
    out << "Timestamp,Value," << m_dataTypeCombo->currentText() << "\n";
    for (const QPointF &p : m_series->points()) {
        out << QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(p.x())).toString(Qt::ISODate) << "," << p.y() << "\n";
    }
    file.close();

    statusBar()->showMessage("数据已成功导出至 " + fileName, 5000);
}

/**
 * @brief 显示“关于”对话框。
 */
void MainWindow::showAboutDialog()
{
    QMessageBox::about(this, "关于本系统",
                       "<b>智慧农业数据可视化与决策系统 v1.3</b><br><br>"
                       "本系统集数据监控、图表可视化和智能决策支持于一体。<br><br>"
                       "<b>功能:</b><ul>"
                       "<li>实时传感器数据显示与历史趋势分析。</li>"
                       "<li>基于随机森林算法的智能施肥建议。</li>"
                       "<li>仪表盘实时监控。</li>"
                       "<li>图表交互与数据导出。");
}

/**
 * @brief 更新左侧的仪表盘（温度和湿度）。
 */
void MainWindow::updateDashboard()
{
    if (m_cropAreaCombo->currentIndex() < 0) return;

    QMap<int, double> latestData = getLatestDataForCurrentArea();
    if (latestData.contains(0)) m_tempGauge->setValue(latestData[0]);      // 0: 温度
    if (latestData.contains(2)) m_humidityGauge->setValue(latestData[2]);  // 2: 土壤湿度
}

/**
 * @brief 从数据库中查询所有唯一的作物区域ID，并填充到下拉框中。
 */
void MainWindow::populateCropAreaCombo()
{
    m_cropAreaCombo->blockSignals(true); // 暂时阻止信号，防止在填充时触发加载
    m_cropAreaCombo->clear();

    QSqlQuery q("SELECT DISTINCT crop_area_id FROM sensor_data ORDER BY crop_area_id", m_db);
    while (q.next()) {
        // 显示文本为"作物区 X"，关联数据为ID本身
        m_cropAreaCombo->addItem(QString("作物区 %1").arg(q.value(0).toInt()), q.value(0));
    }

    m_cropAreaCombo->blockSignals(false); // 恢复信号

    if (m_cropAreaCombo->count() > 0) {
        m_cropAreaCombo->setCurrentIndex(0);
    }
}

/**
 * @brief 如果数据库为空，则创建一些随机的示例数据。
 */
void MainWindow::createSampleData()
{
    m_db.transaction(); // 使用事务以提高插入性能
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO sensor_data (crop_area_id, timestamp, data_type, value) VALUES (?, ?, ?, ?)");

    for (int day = 0; day < 7; ++day) {
        for (int h = 0; h < 24; ++h) {
            for (int id : {101, 201, 301}) { // 三个不同的作物区
                q.bindValue(0, id);
                q.bindValue(1, QDateTime::currentDateTime().addDays(-day).addSecs(-h*3600));

                // 温度 (data_type=0) - 基础值 + 区域差异 + 日夜正弦变化 + 随机噪声
                q.bindValue(2, 0); q.bindValue(3, 25.0 + (id%10) + sin(h*M_PI/12)*5 + (rand()%10/5.0-1)); q.exec();
                // 空气湿度 (data_type=1)
                q.bindValue(2, 1); q.bindValue(3, 70.0 - (id%10) - cos(h*M_PI/12)*10+ (rand()%10/5.0-1)); q.exec();
                // 土壤湿度 (data_type=2)
                q.bindValue(2, 2); q.bindValue(3, 50.0 + (id%10) + sin(h*M_PI/24)*15+ (rand()%10/5.0-1)); q.exec();
                // 氮 (data_type=3)
                q.bindValue(2, 3); q.bindValue(3, 40.0 + (id%5) - (h%6)+ (rand()%10/5.0-1)); q.exec();
                // 磷 (data_type=4)
                q.bindValue(2, 4); q.bindValue(3, 60.0 + (id%5) - (h%8)+ (rand()%10/5.0-1)); q.exec();
                // 钾 (data_type=5)
                q.bindValue(2, 5); q.bindValue(3, 175.0+ (id%5) - (h%4)+ (rand()%10/5.0-1)); q.exec();
            }
        }
    }
    m_db.commit(); // 提交事务
}

/**
 * @brief 根据用户的选择，从数据库加载数据并更新折线图和饼图。
 */
void MainWindow::loadAndDisplayData()
{
    m_series->clear();
    if (m_cropAreaCombo->currentIndex() < 0) return;

    // 获取用户选择的参数
    int id = m_cropAreaCombo->currentData().toInt();
    int type = m_dataTypeCombo->currentIndex();
    QDateTime start = m_startTimeEdit->dateTime();
    QDateTime end = m_endTimeEdit->dateTime();

    // 准备SQL查询
    QSqlQuery q(m_db);
    q.prepare("SELECT timestamp, value FROM sensor_data WHERE crop_area_id = :id AND data_type = :type AND timestamp BETWEEN :start AND :end ORDER BY timestamp");
    q.bindValue(":id", id);
    q.bindValue(":type", type);
    q.bindValue(":start", start);
    q.bindValue(":end", end);
    q.exec();

    // 遍历查询结果，填充数据点并计算Y轴范围
    double minV = 1e9, maxV = -1e9;
    QList<QPointF> points;
    while(q.next()){
        points.append(QPointF(q.value(0).toDateTime().toMSecsSinceEpoch(), q.value(1).toDouble()));
        double val = q.value(1).toDouble();
        if (val < minV) minV = val;
        if (val > maxV) maxV = val;
    }
    m_series->replace(points); // 一次性替换数据点以提高性能

    // 更新折线图的标题和坐标轴
    m_chart->setTitle(QString("%1 - %2").arg(m_cropAreaCombo->currentText()).arg(m_dataTypeCombo->currentText()));
    m_axisX->setRange(start, end);

    if (!points.isEmpty()) {
        // 动态设置Y轴范围，并留出10%的边距
        double margin = (maxV - minV) * 0.1;
        m_axisY->setRange(minV - margin - 1, maxV + margin + 1);
    } else {
        m_axisY->setRange(0, 100); // 如果没有数据，使用默认范围
    }
    m_axisY->setTitleText(m_dataTypeCombo->currentText().split(' ').first());

    // 同步更新饼图数据
    updatePieChart(id, start, end);
}

/**
 * @brief 更新土壤N-P-K平均含量的饼图。
 * @param id 作物区域ID。
 * @param start 开始时间。
 * @param end 结束时间。
 */
void MainWindow::updatePieChart(int id, const QDateTime &start, const QDateTime &end)
{
    m_pieSeries->clear();
    QSqlQuery q(m_db);
    // 查询指定时间范围内N,P,K (type 3,4,5) 的平均值
    q.prepare("SELECT data_type, AVG(value) FROM sensor_data "
              "WHERE crop_area_id = :id AND data_type IN (3,4,5) AND timestamp BETWEEN :start AND :end "
              "GROUP BY data_type");
    q.bindValue(":id", id);
    q.bindValue(":start", start);
    q.bindValue(":end", end);
    q.exec();

    bool hasData = false;
    while(q.next()) {
        hasData = true;
        QString name = (q.value(0).toInt()==3 ? "氮(N)" : (q.value(0).toInt()==4 ? "磷(P)" : "钾(K)"));
        QPieSlice *slice = m_pieSeries->append(name, q.value(1).toDouble());

        // *** 这是修正过的代码 ***
        slice->setLabel(QString("%1\n%2%").arg(name).arg(slice->percentage() * 100, 0, 'f', 1));

        slice->setLabelVisible(true);
        slice->setLabelPosition(QPieSlice::LabelOutside); // 标签放在外部，防止重叠
    }

    // 根据是否有数据设置饼图标题
    m_pieChart->setTitle(hasData ? "土壤 N-P-K 平均含量" : "当前时段无土壤元素数据");
}

/**
 * @brief 获取当前选定作物区域所有传感器类型的最新一条数据。
 * @return QMap，键为数据类型(0-5)，值为最新数据值。如果任何一个类型数据缺失，返回空Map。
 */
QMap<int, double> MainWindow::getLatestDataForCurrentArea()
{
    QMap<int, double> dataMap;
    if (m_cropAreaCombo->currentIndex() < 0) return dataMap;
    int id = m_cropAreaCombo->currentData().toInt();

    // 遍历所有数据类型 (0到5)
    for (int i = 0; i <= 5; ++i) {
        QSqlQuery q(m_db);
        q.prepare("SELECT value FROM sensor_data WHERE crop_area_id = :id AND data_type = :type ORDER BY timestamp DESC LIMIT 1");
        q.bindValue(":id", id);
        q.bindValue(":type", i);

        if (q.exec() && q.next()) {
            dataMap[i] = q.value(0).toDouble();
        } else {
            // 如果任何一个传感器数据缺失，则认为数据不完整，返回空Map
            return QMap<int, double>();
        }
    }
    return dataMap;
}

/**
 * @brief 获取并格式化当前环境数据，以供预测模型使用。
 * @return 格式化后的字符串，例如 "25.1 70.2 55.3 Sandy Wheat 40.5 175.2 60.1"。如果数据不完整则返回空字符串。
 */
QString MainWindow::getCurrentEnvironmentData()
{
    QMap<int, double> data = getLatestDataForCurrentArea();
    if (data.isEmpty()) return ""; // 数据不完整

    // 模拟土壤和作物类型，实际应用中这些可能来自数据库或用户输入
    QString soil = "Loamy";
    int areaId = m_cropAreaCombo->currentData().toInt();
    if (areaId >= 200 && areaId < 300) soil = "Sandy";
    else if (areaId >= 300) soil = "Clay";

    QString crop = "Wheat"; // 假设作物类型为小麦

    // 按照模型要求的格式拼接字符串
    return QString("%1 %2 %3 %4 %5 %6 %7 %8")
        .arg(data[0]) // Temperature
        .arg(data[1]) // Air Humidity
        .arg(data[2]) // Soil Moisture
        .arg(soil)    // Soil Type
        .arg(crop)    // Crop Type
        .arg(data[3]) // Nitrogen (N)
        .arg(data[5]) // Potassium (K)
        .arg(data[4]);// Phosphorus (P)
}
