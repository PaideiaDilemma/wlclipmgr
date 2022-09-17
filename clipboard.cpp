#include <istream>

#include "clipboard.hpp"
#include "procblock.hpp"

extern "C" {
#include "thirdParty/xdgmime/src/xdgmime.h"
}

void Clipboard::addEntry(const std::string &block)
{
    if (!block.empty() && clipboardProcBlock(block))
        return;
    //char buffer[MAX_CLIPBOARD_ENTRY_SIZE];
    freopen(NULL, "rb", stdin);
    std::vector<char> buffer;
    try
    {
        buffer = std::vector<char>{std::istreambuf_iterator<char>(std::cin), {}};
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

    if (newEntry != entries[0])
        entries.push_front(newEntry);
}

void Clipboard::listEntries(const size_t num)
{
    for (size_t i = 0; i < num && i < entries.size(); i++)
    {
        const ClipboardEntry &entry = entries[i];
        std::cout << i << " " << entry << std::endl;
    }
}

void Clipboard::restore(const size_t index)
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
        const std::string prefix{"wl-copy \""};
        const size_t prefixSize = prefix.size();
        char *command = (char *)malloc(prefixSize + entry.size + 2);

        if (command == NULL) throw new std::runtime_error("malloc failed.");

        std::strncpy(command, prefix.c_str(), prefixSize);
        for (size_t i = 0; i < entry.size; i++)
        {
            command[prefixSize + i] = entry.buffer[i];
        }
        command[prefixSize + entry.size] = '\"';
        command[prefixSize + entry.size + 1] = '\0';

        std::cout << command << std::endl;
        if (system(command) != 0)
        {
            std::cerr << "Copy command failed:" << std::endl;
            std::cerr << "\t" << command << std::endl;
        }
    }
    // Delete restored element
    entries.erase(std::next(entries.begin(), index));
}

void Clipboard::writePage() const
{
    // Pack clipboard entries
    msgpack::sbuffer sbuf;
    msgpack::pack(sbuf, entries);

    std::ofstream pageFile{pagePath, std::ios::out | std::ios::binary};
    pageFile.write(sbuf.data(), sbuf.size());
    pageFile.close();
}

void Clipboard::readPage()
{
    std::ifstream pageFile{pagePath, std::ios::in | std::ios::binary};
    size_t pageSize;
    try
    {
        pageSize = fs::file_size(pagePath);
    } catch (const fs::filesystem_error &err)
    {
        return;
    }
    if (pageSize == 0)
        return;

    // Unpack clipboard entries
    char *readBuff = (char *)malloc(pageSize);
    if (readBuff == NULL) throw new std::runtime_error("malloc failed.");
    pageFile.read((char *)readBuff, pageSize);

    msgpack::object_handle oh = msgpack::unpack(readBuff, pageSize);
    msgpack::object obj = oh.get();

    obj.convert(entries);
    free(readBuff);
}

const ClipboardEntry &ClipboardEntry::setMimeType()
{
    char *cBuffer = (char *)malloc(size);
    if (cBuffer == NULL) throw new std::runtime_error("malloc failed.");
    for (size_t i = 0; i < size; i++)
        cBuffer[i] = buffer[i];

    int res_prio;
    // TODO: can we limit the size of big bufferes, while getting the same
    // mime type ?
    const char *res = xdg_mime_get_mime_type_for_data(cBuffer, size, &res_prio);

    mime = std::string{res};
    free(cBuffer);
    return *this;
}

std::ostream &operator<<(std::ostream &os,
        const ClipboardEntry &obj)
{
    if (!obj.mime.empty() && obj.mime != "text/plain")
    {
        os << "mime: [" << obj.mime << "]" << std::endl;
        os << "size: " << obj.size;
        return os;
    }

    const size_t outSize = (obj.size < OUTPUT_LINE_TRUNCATE_AFTER) ?
        obj.size : OUTPUT_LINE_TRUNCATE_AFTER;

    char *outBuffer = (char *)malloc(outSize + 1);
    if (outBuffer == NULL) throw new std::runtime_error("malloc failed.");

    for (size_t i = 0; i < outSize; i++)

        outBuffer[i] = obj.buffer[i];
    outBuffer[outSize] = '\0';

    os << outBuffer;
    free(outBuffer);

    if (outSize == OUTPUT_LINE_TRUNCATE_AFTER)
        os << "... (" << obj.size - outSize << " more chars)";

    return os;
}

std::ofstream &operator<<(std::ofstream &ofs,
        const ClipboardEntry &obj)
{
    char *outBuffer = (char *)malloc(obj.size);
    if (outBuffer == NULL) throw new std::runtime_error("malloc failed.");
    for (size_t i = 0; i < obj.size; i++)
        outBuffer[i] = obj.buffer[i];

    ofs.write(outBuffer, obj.size);
    free(outBuffer);
    return ofs;
}

bool operator==(ClipboardEntry &a,
        const ClipboardEntry &b)
{
    if (a.size != b.size)
        return false;
    for (size_t i = 0; i < a.size; i++)
    {
        if (a.buffer[i] != b.buffer[i])
            return false;
    }
    return true;
}

