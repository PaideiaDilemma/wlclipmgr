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
    return ss.str();
}

enum Command
{
    store,
    watch,
    list,
    restore
};

struct Args : public argparse::Args
{
    Command &command = arg("Command for wlclipmgr");
    std::string &page = kwarg("p,page", "clipboard page").set_default("");
    size_t &index = kwarg("i,index", "page index to restore").set_default(0);
    size_t &lines = kwarg("l,lines", "how many lines to list").set_default(10);
    std::string &block = kwarg("b,block",
        "Block saving the cliboard if a certain process is running.")
        .set_default("");
    /*
        Example: pass:10,scary_app | This will not save the clipboard
        if there is a process newer than 10 seconds and its cmdline
        has "pass" in it,
        or if there is a process that has \"scary_app\" in its cmdline
    */
    bool &notSecure = flag("no-encryption", "disable encryption with gpg");
    std::string &gpgUserName = kwarg("u,gpg-user",
        "User name of the gpg key to use, when de-/encypting.")
        .set_default("");
    /*
       if not provided, the first key, that can encrypt and has a
       secret will be used. (to list your keys, use gpg(2) --list-keys))
    */
};

void doWatch(const Args &args, const std::string &argv_0)
{
    std::string command = "wl-paste -w " + argv_0 + " store";
    if (!args.page.empty())
        command += " --page " + args.page;
    if (!args.block.empty())
        command += " --block " + args.block;
    if (args.notSecure)
        command += " --no-encryption";
    std::system(command.c_str());
}

void
doCommand(const Args &args, Clipboard &clipboard,
        const std::string &argv_0)
{
    switch(args.command)
    {
        case Command::store:
            clipboard.loadPage();
            clipboard.addEntry(args.block);
            clipboard.writePage();
            break;
        case Command::list:
            clipboard.loadPage();
            clipboard.listEntries(args.lines);
            break;
        case Command::restore:
            clipboard.loadPage();
            clipboard.restore(args.index);
            break;
        case Command::watch:
            doWatch(args, argv_0);
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
        // lets just assume this exists
        cacheDir = fs::path(std::string(xdgVar) + "/.cache/");
    }
    else
        cacheDir = fs::path(xdgVar);

    cacheDir = cacheDir / "wlclipmgr";

    if (!fs::exists(cacheDir))
        fs::create_directory(cacheDir);

    Clipboard clipboard{
        cacheDir / page,
        cacheDir / "tmpfile",
        args.gpgUserName,
        args.notSecure
    };

    const std::string argv_0 = argv[0];
    try
    {
        doCommand(args, clipboard, argv_0);
    }
    catch (const std::runtime_error &err)
    {
        std::cerr << err.what() << std::endl;
        return -1;
    }
    return 0;
}
