#include <istream>
#include <unordered_map>

#include <cctype> // used for isprint()

#include "clipboard.hpp"
#include "procblock.hpp"
#include "gpgmeinterface.hpp"

extern "C" {
#include "thirdParty/xdgmime/src/xdgmime.h"
}

void
Clipboard::addEntry(const std::string &block)
{
    if (!block.empty() && clipboardProcBlock(block))
        return;

    std::freopen(NULL, "rb", stdin);
    std::vector<char> buffer;
    try
    {
        buffer = std::vector<char>{
            std::istreambuf_iterator<char>{std::cin}, {}
        };
    } catch (const std::ios::failure &err) {
        std::cerr << "Failed to read clipboard content!" << std::endl;
        std::cerr << err.what() << std::endl;
        return;
    }
    const size_t buffSize = buffer.size();
    if (buffSize == 0)
        return;
    if (buffSize > MAX_SIZE_CLIPBOARD_ENTRY)
    {
        std::cout << "ClipboardEntry size ";
        std::cout << buffer.size() << " is too big, not saving that!";
        std::cout << std::endl;
        return;
    }
    ClipboardEntry newEntry{buffer, buffSize};

    if (entries.empty() || newEntry != entries[0])
        entries.push_front(newEntry);
}

void
Clipboard::listEntries(const size_t num)
{
    for (size_t i = 0; i < num && i < entries.size(); i++)
    {
        const ClipboardEntry &entry = entries[i];
        std::cout << i << " " << entry << std::endl;
    }
}

void
Clipboard::restore(const size_t index)
{
    if (index == 0 || index >= entries.size())
    {
        std::cout << "Nothing to restore" << std::endl;
        return;
    }
    const ClipboardEntry entry = entries[index];
    // Erase and write before copying the ClipboardEntry
    // makes sure the file is written, before wl-paste invokes wlclipmgr again
    entries.erase(std::next(entries.begin(), index));
    writePage();

    if (entry.size > MIN_SIZE_COPY_VIA_FILE || !entry.isPrintable())
    {
        try
        {
            std::ofstream tmpFile{tmpFilePath,
                std::ios::out | std::ios::binary};
            tmpFile << entry;
            tmpFile.close();
        }
        catch (const std::exception &err)
        {
            std::cerr << "Failed to write entry to tmpfile." << std::endl;
            return;
        }
        if (std::system(("wl-copy < " + tmpFilePath.string()).c_str()) != 0)
        {
            std::cerr << "Copy file command failed:" << std::endl;
            std::cerr << "\twl-copy < " << tmpFilePath.string() << std::endl;
        }
    }
    else
    {
        std::cout << "hello" << std::endl;
        const std::string prefix = "wl-copy \"";
        std::vector<char> command(prefix.begin(), prefix.end());

        command.insert(command.end(), entry.buffer.begin(),
                entry.buffer.end());

        command.push_back('\"');
        command.push_back('\0');

        if (std::system(command.data()) != 0)
        {
            std::cerr << "Copy command failed:" << std::endl;
            std::cerr << '\t' << command.data() << std::endl;
        }
    }
}

void
Clipboard::unpackEntries(const std::vector<char> &data)
{
    if (data.empty())
        return;

    msgpack::object_handle oh = msgpack::unpack(data.data(), data.size());
    msgpack::object obj = oh.get();

    obj.convert(entries);
};

void
Clipboard::encryptWritePage(const msgpack::sbuffer &sbuf) const noexcept
{
    const GpgMEInterface gpgInterface{gpgUserName};
    std::vector<char> res = gpgInterface.encrypt(sbuf.data(), sbuf.size());

    std::ofstream outFile{pagePath.string()+".gpg",
        std::ios::out | std::ios::binary};

    outFile.write(res.data(), res.size());
    outFile.close();

    if (fs::exists(pagePath))
        fs::remove(pagePath);
}

void
Clipboard::decryptLoadPage(const std::vector<char> &data) noexcept
{
    const GpgMEInterface gpgInterface{gpgUserName};
    std::vector<char> res = gpgInterface.decrypt(data.data(), data.size());

    unpackEntries(res);
}

void
Clipboard::writePage() const
{
    if (entries.empty())
    {
        std::cout << "Nothing to write!" << std::endl;
        return;
    }
    msgpack::sbuffer sbuf;
    msgpack::pack(sbuf, entries);

    if (notSecure)
    {
        std::ofstream pageFile{pagePath, std::ios::out | std::ios::binary};
        pageFile.write(sbuf.data(), sbuf.size());
        pageFile.close();

        if (fs::exists(pagePath.string() + ".gpg"))
            fs::remove(pagePath.string() + ".gpg");

        return;
    }

    encryptWritePage(sbuf);
}

void
Clipboard::loadPage()
{
    fs::path pageFilePath{pagePath.string() + ".gpg"};
    bool isEncrypted = true;
    if (!fs::exists(pageFilePath))
    {
        if (!fs::exists(pagePath))
            return;
        pageFilePath = fs::path{pagePath};
        isEncrypted = false;
    }

    std::ifstream pageFile{pageFilePath, std::ios::in | std::ios::binary};
    size_t pageSize;
    try
    {
        pageSize = fs::file_size(pageFilePath);
    } catch (const fs::filesystem_error &err)
    {
        std::cerr << err.what() << std::endl;
        return;
    }
    if (pageSize == 0)
        return;

    std::vector<char> readBuff(pageSize);
    pageFile.read(readBuff.data(), pageSize);
    pageFile.close();

    if (isEncrypted)
        decryptLoadPage(readBuff);
    else
        unpackEntries(readBuff);
}

const ClipboardEntry &
ClipboardEntry::setMimeType()
{
    int res_prio;
    const char *res = xdg_mime_get_mime_type_for_data(buffer.data(),
            size, &res_prio);

    mime = std::string{res};
    return *this;
}

bool ClipboardEntry::isPrintable() const noexcept
{
    static const std::string textMime = "text";
    if (mime.empty() || mime.size() < textMime.size())
        return false;
    if ("text" == mime.substr(0, textMime.size() - 1))
        return true;
    return false;
}

bool
ClipboardEntry::operator==(const ClipboardEntry &other) const noexcept
{
    if (size != other.size)
        return false;
    if (buffer != other.buffer)
        return false;
    return true;
}

std::ostream &
operator<<(std::ostream &os, const ClipboardEntry &obj)
{
    static const std::unordered_map<char,std::string> substitute
    {
        {'\n', "\\n"}, {'\r', "\\r"}, {'\r', "\\r"}
    };

    if (!obj.mime.empty() && obj.mime != "text/plain")
    {
        os << "mime: [" << obj.mime << "] size: " << obj.size;
        return os;
    }

    const size_t outSize = (obj.size < OUTPUT_LINE_TRUNCATE_AFTER) ?
        obj.size : OUTPUT_LINE_TRUNCATE_AFTER;

    std::vector<char> outBuffer(obj.buffer.begin(),
            obj.buffer.begin() + outSize);

    for (const char c : outBuffer)
    {
        auto it = substitute.find(c);
        if (it != substitute.end())
            os << it->second;
        else if (std::isprint(c))
            os << c;
        else
            os << "█";
    }

    if (outSize == OUTPUT_LINE_TRUNCATE_AFTER)
        os << "... (" << obj.size - outSize << " more chars)";

    return os;
}

std::ofstream &
operator<<(std::ofstream &ofs, const ClipboardEntry &obj)
{
    ofs.write(obj.buffer.data(), obj.buffer.size());
    return ofs;
}
