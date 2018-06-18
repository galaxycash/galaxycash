#include "overviewpage.h"
#include "ui_overviewpage.h"
#include "util.h"
#include "init.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "galaxycashunits.h"
#include "optionsmodel.h"
#include "transactiontablemodel.h"
#include "transactionfilterproxy.h"
#include "guiutil.h"
#include "guiconstants.h"
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

        painter->setPen(foreground);
        painter->drawText(addressRect, Qt::AlignLeft|Qt::AlignVCenter, address);

        if(amount < 0)
        {
            foreground = COLOR_NEGATIVE;
        }
        else if(!confirmed)
        {
            foreground = COLOR_UNCONFIRMED;
        }
        else
        {
            foreground = option.palette.color(QPalette::Text);
        }
        painter->setPen(foreground);
        QString amountText = GalaxyCashUnits::formatWithUnit(unit, amount, true);
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        painter->setPen(option.palette.color(QPalette::Text));
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
    currentLockedBalance(0),
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

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));


    QTimer *timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(updatePrices()));

    timer->start(10000);


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
    delete ui;
}

void OverviewPage::setBalance(qint64 balance, qint64 stake, qint64 unconfirmedBalance, qint64 immatureBalance, qint64 lockedBalance)
{
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentStake = stake;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    currentLockedBalance = lockedBalance;
    ui->labelBalance->setText(GalaxyCashUnits::formatWithUnit(unit, balance));
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
        setBalance(model->getBalance(), model->getStake(), model->getUnconfirmedBalance(), model->getImmatureBalance(), model->getLockedBalance());
        connect(model, SIGNAL(balanceChanged(qint64, qint64, qint64, qint64, qint64)), this, SLOT(setBalance(qint64, qint64, qint64, qint64, qint64)));
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        connect(model->getOptionsModel(), SIGNAL(transactionFeeChanged(qint64)), this, SLOT(coinControlUpdateLabels()));
    }

    // update the display unit, to not use the default ("GCH")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        if(currentBalance != -1)
            setBalance(currentBalance, currentStake, currentUnconfirmedBalance, currentImmatureBalance, currentLockedBalance);

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
    ui->labelTransactionsStatus->setVisible(fShow);
}

#include <QtNetwork>

QJsonDocument callGetJson(const char *url, QWidget *parent) {
    QNetworkRequest req;
    req.setUrl(QUrl(url));

    QNetworkAccessManager nam(parent);
    QEventLoop loop;
    nam.connect(&nam, &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);
    QNetworkReply *reply = nam.get(req);
    loop.exec();

    QByteArray response_data = reply->readAll();
    QJsonDocument response = QJsonDocument::fromJson(response_data);
    reply->deleteLater();
    return response;
}

QJsonDocument callJson(const char *url, QWidget *parent) {
    QNetworkRequest req;
    req.setUrl(QUrl(url));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));

    QString header = "{}";
    req.setHeader(QNetworkRequest::ContentLengthHeader,QByteArray::number(header.size()));

    QNetworkAccessManager nam(parent);
    QEventLoop loop;
    nam.connect(&nam, &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);
    QNetworkReply *reply = nam.post(req, header.toUtf8());
    loop.exec();

    QByteArray response_data = reply->readAll();
    QJsonDocument response = QJsonDocument::fromJson(response_data);
    reply->deleteLater();
    return response;
}
void OverviewPage::updatePrices()
{
    const char *cryptohubStats = "https://cryptohub.online/api/market/ticker/GCH/";
    const char *cryptohubBTCETH = "https://cryptohub.online/api/market/ticker/ETH/";
    const char *crexUSDBTC = "https://api.crex24.com/CryptoExchangeService/BotPublic/ReturnTicker?request=[NamePairs=USD_BTC]";
    const char *crexEURBTC = "https://api.crex24.com/CryptoExchangeService/BotPublic/ReturnTicker?request=[NamePairs=EUR_BTC]";
    const char *crexRUBBTC = "https://api.crex24.com/CryptoExchangeService/BotPublic/ReturnTicker?request=[NamePairs=RUB_BTC]";
    const char *crexCNYBTC = "https://api.crex24.com/CryptoExchangeService/BotPublic/ReturnTicker?request=[NamePairs=CNY_BTC]";
    const char *crexBTCGCH = "https://api.crex24.com/CryptoExchangeService/BotPublic/ReturnTicker?request=[NamePairs=BTC_GCH]";
    const char *crexBTCETH = "https://api.crex24.com/CryptoExchangeService/BotPublic/ReturnTicker?request=[NamePairs=BTC_ETH]";
    std::string stats;


    double GCH_BTC = 0.0;
    double GCH_ETH = 0.0;
    double GCH_USD = 0.0;
    double GCH_EUR = 0.0;
    double GCH_RUB = 0.0;
    double GCH_CNY = 0.0;
    double BTC_USD = 0.0;
    double BTC_EUR = 0.0;
    double BTC_RUB = 0.0;
    double BTC_CNY = 0.0;
    double ETH_BTC = 0.0;
    double BTC_VOL = 0.0;
    double ETH_VOL = 0.0;

#if defined(_WIN32) || defined(WIN32)
    QJsonDocument res = callGetJson(crexUSDBTC, this);

    res = callGetJson(crexUSDBTC, this);
    if (!res.isEmpty()) {
        const QJsonValue val = res["Tickers"][0];
        if (val["Last"].toDouble() > BTC_USD)
            BTC_USD = val["Last"].toDouble();
    }

    res = callGetJson(crexRUBBTC, this);
    if (!res.isEmpty()) {
        const QJsonValue val = res["Tickers"][0];
        if (val["Last"].toDouble() > BTC_RUB)
            BTC_RUB = val["Last"].toDouble();
    }

    res = callGetJson(crexEURBTC, this);
    if (!res.isEmpty()) {
        const QJsonValue val = res["Tickers"][0];
        if (val["Last"].toDouble() > BTC_EUR)
            BTC_EUR = val["Last"].toDouble();
    }

    res = callGetJson(crexCNYBTC, this);
    if (!res.isEmpty()) {
        const QJsonValue val = res["Tickers"][0];
        if (val["Last"].toDouble() > BTC_CNY)
            BTC_CNY = val["Last"].toDouble();
    }

    res = callGetJson(crexBTCETH, this);
    if (!res.isEmpty()) {
        const QJsonValue val = res["Tickers"][0];
        if (val["Last"].toDouble() > ETH_BTC)
            ETH_BTC = val["Last"].toDouble();
    }

    res = callGetJson(cryptohubBTCETH, this);
    if (!res.isEmpty()) {
        const QJsonValue btc = res["BTC_ETH"];
        if (btc["last"].toDouble() > ETH_BTC)
            ETH_BTC = btc["last"].toDouble();
    }

    res = callGetJson(cryptohubStats, this);
    if (!res.isEmpty()) {
        const QJsonValue btc = res["BTC_GCH"];
        if (btc["last"].toDouble() > GCH_BTC)
            GCH_BTC = btc["last"].toDouble();

        BTC_VOL += btc["baseVolume"].toDouble();

        const QJsonValue eth = res["ETH_GCH"];
        if (eth["last"].toDouble() > GCH_ETH)
            GCH_ETH = eth["last"].toDouble();

        ETH_VOL += eth["baseVolume"].toDouble();
    }

    res = callGetJson(crexBTCGCH, this);
    if (!res.isEmpty()) {
        const QJsonValue btc = res["Tickers"][0];
        if (btc["Last"].toDouble() > GCH_BTC)
            GCH_BTC = btc["Last"].toDouble();

        BTC_VOL += btc["VolumeInBtc"].toDouble();
    }

#endif

    if (GCH_ETH * ETH_BTC > GCH_BTC && ETH_VOL > 0) {
        const double dEthPrice = (GCH_ETH * ETH_BTC);
        GCH_USD = dEthPrice * BTC_USD;
        GCH_EUR = dEthPrice * BTC_EUR;
        GCH_RUB = dEthPrice * BTC_RUB;
        GCH_CNY = dEthPrice * BTC_CNY;
    } else {
        GCH_USD = GCH_BTC * BTC_USD;
        GCH_EUR = GCH_BTC * BTC_EUR;
        GCH_RUB = GCH_BTC * BTC_RUB;
        GCH_CNY = GCH_BTC * BTC_CNY;
    }
    std::stringstream btcvol;
    btcvol << std::fixed << setprecision(8) << BTC_VOL;

    std::stringstream ethvol;
    ethvol << std::fixed << setprecision(8) << ETH_VOL;

    std::stringstream btcval;
    btcval << std::fixed << setprecision(8) << GCH_BTC;
    stats += std::string("GCH/BTC:") + btcval.str() + std::string(",Vol:") + btcvol.str() + std::string(" ");

    std::stringstream ethval;
    ethval << std::fixed << setprecision(8) << GCH_ETH;
    stats += std::string("GCH/ETH:") + ethval.str() + std::string(",Vol:") + ethvol.str() + std::string(" ");

    std::stringstream usdval;
    usdval << std::fixed << setprecision(8) << GCH_USD;
    stats += std::string("\nGCH/USD:") + usdval.str() + std::string(" ");

    std::stringstream eurval;
    eurval << std::fixed << setprecision(8) << GCH_EUR;
    stats += std::string("GCH/EUR:") + eurval.str() + std::string(" ");

    std::stringstream rubval;
    rubval << std::fixed << setprecision(8) << GCH_RUB;
    stats += std::string("GCH/RUB:") + rubval.str() + std::string(" ");

    std::stringstream cnyval;
    cnyval << std::fixed << setprecision(8) << GCH_CNY;
    stats += std::string("GCH/CNY:") + cnyval.str() + std::string(" ");

    ui->priceStats->setText(QString::fromStdString(stats));
}
