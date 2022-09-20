#include <istream>
#include <unordered_map>

#include <cctype> // used for isprint()

#include "clipboard.hpp"
#include "procblock.hpp"
#include "gpgmeinterface.hpp"

#include <gpgme++/context.h>
#include <gpgme++/data.h>
#include <gpgme++/key.h>
#include <gpgme++/keylistresult.h>
#include <gpgme++/engineinfo.h>

extern "C" {
#include "thirdParty/xdgmime/src/xdgmime.h"
}

void
Clipboard::addEntry(const std::string &block)
{
    if (!block.empty() && clipboardProcBlock(block))
        return;

    freopen(NULL, "rb", stdin);
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
    if (buffSize > MAX_SIZE_CLIPBOARD_ENTRY)
    {
        std::cout << "ClipboardEntry size ";
        std::cout << buffer.size() << " is too big, not saving that!";
        std::cout << std::endl;
        return;
    }
    ClipboardEntry newEntry{buffer, buffSize};
    if (buffSize > MIN_SIZE_SET_MIME)
        newEntry.setMimeType();

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
    if (entry.size > MIN_SIZE_COPY_VIA_FILE)
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
    // Delete restored element
    entries.erase(std::next(entries.begin(), index));
}

void
Clipboard::unpackEntries(const std::vector<char> &data)
{
    if (data.empty())
        return;
    // TODO: Errorcheck
    msgpack::object_handle oh = msgpack::unpack(data.data(), data.size());
    msgpack::object obj = oh.get();

    obj.convert(entries);
};

void
Clipboard::encryptPage() const noexcept
{
    if (entries.empty())
    {
        std::cout << "Nothing to encrypt!" << std::endl;
        return;
    }

    // Pack clipboard entries
    msgpack::sbuffer sbuf;
    msgpack::pack(sbuf, entries);

    const GpgMEInterface gpgInterface{gpgUserName};
    std::vector<char> res = gpgInterface.encrypt(sbuf.data(), sbuf.size());

    std::cout << res.size() << std::endl;
    std::cout << pagePath.string()+".gpg" << std::endl;

    std::ofstream outFile{pagePath.string()+".gpg",
        std::ios::out | std::ios::binary};
    outFile.write(res.data(), res.size());
    fs::remove(pagePath);
}

void
Clipboard::decryptPage(const std::vector<char> &data, bool write) noexcept
{
    const GpgMEInterface gpgInterface{gpgUserName};
    std::vector<char> res = gpgInterface.decrypt(data.data(), data.size());

    unpackEntries(res);
    if (write)
    {
        writePage();
        fs::remove(pagePath.string() + ".gpg");
    }
}

void
Clipboard::writePage() const
{
    // Pack clipboard entries
    msgpack::sbuffer sbuf;
    msgpack::pack(sbuf, entries);

    std::ofstream pageFile{pagePath, std::ios::out | std::ios::binary};
    pageFile.write(sbuf.data(), sbuf.size());
    pageFile.close();
}

void
Clipboard::readPage()
{
    fs::path pageFilePath = pagePath;
    bool isEncrypted = false;
    if (!fs::exists(pagePath) && fs::exists(pagePath.string() + ".gpg"))
    {
        pageFilePath = fs::path{pagePath.string() + ".gpg"};
        isEncrypted = true;
    }

    std::ifstream pageFile{pageFilePath, std::ios::in | std::ios::binary};
    size_t pageSize;
    try
    {
        pageSize = fs::file_size(pageFilePath);
    } catch (const fs::filesystem_error &err)
    {
        return;
    }
    if (pageSize == 0)
        return;

    // Unpack clipboard entries
    std::vector<char> readBuff(pageSize);
    pageFile.read(readBuff.data(), pageSize);

    if (isEncrypted)
        decryptPage(readBuff);
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
