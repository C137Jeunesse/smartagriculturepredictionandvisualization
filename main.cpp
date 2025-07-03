#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <exception>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFont font("Microsoft YaHei", 9);
    a.setFont(font);

    try {
        MainWindow w;
        w.show();
        return a.exec();
    } catch (const std::exception &e) {
        QMessageBox::critical(nullptr, "致命错误", QString("程序启动时发生未处理的异常: %1").arg(e.what()));
        return -1;
    } catch (...) {
        QMessageBox::critical(nullptr, "致命错误", "程序启动时发生未知的致命错误。");
        return -1;
    }
}
