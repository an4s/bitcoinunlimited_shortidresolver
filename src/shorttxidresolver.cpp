#include <fstream>
#include "blockrelay/graphene.h"
#include "blockstorage/blockstorage.h"
#include "validation/validation.h"
#include "logFile.h"
#include "shorttxidresolver.h"

static std::string shorttxids_dir_path;
struct BlockInfo
{
    uint64_t shorttxidk0, shorttxidk1, grapheneversion;
    std::set<uint64_t> shorttxids;
    BlockInfo() {}
    BlockInfo(uint64_t s0, uint64_t s1, uint64_t gv, std::set<uint64_t> stxids)
        : shorttxidk0(s0), shorttxidk1(s1), grapheneversion(gv), shorttxids(stxids)
    {}
};

bool initResolveShortTxIDsThread(std::string path)
{
    if(!boost::filesystem::exists(path))
    {
        std::cerr << ">> ERROR : directory <" + path + "> doesn't exist" << std::endl;
        LOGA(">> ERROR : directory <" + path + "> doesn't exist\n");
        logFile(">> ERROR : directory <" + path + "> doesn't exist");
        return false;
    }
    shorttxids_dir_path = path;
    return true;
}

std::vector<std::string> split(const std::string& i_str, const std::string& i_delim)
{
    std::vector<std::string> result;

    size_t found = i_str.find(i_delim);
    size_t startIndex = 0;

    while(found != std::string::npos)
    {
        result.push_back(std::string(i_str.begin() + startIndex, i_str.begin() + found));
        startIndex = found + i_delim.size();
        found = i_str.find(i_delim, startIndex);
    }
    if(startIndex != i_str.size())
        result.push_back(std::string(i_str.begin() + startIndex, i_str.end()));
    return result;
}

void ResolveShortTxIDsThread()
{
    std::unordered_map<std::string, BlockInfo> Block2ShortTxIDs;

    std::cout << ">> INFO - reading short tx ids from files" << std::endl;

    for(const auto &dir : boost::filesystem::directory_iterator(shorttxids_dir_path))
    {
        std::string blockhash = split(dir.path().string(), "/").back();
        if(blockhash.size() != 64) continue;
        for(const auto &file : boost::filesystem::directory_iterator(dir))
        {
            std::ifstream fin(file.path().string());
            if(fin.bad())
                throw std::runtime_error("Failed to open file <" + file.path().string() + ">");
            uint64_t shorttxidk0, shorttxidk1, grapheneversion;
            try
            {
                fin >> shorttxidk0 >> shorttxidk1 >> grapheneversion;
                std::set<uint64_t> shorttxids;
                uint64_t temp;
                while(fin >> temp)
                    shorttxids.insert(temp);
                Block2ShortTxIDs[blockhash] = BlockInfo(shorttxidk0, shorttxidk1, grapheneversion, shorttxids);
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
            }
            fin.close();
        }
    }

    std::cout << ">> INFO - finding full tx hashes" << std::endl;

    std::unordered_map<std::string, std::vector<std::string>> Block2TxIDs;
    for(auto entry : Block2ShortTxIDs)
    {
        CBlockIndex * hdr = LookupBlockIndex(uint256S(entry.first));
        if(!hdr)
            throw std::runtime_error("Requested block is not available");
        else
        {
            CBlock block;
            const Consensus::Params &consensusParams = Params().GetConsensus();
            if (!ReadBlockFromDisk(block, hdr, consensusParams))
            {
                // We do not assign misbehavior for not being able to read a block from disk because we already
                // know that the block is in the block index from the step above. Secondly, a failure to read may
                // be our own issue or the remote peer's issue in requesting too early.  We can't know at this point.
                throw std::runtime_error(
                    "Cannot load block from disk -- Block txn request possibly received before assembled");
            }
            else
            {
                for (auto &tx : block.vtx)
                {
                    uint64_t cheapHash = GetShortID(
                                            entry.second.shorttxidk0,
                                            entry.second.shorttxidk1,
                                            tx->GetHash(),
                                            entry.second.grapheneversion
                                        );

                    if (entry.second.shorttxids.count(cheapHash))
                        Block2TxIDs[entry.first].push_back(tx->GetHash().ToString());
                }
            }
        }
    }

    std::cout << ">> INFO - writing output to files" << std::endl;

    std::string outDir = shorttxids_dir_path + boost::filesystem::path::preferred_separator + "out";
    if(!boost::filesystem::exists(outDir))
        boost::filesystem::create_directory(outDir);
    for(auto entry : Block2TxIDs)
    {
        if(entry.second.size() != Block2ShortTxIDs[entry.first].shorttxids.size())
            throw std::runtime_error("Not all transactions resolved for block <" + entry.first + ">");
        std::ofstream fout(outDir + boost::filesystem::path::preferred_separator + entry.first);
        for(auto tx : entry.second)
            fout << tx << std::endl;
        fout.close();
    }
}
