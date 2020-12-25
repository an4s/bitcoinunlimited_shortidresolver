//Custom log file system written to monitor the behaviour of
//compact blocks, getblocktxn, and blocktxn messages on a bitcoin node
//Soft forks created by two blocks coming in are also recorded

#include "logFile.h"
#include <chrono>
#include <unistd.h>
#include "net.h"
#include <string>
#include <inttypes.h>
#include <stdio.h>
#include <init.h>


#define UNIX_TIMESTAMP \
    std::to_string(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count())
#define UNIX_SYSTEM_TIMESTAMP \
    std::to_string(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count())

bool debug = false;
Env env;
const static std::string nodeID = env.getUserName();
static std::string strDataDir;
static std::string directory;
static std::string invRXdir;
static std::string txdir;
static std::string cmpctblkdir;
static std::string reqTxdir;
static std::string mempoolFileDir;
static std::string addrLoggerdir;
static std::string cpudir;
static int64_t addrLoggerTimeoutSecs = 1;
static int inc = 0;
extern CTxMemPool mempool;

void dumpMemPool(std::string fileName = "", INVTYPE type = FALAFEL_SENT, INVEVENT event = BEFORE, int counter = 0);

bool createDir(std::string dirName)
{
    boost::filesystem::path dir(dirName);
    if(!(boost::filesystem::exists(dir)))
    {
        if(fPrintToConsole)
            std::cout << "Directory <" << dir << "> doesn't exist; creating directory" << std::endl;
        if(boost::filesystem::create_directory(dir))
        {
            if(fPrintToConsole)
                std::cout << "Directory <" << dir << "> created successfully" << std::endl;
        }
        else
        {
            if(fPrintToConsole)
                std::cout << "Directory <" << dir << "> not created" << std::endl;
            return false;
        }
    }
    return true;
}

/*
 * Initialize log file system that records events in Bitcoin
 */
bool initLogger()
{
    // paths to different directories
    strDataDir     = GetDataDir().string();
    directory      = strDataDir + "/expLogFiles/";
    invRXdir       = directory  + "/received/";
    txdir          = directory  + "/txs/";
    cmpctblkdir    = directory  + "/cmpctblk/";
    reqTxdir       = directory  + "/getblocktxn/";
    mempoolFileDir = directory  + "/mempool/";

    // create directories if they do not exist

    // - create main directory if it does not exist
    if(!createDir(directory))
        return false;

    // - create directory to record information about received inv messages
    //   if it does not exist
    if(!createDir(invRXdir))
        return false;

    // - create directory to record information about received tx & inv
    //   if it does not exist
    if(!createDir(txdir))
        return false;

    // - create directory to record information about received compact blocks
    //   if it does not exist
    if(!createDir(cmpctblkdir))
        return false;

    // - create directory to record information about reqiested missing txs
    //   if it does not exist
    if(!createDir(reqTxdir))
        return false;

    // - create directory to record information about mempool when block is received
    //   if it does not exist
    if(!createDir(mempoolFileDir))
        return false;
/*
    // - create main directory if it does not exist
    boost::filesystem::path dir(directory);
    if(!(boost::filesystem::exists(dir)))
    {
        if(fPrintToConsole)
            std::cout << "Directory <" << dir << "> doesn't exist; creating directory" << std::endl;
        if(boost::filesystem::create_directory(dir))
        {
            if(fPrintToConsole)
                std::cout << "Directory <" << dir << "> created successfully" << std::endl;
        }
        else
        {
            if(fPrintToConsole)
                std::cout << "Directory <" << dir << "> not created" << std::endl;
            return false;
        }
    }

    // - create directory to record information about received inv messages
    //   if it does not exit
    boost::filesystem::path RX(invRXdir);
    if(!(boost::filesystem::exists(RX)))
    {
        if(fPrintToConsole)
            std::cout << "Directory <" << RX << "> doesn't exist; creating directory" << std::endl;
        if(boost::filesystem::create_directory(RX))
        {
            if(fPrintToConsole)
                std::cout << "Directory <" << RX << "> created successfully" << std::endl;
        }
        else
        {
            if(fPrintToConsole)
                std::cout << "Directory <" << RX << "> not created" << std::endl;
            return false;
        }
    }

    // - create directory to record information about received tx & inv
    //   if it does not exit
    boost::filesystem::path TXInv(txdir);
    if(!(boost::filesystem::exists(TXInv)))
    {
        if(fPrintToConsole)
            std::cout << "Directory <" << TXInv << "> doesn't exist; creating directory" << std::endl;
        if(boost::filesystem::create_directory(TXInv))
        {
            if(fPrintToConsole)
                std::cout << "Directory <" << TXInv << "> created successfully" << std::endl;
        }
        else
        {
            if(fPrintToConsole)
                std::cout << "Directory <" << TXInv << "> not created" << std::endl;
            return false;
        }
    }
*/
    return true;
}

/*
 * Callback for thread responsible for logging information about connected peers
 */
void AddrLoggerThread()
{
    // record address of connected peers every one second
    while(true)
    {
        // put this thread to sleep for a second
        boost::this_thread::sleep_for(boost::chrono::seconds{addrLoggerTimeoutSecs});

        std::vector<CNodeStats> vstats;
        std::string fileName = addrLoggerdir + UNIX_SYSTEM_TIMESTAMP + ".txt";
        std::ofstream fnOut(fileName, std::ofstream::out);

        // log stat for each connected node (CHECK THAT THIS IS BEHAVING AS INTENDED)
        for(CNode * pNode : vNodes)
            fnOut << pNode->addr.ToStringIP() << std::endl;

        fnOut.close();
    }
}

/*
 * Initialize log file system that logs information about connected peers
 */
bool initAddrLogger()
{
    // path to directory where addresses of connected peers are logged
    addrLoggerdir = directory + "/addrs/";

    // create directory if it does not exist
    boost::filesystem::path addr(addrLoggerdir);
    if(!(boost::filesystem::exists(addr)))
    {
        if(fPrintToConsole)
            std::cout << "Directory <" << addr << "> doesn't exist; creating directory" << std::endl;
        if(boost::filesystem::create_directory(addr))
        {
            if(fPrintToConsole)
                std::cout << "Directory <" << addr << "> created successfully" << std::endl;
        }
        else
        {
            if(fPrintToConsole)
                std::cout << "Directory <" << addr << "> not created" << std::endl;
            return false;
        }
    }
    return true;
}

/*
 * Create a human readable timestamp
 */
std::string createTimeStamp()
{
    time_t currTime;
    struct tm* timeStamp;
    std::string timeString;

    time(&currTime); //returns seconds since epoch
    timeStamp = localtime(&currTime); //converts seconds to tm struct
    timeString = asctime(timeStamp); //converts tm struct to readable timestamp string
    timeString.back() = ' '; //replaces newline with space character
    return timeString + UNIX_SYSTEM_TIMESTAMP + " : ";
}

/*
 * Log information about transactions in a compact block
 */
void logFile(CompactBlock & Cblock, std::string from, std::string fileName)
{
    std::string timeString = createTimeStamp();
    if(fileName == "") fileName = directory + "logNode_" + nodeID + ".txt";
    else fileName = directory + fileName;
    std::string blockHash = Cblock.header.GetHash().ToString();
    std::string compactBlock = cmpctblkdir + blockHash;
    if(boost::filesystem::exists(compactBlock))
    {
        std::cout << "<" + compactBlock + "> already exists.. shutting down";
        StartShutdown();
        return;
    }
    std::vector<uint64_t> txid;
    std::ofstream fnOut;
    std::ofstream fnCmpct;

    fnOut.open(fileName, std::ofstream::app);
    fnCmpct.open(compactBlock, std::ofstream::out);

    fnOut << timeString << "CMPCTRECIVED - compact block received from " << from << std::endl;
    fnOut << timeString << "CMPCTBLKHASH - " << blockHash << std::endl;
    txid = Cblock.getTXID();

    fnCmpct << blockHash << std::endl;

    for(unsigned int i = 0; i < txid.size(); i++)
    {
        fnCmpct << txid[i] << std::endl;
    }

    fnOut << createTimeStamp() << "CMPCTSAVED - " << compactBlock << " file created" << std::endl;

    if(debug){
        // std::cout << "inc: " << inc << std::endl;
        std::cout << "fileName: " << fileName << " --- cmpctblock file: " << compactBlock << std::endl;
        std::cout << timeString << "CMPCTRECIVED - compact block received from " << from << std::endl;
    }

    fnCmpct.close();
    fnOut.close();

    dumpMemPool();
    // inc++;

    // return inc - 1;
}

void logFile(std::vector<uint32_t> req, std::string blockHash, std::string fileName)
{
    std::string timeString = createTimeStamp();
    if(fileName == "") fileName = directory + "logNode_" + nodeID + ".txt";
    else fileName = directory + fileName;
    std::string reqFile = reqTxdir + blockHash;
    if(boost::filesystem::exists(reqFile))
    {
        std::cout << "<" + reqFile + "> already exists.. shutting down";
        StartShutdown();
        return;
    }
    std::ofstream fnOut;
    std::ofstream fnReq;

    fnOut.open(fileName, std::ofstream::app);
    fnReq.open(reqFile, std::ofstream::out);

    fnOut << timeString << "FAILCMPCT - getblocktxn message sent for cmpctblock: " << blockHash << std::endl;
    fnOut << timeString << "REQSENT - cmpctblock " << blockHash << " is missing " << req.size() << " tx" << std::endl;

    fnReq << timeString << "indexes requested for missing tx from cmpctblock: " << blockHash << std::endl;
    for(unsigned int i = 0; i < req.size(); i++)
    {
        fnReq << req[i] << std::endl;
    }

    fnOut << timeString << "REQSAVED -  " << reqFile << " file created" << std::endl;

    if(debug){
        std::cout << "logFile for block req called" << std::endl;
        std::cout << "filename: " << fileName << " --- reqFile: " << reqFile << std::endl;
        std::cout << timeString << "REQSAVED -  " << reqFile << " file created" << std::endl;
    }

    fnOut.close();
    fnReq.close();
}

/*
 * Log the header, ip of peer and dump the mempool for a graphene block
 */
int logFile(std::string header, std::string from, std::string fileName)
{
    std::string timeString = createTimeStamp();
    if(fileName == "") fileName = directory + "logNode_" + nodeID + ".txt";
    else fileName = directory + fileName;
    std::ofstream fnOut;

    fnOut.open(fileName, std::ofstream::app);

    fnOut << timeString << "GRRECEIVED - Graphene block received from peer:  " << from << std::endl;
    fnOut << timeString << "GRHEADER - " << header << std::endl;

    dumpMemPool();

    fnOut.close();

    return inc - 1;
}

/*
 * Log missing transactions in a graphene block
 */
int logFile(std::set<uint64_t> missingTxs, CNode* pfrom,std::string header,std::string fileName)
{
    std::string timeString = createTimeStamp();
    if(fileName == "") fileName = directory + "logNode_" + nodeID + ".txt";
    else fileName = directory + fileName;
    std::string grapheneBlock = directory + std::to_string(inc) + "_grapheneblock.txt";
    std::ofstream fnGraphene;
    std::ofstream fnOut;

    fnOut.open(fileName, std::ofstream::app);
    fnGraphene.open(grapheneBlock, std::ofstream::out);

    if(missingTxs.size() > 0)
        fnOut << timeString << "GRMISSINGTX - Missing " << missingTxs.size() << " transactions from graphene block: " << header << std::endl;
    else
        fnOut << timeString << "GRNOMISSINGTXS - All txs needed to reconstruct this graphene block were found in our mempool: " << header << std::endl;

    fnGraphene << header << std::endl;
    fnGraphene << std::to_string(pfrom->gr_shorttxidk0) << "\n" << std::to_string(pfrom->gr_shorttxidk1) << std::endl;

    for(auto itr : missingTxs)
    {
        fnGraphene << itr << std::endl;
    }

    inc++;
    fnGraphene.close();

    return inc - 1;
}

void logFile(std::string info, INVTYPE type, INVEVENT event, int counter, std::string fileName)
{
	if(info == "mempool")
	{
		dumpMemPool(fileName, type, event, counter);
		return;
	}
}

/*
 * Log some information to a file
 */
void logFile(std::string info, std::string fileName)
{
    std::string timeString = createTimeStamp();
    if(fileName == "") fileName = directory + "logNode_" + nodeID + ".txt";
    else fileName = directory + fileName;
    std::ofstream fnOut;
    fnOut.open(fileName,std::ofstream::app);

    fnOut << timeString << info << std::endl; //Thu Aug 10 11:31:32 2017\n is printed

    if(debug){
        if(!fnOut.is_open()) std::cout << "fnOut failed" << std::endl;
        std::cout << "logfile for string " << std::endl;
        std::cout << "fileName:" << fileName << std::endl;
        std::cout << timeString << info << std::endl;
    }

    fnOut.close();
}

/*
 * Log contents of a block inv message
 */
int logFile(std::vector<CInv> vInv, INVTYPE type, std::string fileName)
{
    static int count = 0;
    std::string timeString = createTimeStamp();
    if(fileName == "") fileName = directory + "logNode_" + nodeID + ".txt";
    else fileName = directory + fileName;
    std::ofstream fnOut;
    fnOut.open(fileName,std::ofstream::app);

    // log contents of an inv message that is being sent to peers
    if(type == FALAFEL_SENT)
    {
        std::string vecFile = directory + std::to_string(count) + "_vecFile_invsent.txt";
        std::ofstream fnVec;
        fnVec.open(vecFile, std::ofstream::out);

        fnOut << timeString << "VECGEN --- generated std::vector of tx to sync" << std::endl;
        fnVec << timeString << std::to_string(vInv.size()) << std::endl;

        for(unsigned int  ii = 0; ii < vInv.size(); ii++)
        {
            fnVec << vInv[ii].ToString() << std::endl; //protocol.* file contains CInv class
        }

        fnOut << timeString << "VECSAVED --- saved file of tx std::vector: " << vecFile << std::endl;

        fnVec.close();
    }
    // log contents of an inv message that is received from a peer
    else if(type == FALAFEL_RECEIVED)
    {
        std::string vecFile = invRXdir + std::to_string(count) + "_vecFile_invreceived.txt";
        std::ofstream fnVec;
        fnVec.open(vecFile, std::ofstream::out);

        fnOut << timeString << "INVRX --- received inv" << std::endl;
        fnVec << timeString << std::to_string(vInv.size()) << std::endl;

        for(unsigned int  ii = 0; ii < vInv.size(); ii++)
        {
            fnVec << vInv[ii].ToString() << std::endl; //protocol.* file contains CInv class
        }

        fnOut << timeString << "INVSAVED --- received inv saved to: " << vecFile << std::endl;

        fnVec.close();
    }

    fnOut.close();

    return count++;
}

/*
 * Log hash of a transaction inv to file
 */
void logFile(CInv inv, std::string from, std::string fileName)
{
    if(fileName == "") fileName = txdir + "txinvs_" + nodeID + ".txt";
    else fileName = txdir + fileName;
    std::ofstream fnOut(fileName, std::ofstream::app);
    fnOut << createTimeStamp() << inv.hash.ToString() << " from " << from << std::endl;
    fnOut.close();
}

/*
 * Log arrival of a valid transaction to file
 */
void logFile(CTransaction tx, std::string from, std::string fileName)
{
    if(fileName == "") fileName = txdir + "txs_" + nodeID + ".txt";
    else fileName = txdir + fileName;
    std::ofstream fnOut(fileName, std::ofstream::app);
    fnOut << createTimeStamp() << tx.GetHash().ToString() << " from " << from << std::endl;
    fnOut.close();
}

/*
 * Dump current state of the mempool to a file
 */
void dumpMemPool(std::string fileName, INVTYPE type, INVEVENT event, int counter)
{
    std::string timeString = createTimeStamp();
    std::ofstream fnOut;
    std::string sysCmd;
    std::string mempoolFile;
    fnOut.open(directory + "logNode_" + nodeID + ".txt", std::ofstream::app);
    // dump mempool before or after an inv message is received
    // (to use with finding how the inv message affects the mempool)
    if(type == FALAFEL_RECEIVED)
    {
        mempoolFile = invRXdir + std::to_string(counter) + ((event == BEFORE)? "_before" : "_after") + "_mempoolFile.txt";
        fnOut << timeString << "INV" << ((event == BEFORE) ? "B" : "A") << "DMPMEMPOOL --- Dumping mempool to file: " << mempoolFile << std::endl;
    }
    else
    {
	    mempoolFile = mempoolFileDir + fileName;
        if(boost::filesystem::exists(mempoolFile))
        {
            std::cout << "<" + mempoolFile + "> already exists.. shutting down";
            StartShutdown();
            return;
        }

	    fnOut << timeString << "DMPMEMPOOL --- Dumping mempool to file: " << mempoolFile << std::endl;
    }

    std::vector<uint256> vtxid;
    mempool.queryHashes(vtxid);

    std::ofstream fnMP(mempoolFile, std::ofstream::out);

    for (auto txid : vtxid)
        fnMP << txid.ToString() << std::endl;

    fnOut.close();
    fnMP.close();
}

long getProcessCPUStats()
{
    std::ifstream fnIn("/proc/" + std::to_string(getpid()) + "/stat");
    std::string line;
    getline(fnIn, line);

    std::istringstream ss(line);
    // refer to 'man proc', section '/proc/[pid]/stat'
    long utime = 0, stime = 0, cutime = 0, cstime = 0;
    for(int i = 1; i <= 17; i++)
    {
        std::string word;
        ss >> word;
        switch(i)
        {
            case 14: utime     = std::stoi(word); break;
            case 15: stime     = std::stoi(word); break;
            case 16: cutime    = std::stoi(word); break;
            case 17: cstime    = std::stoi(word); break;
            default: break;
        }
    }

    return utime + stime + cutime + cstime;
}

long getTotalTime()
{
    std::ifstream fnIn("/proc/stat");
    std::string line;
    getline(fnIn, line);

    std::stringstream ss(line);
    std::string word;
    ss >> word;
    long total = 0;
    while(ss >> word)
        total += std::stol(word);

    return total;
}

void CPUUsageLoggerThread()
{
    std::ofstream fnOut(directory + "/cpuusage_" + nodeID + ".txt", std::ofstream::app);
    while(true)
    {
        long first = getProcessCPUStats();
        long firstTotalTime = getTotalTime();
        // put this thread to sleep for a second
        boost::this_thread::sleep_for(boost::chrono::milliseconds{10});
        fnOut << createTimeStamp() << (100.0 * (getProcessCPUStats() - first)/(getTotalTime() - firstTotalTime)) << std::endl;
    }
}

// empty: for future uses
bool initProcessCPUUsageLogger()
{
    return true;
}