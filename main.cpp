#include <iostream>
#include <fstream>
#include <filesystem>

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
    Command &command_ = arg("Command for wlclipmgr");
    std::string &page_ = kwarg("p,page", "clipboard page").set_default("");
    size_t &index_ = kwarg("i,index", "page index to restore").set_default(0);
    size_t &lines_ = kwarg("l,lines", "how many lines to list").set_default(10);
    std::string &block_ = kwarg("b,block",
        "Block saving the cliboard if a certain process is running.")
        .set_default("");
    /*
        Example: pass:10,scary_app | This will not save the clipboard
        if there is a process newer than 10 seconds and its cmdline
        has "pass" in it,
        or if there is a process that has \"scary_app\" in its cmdline
    */
    bool &notSecure_ = flag("no-encryption", "disable encryption with gpg");
    std::string &gpgUserName_ = kwarg("u,gpg-user",
        "User name of the gpg key to use, when de-/encypting.")
        .set_default("");
    /*
       if not provided, the first key, that can encrypt and has a
       secret will be used. (to list your keys, use gpg(2) --list-keys))
    */
    std::string programm_name_{};
};

void doWatch(const Args &args)
{
    std::stringstream command;
    command << "wl-paste -w " << args.programm_name_ << " store";
    if (!args.page_.empty())
        command << " --page " << args.page_;
    if (!args.block_.empty())
        command << " --block \"" << args.block_ << '\"';
    if (args.notSecure_)
        command << " --no-encryption";
    std::system(command.str().c_str());
}

void
doCommand(const Args &args, Clipboard &clipboard)
{
    switch(args.command_)
    {
        case Command::store:
            clipboard.loadPage();
            clipboard.addEntry(args.block_);
            clipboard.writePage();
            break;
        case Command::list:
            clipboard.loadPage();
            clipboard.listEntries(args.lines_);
            break;
        case Command::restore:
            clipboard.loadPage();
            clipboard.restore(args.index_);
            break;
        case Command::watch:
            doWatch(args);
            break;
    }
}

int
main(int argc, char *argv[])
{
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

    auto args = argparse::parse<Args>(argc, argv);
    args.programm_name_ = argv[0];

    std::string page;
    if (args.page_.empty())
        page = getDefaultPage();

    else page = args.page_;


    Clipboard clipboard{
        cacheDir / page,
        cacheDir / "tmpfile",
        args.gpgUserName_,
        args.notSecure_
    };

    try
    {
        doCommand(args, clipboard);
    }
    catch (const std::runtime_error &err)
    {
        std::cerr << err.what() << std::endl;
        return -1;
    }
    return 0;
}
