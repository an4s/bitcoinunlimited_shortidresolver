//Created by Nabeel Younis <nyounis@bu.edu> on 08/10/2017
//log file header function used to log certain activity of Bitcoin Core
#ifndef LOGFILE_H
#define LOGFILE_H

#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include "blockrelay/compactblock.h" // Compact block, getblocktxn, blocktxn, normal block
#include "protocol.h" //CInv
#include "consensus/validation.h" //for cs_main
#include "net.h" // CConnman
#include "blockrelay/graphene.h"

// get current user name for logfile
// https://stackoverflow.com/a/8953445
#include <pwd.h>
#include <unistd.h>

class Env
{
    public:
    static std::string getUserName()
    {
        register struct passwd *pw;
        register uid_t uid;

        uid = geteuid ();
        pw = getpwuid (uid);
        if (pw)
        {
                return std::string(pw->pw_name);
        }
        return std::string("");
    }
};

enum INVTYPE
{
    FALAFEL_SENT,
    FALAFEL_RECEIVED,
};

enum INVEVENT
{
    BEFORE,
    AFTER,
};

#define LOG_NEIGHBOR_ADDRESSES  0
#define LOG_CPU_USAGE           0
#define ENABLE_FALAFEL_SYNC     0
#define FALAFEL_SENDER          0
#define FALAFEL_RECEIVER        0
#define LOG_TRANSACTION_INVS    0
#define LOG_TRANSACTIONS        0

#if !ENABLE_FALAFEL_SYNC && (FALAFEL_SENDER || FALAFEL_RECEIVER)
    #error "FalafelSync must be enabled"
#endif

#if ENABLE_FALAFEL_SYNC && !(FALAFEL_SENDER ^ FALAFEL_RECEIVER)
    #error "Must be only Falafel sender or receiver"
#endif

// function prototypes for different logging functions
bool initLogger();
bool initAddrLogger();
void AddrLoggerThread();
bool initProcessCPUUsageLogger();
void CPUUsageLoggerThread();
std::string createTimeStamp();
void logFile();

void logFile(std::string info, std::string fileName = ""); //logging a simple statement with timestamp
int  logFile(CompactBlock&Cblock, std::string from, std::string fileName = "");//info from cmpctBlock
void logFile(CompactReRequest&req,std::string fileName = ""); //info from getblocktxn
int logFile(std::string header, std::string from, std::string fileName);
int logFile(std::set<uint64_t> missingTxs, CNode* pfrom,std::string header,std::string fileName);
int  logFile(std::vector <CInv> vInv, INVTYPE type = FALAFEL_SENT, std::string fileName = "");
void logFile(std::string info, INVTYPE type, INVEVENT = BEFORE, int counter = 0, std::string fileName = "");
void logFile(CInv inv, std::string from, std::string fileName = "");
void logFile(CTransaction tx, std::string from, std::string fileName = "");
int logFile(CGrapheneBlock&GRblock, CNode* pfrom, std::string fileName = "");
#endif