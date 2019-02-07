#include "masternodemanager.h"
#include "ui_masternodemanager.h"
#include "addeditgalaxynode.h"
#include "galaxynodeconfigdialog.h"

#include "sync.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "activemasternode.h"
#include "masternodeconfig.h"
#include "masternodeman.h"
#include "masternode.h"
#include "walletdb.h"
#include "wallet.h"
#include "init.h"
#include "rpcserver.h"
#include <boost/lexical_cast.hpp>
#include <fstream>
using namespace json_spirit;
using namespace std;

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QTimer>
#include <QDebug>
#include <QScrollArea>
#include <QScroller>
#include <QDateTime>
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>

MasternodeManager::MasternodeManager(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MasternodeManager),
    clientModel(0),
    walletModel(0)
{
    ui->setupUi(this);

    ui->editButton->setEnabled(false);
    ui->startButton->setEnabled(false);

    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableWidget_2->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateNodeList()));
    if(!GetBoolArg("-reindexaddr", false))
        timer->start(30000);

    updateNodeList();
}

MasternodeManager::~MasternodeManager()
{
    delete ui;
}

void MasternodeManager::on_tableWidget_2_itemSelectionChanged()
{
    if(ui->tableWidget_2->selectedItems().count() > 0)
    {
        ui->editButton->setEnabled(true);
        ui->startButton->setEnabled(true);
    }
}

void MasternodeManager::updateGalaxyNode(QString alias, QString addr, QString privkey, QString txHash, QString txIndex, QString status)
{
    LOCK(cs_adrenaline);
    bool bFound = false;
    int nodeRow = 0;
    for(int i=0; i < ui->tableWidget_2->rowCount(); i++)
    {
        if(ui->tableWidget_2->item(i, 0)->text() == alias)
        {
            bFound = true;
            nodeRow = i;
            break;
        }
    }

    if(nodeRow == 0 && !bFound)
        ui->tableWidget_2->insertRow(0);

    QTableWidgetItem *aliasItem = new QTableWidgetItem(alias);
    QTableWidgetItem *addrItem = new QTableWidgetItem(addr);
    QTableWidgetItem *statusItem = new QTableWidgetItem(status);

    ui->tableWidget_2->setItem(nodeRow, 0, aliasItem);
    ui->tableWidget_2->setItem(nodeRow, 1, addrItem);
    ui->tableWidget_2->setItem(nodeRow, 2, statusItem);
}

static QString seconds_to_DHMS(quint32 duration)
{
  QString res;
  int seconds = (int) (duration % 60);
  duration /= 60;
  int minutes = (int) (duration % 60);
  duration /= 60;
  int hours = (int) (duration % 24);
  int days = (int) (duration / 24);
  if((hours == 0)&&(days == 0))
      return res.sprintf("%02dm:%02ds", minutes, seconds);
  if (days == 0)
      return res.sprintf("%02dh:%02dm:%02ds", hours, minutes, seconds);
  return res.sprintf("%dd %02dh:%02dm:%02ds", days, hours, minutes, seconds);
}

#define MY_MASTERNODELIST_UPDATE_SECONDS 60

uint32_t GetBlocksPerHourItr(CBlockIndex *pbest, uint32_t iterations) {
    uint32_t blocks = 1;

    time_t t = pbest->nTime;
    struct tm *tm = localtime(&t);
    uint32_t hour = tm->tm_hour;

    CBlockIndex *pindex = pbest;
    while (pindex) {
        time_t t = pindex->nTime;
        struct tm *tm = localtime(&t);
        if (tm->tm_hour != hour) {
            blocks = 1;
            hour = tm->tm_hour;
            pindex = pindex->pprev;
            break;
        }
        pindex = pindex->pprev;
    }

    while (pindex) {
        time_t t = pindex->nTime;
        struct tm *tm = localtime(&t);
        uint32_t h = tm->tm_hour;
        if (tm->tm_hour != hour)
            break;
        blocks++;
        pindex = pindex->pprev;
    }


    if (iterations >= 1) {
        uint32_t iteration = GetBlocksPerHourItr(pindex, iterations - 1);
        return iteration > blocks ? iteration : blocks;
    }
    return blocks;
}

uint32_t GetBlocksPerHour() {
    static uint32_t bestBlocks = 0;

    uint32_t blocks = GetBlocksPerHourItr(pindexBest, 8);
    if (blocks > bestBlocks)
        bestBlocks = blocks;

    return bestBlocks;
}

uint32_t GetBlocksPerDay() {
    return GetBlocksPerHour() * 24;
}

void MasternodeManager::updateNodeList()
{
    TRY_LOCK(cs_masternodes, lockMasternodes);
    if(!lockMasternodes)
        return;

    if (!ui->tableWidget->isVisible())
        return;

    ui->countLabel->setText("Updating...");
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);
    std::vector<CMasternode> vMasternodes = mnodeman.GetFullMasternodeVector();
    BOOST_FOREACH(CMasternode& mn, vMasternodes)
    {
        int mnRow = 0;
        ui->tableWidget->insertRow(0);

        // populate list
        // Address, Rank, Active, Active Seconds, Last Seen, Pub Key
        QTableWidgetItem *activeItem = new QTableWidgetItem(QString::number(mn.IsEnabled()));
        QTableWidgetItem *addressItem = new QTableWidgetItem(QString::fromStdString(mn.addr.ToString()));
        QString Rank = QString::number(mnodeman.GetMasternodeRank(mn.vin, pindexBest->nHeight));
        QTableWidgetItem *rankItem = new QTableWidgetItem(Rank.rightJustified(2, '0', false));
        QTableWidgetItem *activeSecondsItem = new QTableWidgetItem(seconds_to_DHMS((qint64)(mn.lastTimeSeen - mn.sigTime)));
        QTableWidgetItem *lastSeenItem = new QTableWidgetItem(QString::fromStdString(DateTimeStrFormat(mn.lastTimeSeen)));

        CScript pubkey;
        pubkey =GetScriptForDestination(mn.pubkey.GetID());
        CTxDestination address1;
        ExtractDestination(pubkey, address1);
        CGalaxyCashAddress address2(address1);
        QTableWidgetItem *pubkeyItem = new QTableWidgetItem(QString::fromStdString(address2.ToString()));

        ui->tableWidget->setItem(mnRow, 0, addressItem);
        ui->tableWidget->setItem(mnRow, 1, rankItem);
        ui->tableWidget->setItem(mnRow, 2, activeItem);
        ui->tableWidget->setItem(mnRow, 3, activeSecondsItem);
        ui->tableWidget->setItem(mnRow, 4, lastSeenItem);
        ui->tableWidget->setItem(mnRow, 5, pubkeyItem);
    }

    ui->countLabel->setText(QString::number(ui->tableWidget->rowCount()));
    on_UpdateButton_clicked();

    uint32_t nodes = mnodeman.CountEnabled() ? mnodeman.CountEnabled() : 1;
    uint32_t blocksHour = GetBlocksPerHour();
    uint32_t blocks = blocksHour * 24;
    uint32_t blocksPerNode = blocks / nodes;
    double dailyIncome = blocksPerNode * (double)(GetMNProofOfStakeReward(GetProofOfStakeReward(pindexBest->pprev, 0, 0), pindexBest->nHeight) / COIN);
    double ROI = (dailyIncome / (double) MasternodeCollateral(pindexBest->nHeight)) * 100.0;


    std::string roi;

    std::stringstream val0, val1, val2, val3, val4, val5;

    val0 << blocksHour;
    roi += std::string("Blocks per hour: ") + val0.str() + std::string(" blocks\n");

    val1 << blocks;
    roi += std::string("Blocks per day: ") + val1.str() + std::string(" blocks\n");

    val2 << std::fixed << setprecision(6) << dailyIncome;
    roi += std::string("Daily income: ") + val2.str() + std::string(" TGCH\n");

    val3 << std::fixed << setprecision(2) << ROI;
    roi += std::string("ROI: Daily ") + val3.str() + std::string("% ");

    val4 << std::fixed << setprecision(2) << (ROI * 30);
    roi += std::string("Monthly ") + val4.str() + std::string("% ");


    val5 << std::fixed << setprecision(2) << (ROI * 30 * 12);
    roi += std::string("Yearly ") + val5.str() + std::string("%");


    ui->roi->setText(QString::fromStdString(roi));
}


void MasternodeManager::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model)
    {
    }
}

void MasternodeManager::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if(model && model->getOptionsModel())
    {
    }

}

void MasternodeManager::on_createButton_clicked()
{
    AddEditGalaxyNode* aenode = new AddEditGalaxyNode();
    aenode->exec();
}

void MasternodeManager::on_startButton_clicked()
{
    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if(!ctx.isValid())
        return;

    // start the node
    QItemSelectionModel* selectionModel = ui->tableWidget_2->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    if(selected.count() == 0)
        return;

    QModelIndex index = selected.at(0);
    int r = index.row();
    std::string sAlias = ui->tableWidget_2->item(r, 0)->text().toStdString();

    std::string statusObj;
    statusObj += "<center>Alias: " + sAlias;

    BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
        if(mne.getAlias() == sAlias) {
            std::string errorMessage;
            std::string strDonateAddress = "";
            std::string strDonationPercentage = "";

            bool result = activeMasternode.Register(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), strDonateAddress, strDonationPercentage, errorMessage);

            if(result) {
                statusObj += "<br>Successfully started masternode." ;
            } else {
                statusObj += "<br>Failed to start masternode.<br>Error: " + errorMessage;
            }
            break;
        }
    }
    statusObj += "</center>";

    QMessageBox msg;
    msg.setText(QString::fromStdString(statusObj));

    msg.exec();
}

void MasternodeManager::on_startAllButton_clicked()
{   
    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if(!ctx.isValid())
        return;

    std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;

    int total = 0;
    int successful = 0;
    int fail = 0;
    std::string statusObj;

    BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
        total++;

        std::string errorMessage;
        std::string strDonateAddress = "";
        std::string strDonationPercentage = "";

        bool result = activeMasternode.Register(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), strDonateAddress, strDonationPercentage, errorMessage);

        if(result) {
            successful++;
        } else {
            fail++;
            statusObj += "\nFailed to start " + mne.getAlias() + ". Error: " + errorMessage;
        }
    }

    std::string returnObj;
    returnObj = "Successfully started " + boost::lexical_cast<std::string>(successful) + " masternodes, failed to start " +
            boost::lexical_cast<std::string>(fail) + ", total " + boost::lexical_cast<std::string>(total);
    if (fail > 0)
        returnObj += statusObj;

    QMessageBox msg;
    msg.setText(QString::fromStdString(returnObj));
    msg.exec();
}

void MasternodeManager::on_UpdateButton_clicked()
{
    BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
        std::string errorMessage;
        std::string strDonateAddress = "";
        std::string strDonationPercentage = "";

        std::vector<CMasternode> vMasternodes = mnodeman.GetFullMasternodeVector();
        if (errorMessage == ""){
            updateGalaxyNode(QString::fromStdString(mne.getAlias()), QString::fromStdString(mne.getIp()), QString::fromStdString(mne.getPrivKey()), QString::fromStdString(mne.getTxHash()),
                QString::fromStdString(mne.getOutputIndex()), QString::fromStdString("Not in the masternode list."));
        }
        else {
            updateGalaxyNode(QString::fromStdString(mne.getAlias()), QString::fromStdString(mne.getIp()), QString::fromStdString(mne.getPrivKey()), QString::fromStdString(mne.getTxHash()),
                QString::fromStdString(mne.getOutputIndex()), QString::fromStdString(errorMessage));
        }

        BOOST_FOREACH(CMasternode& mn, vMasternodes) {
            if (mn.addr.ToString().c_str() == mne.getIp()){
                updateGalaxyNode(QString::fromStdString(mne.getAlias()), QString::fromStdString(mne.getIp()), QString::fromStdString(mne.getPrivKey()), QString::fromStdString(mne.getTxHash()),
                QString::fromStdString(mne.getOutputIndex()), QString::fromStdString("Masternode is Running."));
            }
        }
    }
}
