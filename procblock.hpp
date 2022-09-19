#ifndef __WLCLIPMGR_PROCBLOCK_HPP
#define __WLCLIPMGR_PROCBLOCK_HPP

#include <string>
#include <vector>

struct procInfo_m
{
    const std::string name;
    const size_t age;
    std::vector<std::string> args;
    procInfo_m(const std::string name, const size_t age)
        : name{name}, age{age}
    {
        args.push_back(name);
    }
    bool matches(const procInfo_m &other) const
    {
        for (const auto &arg : args)
        {
            if ((arg.find(other.name) != std::string::npos) && age < other.age)
                return true;
        }
        return false;
    }
};

// Check if smth blocks copying the clipbaord
bool clipboardProcBlock(const std::string &blockString);

std::vector<procInfo_m> parseBlockOpt(const std::string blockStr,
        size_t &newerThanMax); // parse the block argument string
unsigned long long getUpTime(void); // get /proc/uptime
std::vector<std::string> stringSplit(std::string s, std::string delim);
long procps_hertz_get(void); // get system hertz (ripped from procps)


#endif //__WLCLIPMGR_PROCBLOCK_HPP

