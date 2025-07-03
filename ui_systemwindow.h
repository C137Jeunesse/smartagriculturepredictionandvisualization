/********************************************************************************
** Form generated from reading UI file 'systemwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.5.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SYSTEMWINDOW_H
#define UI_SYSTEMWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SystemWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QPushButton *btnSystemSuggestion;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *SystemWindow)
    {
        if (SystemWindow->objectName().isEmpty())
            SystemWindow->setObjectName("SystemWindow");
        SystemWindow->resize(800, 600);
        centralwidget = new QWidget(SystemWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName("verticalLayout");
        btnSystemSuggestion = new QPushButton(centralwidget);
        btnSystemSuggestion->setObjectName("btnSystemSuggestion");

        verticalLayout->addWidget(btnSystemSuggestion);

        SystemWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(SystemWindow);
        menubar->setObjectName("menubar");
        SystemWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(SystemWindow);
        statusbar->setObjectName("statusbar");
        SystemWindow->setStatusBar(statusbar);

        retranslateUi(SystemWindow);

        QMetaObject::connectSlotsByName(SystemWindow);
    } // setupUi

    void retranslateUi(QMainWindow *SystemWindow)
    {
        SystemWindow->setWindowTitle(QCoreApplication::translate("SystemWindow", "\347\263\273\347\273\237\346\231\272\350\203\275\345\273\272\350\256\256", nullptr));
        btnSystemSuggestion->setText(QCoreApplication::translate("SystemWindow", "\347\263\273\347\273\237\346\231\272\350\203\275\345\273\272\350\256\256", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SystemWindow: public Ui_SystemWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SYSTEMWINDOW_H
