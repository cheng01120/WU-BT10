#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QStatusBar>

QTextEdit* MainWindow::teLog = 0;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
		teLog = ui->receivedTextEdit;
    m_bleInterface = new BLEInterface(this);

    connect(m_bleInterface, &BLEInterface::dataReceived,
            this, &MainWindow::dataReceived);
    connect(m_bleInterface, &BLEInterface::devicesNamesChanged,
            [this] (QStringList devices){
        ui->devicesComboBox->clear();
        ui->devicesComboBox->addItems(devices);
    });
    connect(m_bleInterface, &BLEInterface::servicesChanged,
            [this] (QStringList services){
        ui->servicesComboBox->clear();
        ui->servicesComboBox->addItems(services);
    });
    connect(m_bleInterface, &BLEInterface::statusInfoChanged,
            [this](QString info, bool){
        this->statusBar()->showMessage(info);
    });

		setWindowTitle("WU-BT10");
		ui->servicesComboBox->setEnabled(false);
    m_bleInterface->scanDevices();
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::dataReceived(QByteArray data){
		ui->receivedTextEdit->append(data.toHex(':').toUpper());
}

void MainWindow::on_servicesComboBox_currentIndexChanged(int index)
{
    m_bleInterface->setCurrentService(index);
}
