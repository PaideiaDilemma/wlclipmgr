#include <iostream>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

#include "clipboard.hpp"
#include "thirdParty/argparse/include/argparse/argparse.hpp"


std::string
getDefaultPage()
{
    // Handle default page by setting it to date format clipddmmyy
    time_t now = std::time(0);
    tm *ltm = std::localtime(&now);
    std::stringstream ss;
    std::string year{std::to_string(ltm->tm_year)};
    ss << "clip";
    ss << std::setfill('0') << std::setw(2) << ltm->tm_mday;
    ss << std::setfill('0') << std::setw(2) << ltm->tm_mon;
    ss << std::string(year.end()-2, year.end());
    std::cout << std::string(year.end()-2, year.end()) << std::endl;
    return ss.str();
}

enum Command
{
    save,
    watch,
    list,
    restore,
    encrypt
};

struct Args : public argparse::Args
{
    Command &command = arg("Command for wlclipmgr");
    std::string &page = kwarg("p,page", "clipboard page").set_default("");
    size_t &index = kwarg("i,index", "page index to restore").set_default(0);
    size_t &lines = kwarg("l,lines", "how many lines to list").set_default(10);
    std::string &block = kwarg("b,block",
        "Block saving the cliboard if a certain process is running."
        "Example: pass:10,scary_app | This will not save the clipboard"
        "if there is a process newer than 10 seconds and its cmdline"
        "has \"pass\" in it,"
        "or if there is a process that has \"scary_app\" in its cmdline")
        .set_default("");
};

void
doCommand(const Args &args, Clipboard &clipboard)
{
    switch(args.command)
    {
        case Command::save:
            clipboard.readPage();
            clipboard.addEntry(args.block);
            clipboard.writePage();
            break;
        case Command::list:
            clipboard.readPage();
            clipboard.listEntries(args.lines);
            break;
        case Command::restore:
            clipboard.readPage();
            clipboard.restore(args.index);
            clipboard.writePage();
            break;
        case Command::watch:
            break;
        case Command::encrypt:
            clipboard.readPage();
            clipboard.encryptPage();
            break;
    }
}

int
main(int argc, char *argv[])
{
    const auto args = argparse::parse<Args>(argc, argv);

    std::string page;
    if (args.page.empty())
        page = getDefaultPage();

    else page = args.page;

    fs::path cacheDir;
    const char *xdgVar = std::getenv("XDG_CACHE_HOME");
    if (xdgVar == NULL)
    {
        xdgVar = std::getenv("HOME");
        if (xdgVar == NULL)
        {
            std::cerr << "Fatal: env $HOME not found!" << std::endl;
            return -1;
        }
        cacheDir = fs::path(std::string(xdgVar) + "/.cache/");
    }
    else
        cacheDir = fs::path(xdgVar);

    cacheDir = cacheDir / "wlclipmgr";
    if (!fs::exists(cacheDir))
        fs::create_directory(cacheDir);

    Clipboard clipboard{cacheDir / page, cacheDir / "tmpfile"};

    try
    {
        doCommand(args, clipboard);
    }
    catch (const std::exception &err)
    {
        std::cerr << err.what() << std::endl;
        return -1;
    }
    return 0;
}
