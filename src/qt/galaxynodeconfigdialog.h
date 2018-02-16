#ifndef ADRENALINENODECONFIGDIALOG_H
#define ADRENALINENODECONFIGDIALOG_H

#include <QDialog>

namespace Ui {
    class GalaxyNodeConfigDialog;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Dialog showing transaction details. */
class GalaxyNodeConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GalaxyNodeConfigDialog(QWidget *parent = 0, QString nodeAddress = "123.456.789.123:51315", QString privkey="MASTERNODEPRIVKEY");
    ~GalaxyNodeConfigDialog();

private:
    Ui::GalaxyNodeConfigDialog *ui;
};

#endif // ADRENALINENODECONFIGDIALOG_H
