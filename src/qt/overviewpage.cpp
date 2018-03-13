#include "overviewpage.h"
#include "ui_overviewpage.h"
#include "util.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "galaxycashunits.h"
#include "optionsmodel.h"
#include "transactiontablemodel.h"
#include "transactionfilterproxy.h"
#include "guiutil.h"
#include "guiconstants.h"
#include "anonsend.h"
#include "anonsendconfig.h"
#include <QAbstractItemDelegate>
#include <QPainter>

#define DECORATION_SIZE 64
#define NUM_ITEMS 6

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate(): QAbstractItemDelegate(), unit(GalaxyCashUnits::GCH)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        QRect mainRect = option.rect;
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2*ypad)/2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top()+ypad, mainRect.width() - xspace, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight, mainRect.width() - xspace, halfheight);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = option.palette.color(QPalette::Text);
        if(qVariantCanConvert<QColor>(value))
        {
            foreground = qvariant_cast<QColor>(value);
        }

        if (GetBoolArg("-black", false))
            foreground = QColor(120 * 0.6,127 * 0.6,139 * 0.6);

        painter->setPen(foreground);
        painter->drawText(addressRect, Qt::AlignLeft|Qt::AlignVCenter, address);

        if(amount < 0)
        {
            foreground = COLOR_NEGATIVE;
        }
        else if(!confirmed)
        {
            foreground = GetBoolArg("-black", false) ? QColor(120 * 0.6,127 * 0.6,139 * 0.6) : COLOR_UNCONFIRMED;
        }
        else
        {
            foreground = GetBoolArg("-black", false) ? QColor(120,127,139) : option.palette.color(QPalette::Text);
        }
        painter->setPen(foreground);
        QString amountText = GalaxyCashUnits::formatWithUnit(unit, amount, true);
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        painter->setPen(GetBoolArg("-black", false) ? QColor(120,127,139) : option.palette.color(QPalette::Text));
        painter->drawText(amountRect, Qt::AlignLeft|Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }

    int unit;

};
#include "overviewpage.moc"

OverviewPage::OverviewPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverviewPage),
    clientModel(0),
    walletModel(0),
    currentBalance(-1),
    currentStake(0),
    currentUnconfirmedBalance(-1),
    currentImmatureBalance(-1),
    txdelegate(new TxViewDelegate()),
    filter(0)
{
    ui->setupUi(this);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->labelAnonsendSyncStatus->setText("(" + tr("out of sync") + ")");

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));
    fLiteMode = GetBoolArg("-litemode", false);

    if(fLiteMode ){
        ui->frameAnonsend->setVisible(false);
    } else {
        if(fMasterNode){
            ui->toggleAnonsend->setText("(" + tr("Disabled") + ")");
            ui->anonsendAuto->setText("(" + tr("Disabled") + ")");
            ui->anonsendReset->setText("(" + tr("Disabled") + ")");
            ui->frameAnonsend->setEnabled(false);
        } else {
            if(!fEnableAnonsend){
                ui->toggleAnonsend->setText(tr("Start Anonsend Mixing"));
            } else {
                ui->toggleAnonsend->setText(tr("Stop Anonsend Mixing"));
            }
            timer = new QTimer(this);
            connect(timer, SIGNAL(timeout()), this, SLOT(anonSendStatus()));
            if(!GetBoolArg("-reindexaddr", false))
                timer->start(60000);
        }
    }

    // init "out of sync" warning labels
    ui->labelWalletStatus->setText("(" + tr("out of sync") + ")");
    ui->labelTransactionsStatus->setText("(" + tr("out of sync") + ")");

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);
}

void OverviewPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        emit transactionClicked(filter->mapToSource(index));
}

OverviewPage::~OverviewPage()
{
    if(!fLiteMode && !fMasterNode) disconnect(timer, SIGNAL(timeout()), this, SLOT(anonSendStatus()));
    delete ui;
}

void OverviewPage::setBalance(qint64 balance, qint64 stake, qint64 unconfirmedBalance, qint64 immatureBalance, qint64 anonymizedBalance, qint64 lockedBalance)
{
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentStake = stake;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    currentAnonymizedBalance = anonymizedBalance;
    currentLockedBalance = lockedBalance;
    ui->labelBalance->setText(GalaxyCashUnits::formatWithUnit(unit, balance));
    ui->labelAnonymized->setText(GalaxyCashUnits::formatWithUnit(unit, anonymizedBalance));
    ui->labelStake->setText(GalaxyCashUnits::formatWithUnit(unit, stake));
    ui->labelUnconfirmed->setText(GalaxyCashUnits::formatWithUnit(unit, unconfirmedBalance));
    ui->labelImmature->setText(GalaxyCashUnits::formatWithUnit(unit, immatureBalance));
    ui->labelLockedBalance->setText(GalaxyCashUnits::formatWithUnit(unit, lockedBalance));
    ui->labelTotal->setText(GalaxyCashUnits::formatWithUnit(unit, balance + lockedBalance + unconfirmedBalance + immatureBalance));

    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = immatureBalance != 0;
    ui->labelImmature->setVisible(showImmature);
    ui->labelImmatureText->setVisible(showImmature);

    bool showLocked = lockedBalance != 0;
    ui->labelLocked->setVisible(showLocked);
    ui->labelLockedBalance->setVisible(showLocked);
    updateAnonsendProgress();
}

void OverviewPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model)
    {
        // Show warning if this is a prerelease version
        connect(model, SIGNAL(alertsChanged(QString)), this, SLOT(updateAlerts(QString)));
        updateAlerts(model->getStatusBarWarnings());
    }
}

void OverviewPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if(model && model->getOptionsModel())
    {
        // Set up transaction list
        filter = new TransactionFilterProxy();
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Status, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter);
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getStake(), model->getUnconfirmedBalance(), model->getImmatureBalance(), model->getAnonymizedBalance(), model->getLockedBalance());
        connect(model, SIGNAL(balanceChanged(qint64, qint64, qint64, qint64, qint64, qint64)), this, SLOT(setBalance(qint64, qint64, qint64, qint64, qint64, qint64)));
        connect(ui->anonsendAuto, SIGNAL(clicked()), this, SLOT(anonsendAuto()));
        connect(ui->anonsendReset, SIGNAL(clicked()), this, SLOT(anonsendReset()));
        connect(ui->toggleAnonsend, SIGNAL(clicked()), this, SLOT(toggleAnonsend()));
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        connect(model->getOptionsModel(), SIGNAL(transactionFeeChanged(int)), this, SLOT(coinControlUpdateLabels()));
        connect(model->getOptionsModel(), SIGNAL(anonsendRoundsChanged(int)), this, SLOT(anonsendRoundsChanged()));
        connect(model->getOptionsModel(), SIGNAL(anonymizeAmountChanged(int)), this, SLOT(anonsendAmountChanged()));
    }

    // update the display unit, to not use the default ("GCH")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        if(currentBalance != -1)
            setBalance(currentBalance, currentStake, currentUnconfirmedBalance, currentImmatureBalance, currentAnonymizedBalance, pwalletMain->GetLockedBalance());

        // Update txdelegate->unit with the current unit
        txdelegate->unit = walletModel->getOptionsModel()->getDisplayUnit();

        ui->listTransactions->update();
    }
}

void OverviewPage::updateAlerts(const QString &warnings)
{
    this->ui->labelAlerts->setVisible(!warnings.isEmpty());
    this->ui->labelAlerts->setText(warnings);
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelWalletStatus->setVisible(fShow);
    ui->labelAnonsendSyncStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
}
void OverviewPage::updateAnonsendProgress()
{
    if(!anonSendPool.IsBlockchainSynced() || ShutdownRequested()) return;

    if(!pwalletMain) return;

    int unit = walletModel->getOptionsModel()->getDisplayUnit();

    QString strAmountAndRounds;
    QString strAnonymizeAmount = GalaxyCashUnits::formatWithUnit(unit, nAnonymizeAmount * COIN, false);

    if(currentBalance == 0)
    {
        ui->anonsendProgress->setValue(0);
        ui->anonsendProgress->setToolTip(tr("No inputs detected"));
        // when balance is zero just show info from settings
        strAnonymizeAmount = strAnonymizeAmount.remove(strAnonymizeAmount.indexOf("."), GalaxyCashUnits::decimals(unit) + 1);
        strAmountAndRounds = strAnonymizeAmount + " / " + tr("%n Rounds", "", nAnonsendRounds);

        ui->labelAmountRounds->setToolTip(tr("No inputs detected"));
        ui->labelAmountRounds->setText(strAmountAndRounds);
        return;
    }

    int64_t nDenominatedConfirmedBalance;
    int64_t nDenominatedUnconfirmedBalance;
    int64_t nAnonymizableBalance;
    int64_t nNormalizedAnonymizedBalance;
    double nAverageAnonymizedRounds;

    {
        TRY_LOCK(cs_main, lockMain);
        if(!lockMain) return;

        nDenominatedConfirmedBalance = pwalletMain->GetDenominatedBalance();
        nDenominatedUnconfirmedBalance = pwalletMain->GetDenominatedBalance(true);
        nAnonymizableBalance = pwalletMain->GetAnonymizableBalance();
        nNormalizedAnonymizedBalance = pwalletMain->GetNormalizedAnonymizedBalance();
        nAverageAnonymizedRounds = pwalletMain->GetAverageAnonymizedRounds();
    }

    //Get the anon threshold
    int64_t nMaxToAnonymize = nAnonymizableBalance + currentAnonymizedBalance + nDenominatedUnconfirmedBalance;

    // If it's more than the anon threshold, limit to that.
    if(nMaxToAnonymize > nAnonymizeAmount*COIN) nMaxToAnonymize = nAnonymizeAmount*COIN;

    if(nMaxToAnonymize == 0) return;

    if(nMaxToAnonymize >= nAnonymizeAmount * COIN) {
        ui->labelAmountRounds->setToolTip(tr("Found enough compatible inputs to anonymize %1")
                                          .arg(strAnonymizeAmount));
        strAnonymizeAmount = strAnonymizeAmount.remove(strAnonymizeAmount.indexOf("."), GalaxyCashUnits::decimals(unit) + 1);
        strAmountAndRounds = strAnonymizeAmount + " / " + tr("%n Rounds", "", nAnonsendRounds);
    } else {
        QString strMaxToAnonymize = GalaxyCashUnits::formatWithUnit(unit, nMaxToAnonymize, false);
        ui->labelAmountRounds->setToolTip(tr("Not enough compatible inputs to anonymize <span style='color:red;'>%1</span>,<br>"
                                             "will anonymize <span style='color:red;'>%2</span> instead")
                                          .arg(strAnonymizeAmount)
                                          .arg(strMaxToAnonymize));
        strMaxToAnonymize = strMaxToAnonymize.remove(strMaxToAnonymize.indexOf("."), GalaxyCashUnits::decimals(unit) + 1);
        strAmountAndRounds = "<span style='color:red;'>" +
                QString(GalaxyCashUnits::factor(unit) == 1 ? "" : "~") + strMaxToAnonymize +
                " / " + tr("%n Rounds", "", nAnonsendRounds) + "</span>";
    }
    ui->labelAmountRounds->setText(strAmountAndRounds);

    // calculate parts of the progress, each of them shouldn't be higher than 1
    // progress of denominating
    float denomPart = 0;
    // mixing progress of denominated balance
    float anonNormPart = 0;
    // completeness of full amount anonimization
    float anonFullPart = 0;

    int64_t denominatedBalance = nDenominatedConfirmedBalance + nDenominatedUnconfirmedBalance;
    denomPart = (float)denominatedBalance / nMaxToAnonymize;
    denomPart = denomPart > 1 ? 1 : denomPart;
    denomPart *= 100;

    anonNormPart = (float)nNormalizedAnonymizedBalance / nMaxToAnonymize;
    anonNormPart = anonNormPart > 1 ? 1 : anonNormPart;
    anonNormPart *= 100;

    anonFullPart = (float)currentAnonymizedBalance / nMaxToAnonymize;
    anonFullPart = anonFullPart > 1 ? 1 : anonFullPart;
    anonFullPart *= 100;

    // apply some weights to them ...
    float denomWeight = 1;
    float anonNormWeight = nAnonsendRounds;
    float anonFullWeight = 2;
    float fullWeight = denomWeight + anonNormWeight + anonFullWeight;
    // ... and calculate the whole progress
    float denomPartCalc = ceilf((denomPart * denomWeight / fullWeight) * 100) / 100;
    float anonNormPartCalc = ceilf((anonNormPart * anonNormWeight / fullWeight) * 100) / 100;
    float anonFullPartCalc = ceilf((anonFullPart * anonFullWeight / fullWeight) * 100) / 100;
    float progress = denomPartCalc + anonNormPartCalc + anonFullPartCalc;
    if(progress >= 100) progress = 100;

    ui->anonsendProgress->setValue(progress);

    QString strToolPip = ("<b>" + tr("Overall progress") + ": %1%</b><br/>" +
                          tr("Denominated") + ": %2%<br/>" +
                          tr("Mixed") + ": %3%<br/>" +
                          tr("Anonymized") + ": %4%<br/>" +
                          tr("Denominated inputs have %5 of %n rounds on average", "", nAnonsendRounds))
            .arg(progress).arg(denomPart).arg(anonNormPart).arg(anonFullPart)
            .arg(nAverageAnonymizedRounds);
    ui->anonsendProgress->setToolTip(strToolPip);
}


void OverviewPage::anonSendStatus()
{
    static int64_t nLastDSProgressBlockTime = 0;

    int nBestHeight = pindexBest->nHeight;

    // we we're processing more then 1 block per second, we'll just leave
    if(((nBestHeight - anonSendPool.cachedNumBlocks) / (GetTimeMillis() - nLastDSProgressBlockTime + 1) > 1)) return;
    nLastDSProgressBlockTime = GetTimeMillis();

    if(!fEnableAnonsend) {
        if(nBestHeight != anonSendPool.cachedNumBlocks)
        {
            anonSendPool.cachedNumBlocks = nBestHeight;
            updateAnonsendProgress();

            ui->anonsendEnabled->setText(tr("Disabled"));
            ui->anonsendStatus->setText("");
            ui->toggleAnonsend->setText(tr("Start Anonsend Mixing"));
        }

        return;
    }

    // check anonsend status and unlock if needed
    if(nBestHeight != anonSendPool.cachedNumBlocks)
    {
        // Balance and number of transactions might have changed
        anonSendPool.cachedNumBlocks = nBestHeight;

        updateAnonsendProgress();

        ui->anonsendEnabled->setText(tr("Enabled"));
    }

    QString strStatus = QString(anonSendPool.GetStatus().c_str());

    QString s = tr("Last Anonsend message:\n") + strStatus;

    if(s != ui->anonsendStatus->text())
        LogPrintf("Last Anonsend message: %s\n", strStatus.toStdString());

    ui->anonsendStatus->setText(s);

    if(anonSendPool.sessionDenom == 0){
        ui->labelSubmittedDenom->setText(tr("N/A"));
    } else {
        std::string out;
        anonSendPool.GetDenominationsToString(anonSendPool.sessionDenom, out);
        QString s2(out.c_str());
        ui->labelSubmittedDenom->setText(s2);
    }

    // Get AnonSend Denomination Status
}

void OverviewPage::anonsendRoundsChanged(){
    QSettings settings;
    nAnonsendRounds = settings.value("nAnonsendRounds").toInt();
    anonSendPool.Reset();
    anonSendStatus();
}

void OverviewPage::anonsendAmountChanged(){
    QSettings settings;
    nAnonymizeAmount = settings.value("nAnonymizeAmount").toInt();
    anonSendPool.Reset();
    anonSendStatus();
}

void OverviewPage::anonsendAuto(){
    anonSendPool.DoAutomaticDenominating();
}

void OverviewPage::anonsendReset(){
    anonSendPool.Reset();
    anonSendStatus();

    QMessageBox::warning(this, tr("Anonsend"),
        tr("Anonsend was successfully reset."),
        QMessageBox::Ok, QMessageBox::Ok);
}

void OverviewPage::toggleAnonsend(){

    QSettings settings;
    // Popup some information on first mixing
    QString hasMixed = settings.value("hasMixed").toString();
    if(hasMixed.isEmpty()){
        QMessageBox::information(this, tr("Anonsend"),
                tr("If you don't want to see internal Anonsend fees/transactions select \"Received By\" as Type on the \"Transactions\" tab."),
                QMessageBox::Ok, QMessageBox::Ok);
        settings.setValue("hasMixed", "hasMixed");
    }
    if(!fEnableAnonsend){
        int64_t balance = currentBalance;
        float minAmount = 1.49 * COIN;
        if(balance < minAmount){
            QString strMinAmount(GalaxyCashUnits::formatWithUnit(nDisplayUnit, minAmount));
            QMessageBox::warning(this, tr("Anonsend"),
                tr("Anonsend requires at least %1 to use.").arg(strMinAmount),
                QMessageBox::Ok, QMessageBox::Ok);
            return;
        }

        // if wallet is locked, ask for a passphrase
        if (walletModel->getEncryptionStatus() == WalletModel::Locked)
        {
            WalletModel::UnlockContext ctx(walletModel->requestUnlock());
            if(!ctx.isValid())
            {
                //unlock was cancelled
                anonSendPool.cachedNumBlocks = std::numeric_limits<int>::max();
                QMessageBox::warning(this, tr("Anonsend"),
                    tr("Wallet is locked and user declined to unlock. Disabling Anonsend."),
                    QMessageBox::Ok, QMessageBox::Ok);
                if (fDebug) LogPrintf("Wallet is locked and user declined to unlock. Disabling Anonsend.\n");
                return;
            }
        }

    }

    fEnableAnonsend = !fEnableAnonsend;
    anonSendPool.cachedNumBlocks = std::numeric_limits<int>::max();

    if(!fEnableAnonsend){
        ui->toggleAnonsend->setText(tr("Start Anonsend Mixing"));
        anonSendPool.UnlockCoins();
    } else {
        ui->toggleAnonsend->setText(tr("Stop Anonsend Mixing"));

        /* show anonsend configuration if client has defaults set */

        if(nAnonymizeAmount == 0){
            AnonsendConfig dlg(this);
            dlg.setModel(walletModel);
            dlg.exec();
        }

    }
}

