/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.7.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralWidget;
    QGridLayout *gridLayout;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label;
    QComboBox *devicesComboBox;
    QHBoxLayout *horizontalLayout;
    QLabel *label_2;
    QComboBox *servicesComboBox;
    QTextEdit *receivedTextEdit;
    QPushButton *pushButton;
    QStatusBar *statusBar;
    QButtonGroup *buttonGroup;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(517, 436);
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName("centralWidget");
        gridLayout = new QGridLayout(centralWidget);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName("gridLayout");
        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(6);
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        label = new QLabel(centralWidget);
        label->setObjectName("label");

        horizontalLayout_3->addWidget(label);

        devicesComboBox = new QComboBox(centralWidget);
        devicesComboBox->setObjectName("devicesComboBox");

        horizontalLayout_3->addWidget(devicesComboBox);

        horizontalLayout_3->setStretch(1, 1);

        gridLayout->addLayout(horizontalLayout_3, 0, 0, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName("horizontalLayout");
        label_2 = new QLabel(centralWidget);
        label_2->setObjectName("label_2");

        horizontalLayout->addWidget(label_2);

        servicesComboBox = new QComboBox(centralWidget);
        servicesComboBox->setObjectName("servicesComboBox");

        horizontalLayout->addWidget(servicesComboBox);

        horizontalLayout->setStretch(1, 1);

        gridLayout->addLayout(horizontalLayout, 1, 0, 1, 1);

        receivedTextEdit = new QTextEdit(centralWidget);
        receivedTextEdit->setObjectName("receivedTextEdit");
        receivedTextEdit->setReadOnly(true);

        gridLayout->addWidget(receivedTextEdit, 5, 0, 1, 1);

        pushButton = new QPushButton(centralWidget);
        pushButton->setObjectName("pushButton");

        gridLayout->addWidget(pushButton, 6, 0, 1, 1);

        MainWindow->setCentralWidget(centralWidget);
        statusBar = new QStatusBar(MainWindow);
        statusBar->setObjectName("statusBar");
        MainWindow->setStatusBar(statusBar);

        retranslateUi(MainWindow);
        QObject::connect(pushButton, &QPushButton::clicked, receivedTextEdit, qOverload<>(&QTextEdit::clear));

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "Device", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "Service", nullptr));
        pushButton->setText(QCoreApplication::translate("MainWindow", "Clear", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
