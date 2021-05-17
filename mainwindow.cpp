#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "BalloonTip.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{
    BalloonTip::showBalloon(QMessageBox::Warning, "Test", "Hello World", QCursor::pos(), 10000, true, ui->spinBox->value());
}
