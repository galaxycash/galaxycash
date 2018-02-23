#ifndef BLOCKBROWSER_H
#define BLOCKBROWSER_H

#include "clientmodel.h"
#include "main.h"
#include "base58.h"
#include <QDialog>
#include <stack>

namespace Ui {
class BlockBrowser;
}
class ClientModel;

class BlockBrowser : public QDialog
{
    Q_OBJECT

public:
    enum
    {
        BLOCK = 0,
        TRANSACTION,
        ADDRESS
    };

    explicit BlockBrowser(QWidget *parent = 0);
    ~BlockBrowser();

    void setModel(ClientModel *model);

protected:
    void showEvent(QShowEvent *ev) override;
    
public slots:

    void backClicked();
    void searchClicked();

    void updateExplorerBlock(const CBlockIndex *block);
    void updateExplorerTransaction(const CTransaction &tx, const uint256 &block);
    void updateExplorerAddress(const CGalaxyCashAddress &address);
    double getTxFees(std::string);

private slots:

private:
    Ui::BlockBrowser *ui;
    ClientModel *model;
    std::stack<std::string> pid;
    std::string cid;
};

double getTxTotalValue(std::string); 
double getMoneySupply(const CBlockIndex*);
double convertCoins(qint64); 
qint64 getBlockTime(const CBlockIndex*);
qint64 getBlocknBits(const CBlockIndex*);
qint64 getBlockNonce(const CBlockIndex*);
qint64 getBlockHashrate(const CBlockIndex*);
std::string getInputs(std::string); 
std::string getOutputs(std::string); 
std::string getBlockHash(const CBlockIndex*);
std::string getBlockAlgo(const CBlockIndex*);
std::string getBlockMerkle(const CBlockIndex*);
bool addnode(std::string); 
const CBlockIndex* getBlockIndexByHeight(qint64);
const CBlockIndex* getBlockIndexByHash(const uint256 &);

#endif // BLOCKBROWSER_H
