#include "blockbrowser.h"
#include "ui_blockbrowser.h"
#include "main.h"
#include "base58.h"
#include "clientmodel.h"
#include "txdb.h"


double GetDifficulty(const CBlockIndex* blockindex, const int);
double GetDifficultyFromBits(unsigned int);

using namespace std;

const CBlockIndex* getBlockIndexByHeight(qint64 height)
{
    if(height > pindexBest->nHeight) { return pindexBest; }
    if(height < 0) { return pindexGenesisBlock; }
    qint64 desiredheight;
    desiredheight = height;
    if (desiredheight < 0 || desiredheight > nBestHeight)
        return 0;

    CBlockIndex* pblockindex = mapBlockIndex[hashBestChain];
    while (pblockindex->nHeight > desiredheight)
        pblockindex = pblockindex->pprev;
    return  pblockindex;
}

const CBlockIndex* getBlockIndexByHash(const uint256 &hash)
{
    if (mapBlockIndex.find(hash) == mapBlockIndex.end())
        return pindexGenesisBlock;

    return mapBlockIndex[hash];
}

std::string getBlockHash(const CBlockIndex *pindex)
{
    if(!pindex) { return ""; }
    return  pindex->GetBlockHash().GetHex();
}

std::string getBlockAlgorithm(const CBlockIndex *pindex)
{
    if(!pindex) { return ""; }

    switch (pindex->GetBlockAlgorithm())
    {
    case CBlock::ALGO_X11:
        return "x11";
    case CBlock::ALGO_X13:
        return "x13";
    case CBlock::ALGO_SHA256D:
        return "sha256d";
    case CBlock::ALGO_BLAKE2S:
        return "blake2s";
    default:
        return "x12";
    }
}

std::string getBlockType(const CBlockIndex *pindex)
{
    if(!pindex) { return ""; }

    if (pindex->IsProofOfWork())
        return "PoW";
    else
        return "PoS";
}

qint64 getBlockTime(const CBlockIndex *pindex)
{
    if(!pindex) { return 0; }
    return pindex->nTime;
}

std::string getBlockMerkle(const CBlockIndex *pindex)
{
    if(!pindex) { return ""; }
    return pindex->hashMerkleRoot.ToString();//.substr(0,10).c_str();
}

qint64 getBlocknBits(const CBlockIndex *pindex)
{
    if(!pindex) { return 0; }
    return pindex->nBits;
}

qint64 getBlockNonce(const CBlockIndex *pindex)
{
    if(!pindex) { return 0; }
    return pindex->nNonce;
}

double getTxTotalValue(std::string txid)
{
    uint256 hash;
    hash.SetHex(txid);

    CTransaction tx;
    uint256 hashBlock = 0;
    if (!GetTransaction(hash, tx, hashBlock))
        return 0;

    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << tx;

    double value = 0;
    double buffer = 0;
    for (unsigned int i = 0; i < tx.vout.size(); i++)
    {
        const CTxOut& txout = tx.vout[i];

        buffer = value + convertCoins(txout.nValue);
        value = buffer;
    }

    return value;
}

double getMoneySupply(const CBlockIndex *pindex)
{
    if (!pindex) return 0;
    return convertCoins(pindex->nMoneySupply);
}

double convertCoins(qint64 amount)
{
    return (double)amount / (double)COIN;
}

std::string getOutputs(std::string txid)
{
    uint256 hash;
    hash.SetHex(txid);

    CTransaction tx;
    uint256 hashBlock = 0;
    if (!GetTransaction(hash, tx, hashBlock))
        return "N/A";

    std::string str = "";
    for (unsigned int i = 0; i < tx.vout.size(); i++)
    {
        const CTxOut& txout = tx.vout[i];

        CTxDestination address;
        if (!ExtractDestination(txout.scriptPubKey, address) )
            address = CNoDestination();

        double buffer = convertCoins(txout.nValue);
        std::string amount = boost::to_string(buffer);
		str.append(CGalaxyCashAddress(address).ToString());
        str.append(": ");
        str.append(amount);
        str.append(" GCH");
        str.append("\n");
    }

    return str;
}

std::string getInputs(std::string txid)
{
    uint256 hash;
    hash.SetHex(txid);

    CTransaction tx;
    uint256 hashBlock = 0;
    if (!GetTransaction(hash, tx, hashBlock))
        return "N/A";

    std::string str = "";
    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        uint256 hash;
        const CTxIn& vin = tx.vin[i];

        hash.SetHex(vin.prevout.hash.ToString());
        CTransaction wtxPrev;
        uint256 hashBlock = 0;
        if (!GetTransaction(hash, wtxPrev, hashBlock))
             return "N/A";

        CTxDestination address;
        if (!ExtractDestination(wtxPrev.vout[vin.prevout.n].scriptPubKey, address) )
            address = CNoDestination();

        double buffer = convertCoins(wtxPrev.vout[vin.prevout.n].nValue);
        std::string amount = boost::to_string(buffer);
        str.append(CGalaxyCashAddress(address).ToString());
        str.append(": ");
        str.append(amount);
        str.append(" GCH");
        str.append("\n");
    }

    return str;
}

double BlockBrowser::getTxFees(std::string txid)
{
    return 0.0;
}

BlockBrowser::BlockBrowser(QWidget *parent) :
    QDialog(parent, (Qt::WindowMinMaxButtonsHint|Qt::WindowCloseButtonHint)),
    ui(new Ui::BlockBrowser)
{
    ui->setupUi(this);

    setBaseSize(850, 524);
    setFixedSize(QSize(850, 524));
    //this->layout()->setSizeConstraint(QLayout::SetFixedSize);

    connect(ui->searchButton, SIGNAL(pressed()), this, SLOT(searchClicked()));
    connect(ui->backButton, SIGNAL(pressed()), this, SLOT(backClicked()));
    connect(ui->closeButton, SIGNAL(pressed()), this, SLOT(close()));
}

bool isdigit(const string &value){
    QRegExp re("\\d*");  // a digit (\d), zero or more times (*)
    if (re.exactMatch(QString::fromStdString(value)))
        return true;
    return false;
}

void BlockBrowser::updateExplorerBlock(const CBlockIndex *pindex)
{
    if (!pindex)
        return;
    ui->heightLabelBE2->setVisible(true);
    ui->heightLabelBE1->setVisible(true);
    ui->algoBox->setVisible(true);
    ui->algoLabel->setVisible(true);
    ui->bitsBox->setVisible(true);
    ui->bitsLabel->setVisible(true);
    ui->nonceBox->setVisible(true);
    ui->nonceLabel->setVisible(true);
    ui->diffBox->setVisible(true);
    ui->diffLabel->setVisible(true);
    ui->merkleBox->setVisible(true);
    ui->merkleLabel->setVisible(true);
    ui->moneySupplyBox->setVisible(true);
    ui->moneySupplyLabel->setVisible(true);
    ui->inputsBox->setVisible(false);
    ui->inputsLabel->setVisible(false);
    ui->outputsBox->setVisible(false);
    ui->outputsLabel->setVisible(false);

    qint64 height = pindex->nHeight;
    ui->heightLabelBE1->setText(QString::number(height));
    ui->hashBox->setText(QString::fromUtf8(getBlockHash(pindex).c_str()));
    ui->typeBox->setText(QString::fromUtf8(getBlockType(pindex).c_str()));
    ui->algoBox->setText(QString::fromUtf8(getBlockAlgorithm(pindex).c_str()));
    ui->merkleBox->setText(QString::fromUtf8(getBlockMerkle(pindex).c_str()));
    ui->bitsBox->setText(QString::number(getBlocknBits(pindex)));
    ui->nonceBox->setText(QString::number(getBlockNonce(pindex)));
    ui->timeBox->setText(QString::fromUtf8(DateTimeStrFormat(getBlockTime(pindex)).c_str()));
    ui->diffBox->setText(QString::number(GetDifficultyFromBits(pindex->nBits), 'f', 6));
    ui->moneySupplyBox->setText(QString::number(getMoneySupply(pindex), 'f', 6) + " GCH");
}

void BlockBrowser::updateExplorerTransaction(const CTransaction &tx, const uint256 &block)
{
    ui->heightLabelBE2->setVisible(false);
    ui->heightLabelBE1->setVisible(false);
    ui->algoBox->setVisible(false);
    ui->algoLabel->setVisible(false);
    ui->bitsBox->setVisible(false);
    ui->bitsLabel->setVisible(false);
    ui->nonceBox->setVisible(false);
    ui->nonceLabel->setVisible(false);
    ui->diffBox->setVisible(false);
    ui->diffLabel->setVisible(false);
    ui->merkleBox->setVisible(false);
    ui->merkleLabel->setVisible(false);
    ui->moneySupplyBox->setVisible(false);
    ui->moneySupplyLabel->setVisible(false);
    ui->inputsBox->setVisible(true);
    ui->inputsLabel->setVisible(true);
    ui->outputsBox->setVisible(true);
    ui->outputsLabel->setVisible(true);

    ui->typeBox->setText(QString::fromUtf8("Transaction"));
    ui->hashBox->setText(QString::fromUtf8(tx.GetHash().GetHex().c_str()));
    ui->timeBox->setText(QString::fromUtf8(DateTimeStrFormat(tx.nTime).c_str()));
    ui->inputsBox->setText(QString::fromUtf8(getInputs(tx.GetHash().GetHex()).c_str()));
    ui->outputsBox->setText(QString::fromUtf8(getOutputs(tx.GetHash().GetHex()).c_str()));
}

void BlockBrowser::updateExplorerAddress(const CGalaxyCashAddress &address)
{
    ui->typeBox->setText(QString::fromUtf8("Address"));
}

void BlockBrowser::backClicked()
{
    if (!pid.empty())
    {
        cid  = pid.top(); pid.pop();
        ui->lineSearch->setText(QString::fromStdString(cid));

        if (!isdigit(cid))
        {
            CTransaction tx; uint256 hash;
            CGalaxyCashAddress address(cid);
            if (address.IsValid())
                updateExplorerAddress(address);
            else if (GetTransaction(uint256S(cid), tx, hash))
                updateExplorerTransaction(tx, hash);
            else
                updateExplorerBlock(getBlockIndexByHash(uint256S(cid)));
        }
        else
        {
            qint64 height = ui->lineSearch->text().toLongLong();
            updateExplorerBlock(getBlockIndexByHeight(height));
        }
    }
}

void BlockBrowser::searchClicked()
{
    if (ui->lineSearch->text().toStdString().empty())
        return;

    if (!cid.empty())
        pid.push(cid);

    cid = ui->lineSearch->text().toStdString();

    if (!isdigit(cid))
    {
        CTransaction tx; uint256 hash;
        CGalaxyCashAddress address(cid);
        if (address.IsValid())
            updateExplorerAddress(address);
        else if (GetTransaction(uint256S(cid), tx, hash))
            updateExplorerTransaction(tx, hash);
        else
            updateExplorerBlock(getBlockIndexByHash(uint256S(cid)));
    }
    else
    {
        qint64 height = ui->lineSearch->text().toLongLong();
        updateExplorerBlock(getBlockIndexByHeight(height));
    }
}

void BlockBrowser::setModel(ClientModel *model)
{
    this->model = model;
}

void BlockBrowser::showEvent(QShowEvent *ev)
{
    QDialog::showEvent(ev);
    int maxBlocks = 8;
    const CBlockIndex *pindex = pindexBest;
    for (int i = 0; i < maxBlocks; i++)
    {
        std::string id = std::string("block ") + std::to_string(pindex->nHeight);
        QAction *action = new QAction(QString::fromStdString(id), ui->listBlocks);
        ui->listBlocks->addAction(action);

        if (pindex->pprev)
            pindex = pindex->pprev;
    }
}

BlockBrowser::~BlockBrowser()
{
    delete ui;
}
