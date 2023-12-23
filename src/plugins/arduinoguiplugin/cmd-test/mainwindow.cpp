#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_plugin_dialog = new CheckArduinoPluginInitialization(this);
    m_plugin_dialog->show();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_plugin_dialog;
}

