
#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <exception>
#include <vector> // Required for std::vector

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setFont(QFont("Microsoft YaHei", 9));

    try {
        // **MODIFIED**: Create a sample vector of crop IDs for testing.
        // In your main application, you would get this from the logged-in user.
        std::vector<int> userCropIds = {1};

        MainWindow w(userCropIds); // Pass the vector to the constructor

        if (w.initialize()) {
            w.show();
            return a.exec();
        } else {
            QMessageBox::critical(nullptr, "初始化失败", "应用程序无法启动，请检查数据库连接和配置文件。");
            return -1;
        }
    } catch (const std::exception &e) {
        QMessageBox::critical(nullptr, "致命错误", QString("程序启动时发生未处理的异常: %1").arg(e.what()));
        return -1;
    } catch (...) {
        QMessageBox::critical(nullptr, "致命错误", "程序启动时发生未知的致命错误。");
        return -1;
    }
}
