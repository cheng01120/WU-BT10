#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QDateTime>

#include "bleinterface.h"
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

		static QTextEdit* teLog;

private slots:
    void on_servicesComboBox_currentIndexChanged(int index);

private:
    Ui::MainWindow *ui;
    BLEInterface *m_bleInterface;
    void dataReceived(QByteArray data);
};

#endif // MAINWINDOW_H
