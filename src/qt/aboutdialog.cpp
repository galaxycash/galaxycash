#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include "clientmodel.h"
#include "util.h"
#include <QApplication>
#include <QMenuBar>
#include <QMenu>
#include <QIcon>
#include <QVBoxLayout>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressBar>
#include <QStackedWidget>
#include <QDateTime>
#include <QMovie>
#include <QFileDialog>
#include <QDesktopServices>
#include <QTimer>
#include <QDragEnterEvent>
#include <QUrl>
#include <QMimeData>
#include <QStyle>
#include <QPushButton>

#include "version.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    if (GetArg("-ui", "default") == "dark") {

        // Load an application style
        QFile styleFile( ":styles/dark");
        styleFile.open( QFile::ReadOnly );

        // Apply the loaded stylesheet
        QString style( styleFile.readAll() );
        this->setStyleSheet( style );

    }  else
        this->setStyleSheet("QToolBar > QToolButton { Alignment: left;background-color: #ffffff;selection-background-color:transparent;border-left: 0px;border-right: 0px;border-top: 0px;min-height:2.5em;padding: 0em 0em;font-size: 11px;color:rgb(43, 43, 43);width: 82px;height: 32px;}QToolBar > QToolButton:checked {background-color: rgb(142, 142, 142);color:#fff;}QToolBar > QToolButton:hover {background-color: rgb(138, 138, 138);color:#fff;}QToolBar > QToolButton#ToolbarSpacer:hover:!selected {background-color: rgb(110, 110, 110);}QToolBar > QToolButton:pressed {background-color: qlineargradient(x1:0, x2: 1, stop: 0 #00a300, stop: 0.02 #00a300, stop: 0.0201 rgb(105, 105, 105), stop: 1 rgb(105, 105, 105));color:#fff;}"
                        "QProgressBar {color: #AAAAAA;border:0px solid grey;border-radius:5px;background-color:transparent;padding-left:0px;padding-right:0px;} QProgressBar::chunk {background-color:rgb(72, 72, 72);width: 20px;} QLabel#progressBarLabel {background-color:transparent;color: rgb(64, 64, 64);padding-left:5px;padding-right:5px;}"
                        "QPushButton {background-color: rgb(72, 72, 72); border-width: 1px;border-style: outset;border-color: rgb(64, 64, 64);border-radius: 2px;color:#ffffff;font-size:12px;font-weight:bold;padding-left:25px;padding-right:25px;padding-top:1px;padding-bottom:1px;height: 26px;margin: 2px;} QPushButton:hover {background-color: qlineargradient(y1:0, y2: 1, stop: 0 #ffffff, stop: 0.1 #ffffff, stop: 0.101 rgb(74, 74, 74), stop: 0.9 rgb(74, 74, 74), stop: 0.901 #ffffff, stop: 1 #ffffff);} QPushButton:pressed {background-color: rgb(72, 72, 72);border:1px solid #000000;background-color: qlineargradient(y1:0, y2: 1, stop: 0 #00ff00, stop: 0.1 #00ff00, stop: 0.101 rgb(74, 74, 74), stop: 0.9 rgb(74, 74, 74), stop: 0.901 #00ff00, stop: 1 #00ff00);}");

}

void AboutDialog::setModel(ClientModel *model)
{
    if(model)
    {
        ui->versionLabel->setText(model->formatFullVersion());
    }
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::on_buttonBox_accepted()
{
    close();
}
