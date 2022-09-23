#ifndef __WLCIPMGR_CLIPBOARD_HPP
#define __WLCIPMGR_CLIPBOARD_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <deque>
#include <filesystem>
namespace fs = std::filesystem;

#include <msgpack.hpp>

#define MAX_SIZE_CLIPBOARD_ENTRY 0x1000000
#define MIN_SIZE_COPY_VIA_FILE 0x100
#define OUTPUT_LINE_TRUNCATE_AFTER 0x20

class ClipboardEntry
{
    std::vector<char> buffer;
    size_t size;
    std::string mime;

    ClipboardEntry(const std::vector<char> &input, const size_t inputSize) :
        buffer{input}, size{inputSize}
    {
        setMimeType();
    }

    friend std::ostream &operator<<(std::ostream &os,
            const ClipboardEntry &obj);
    friend std::ofstream &operator<<(std::ofstream &os,
            const ClipboardEntry &obj);
    friend class Clipboard;

    public:
    ClipboardEntry() = default;

    bool isPrintable() const noexcept;

    bool operator==(const ClipboardEntry &other) const noexcept;
    const ClipboardEntry &setMimeType();

    MSGPACK_DEFINE(buffer, size, mime)
};

class Clipboard
{
    const fs::path pagePath;
    const fs::path tmpFilePath;
    std::deque<ClipboardEntry> entries;

    const std::string gpgUserName;
    const bool notSecure;

    void encryptWritePage(const msgpack::sbuffer &sbuf) const noexcept;
    void decryptLoadPage(const std::vector<char> &data) noexcept;

    public:
    Clipboard(const fs::path &pagePath, const fs::path &tmpFilePath,
            const std::string &gpgUserName, bool notSecure) :
        pagePath{pagePath}, tmpFilePath{tmpFilePath}, gpgUserName{gpgUserName},
        notSecure{notSecure}
    {
    }

    ~Clipboard() = default;

    void addEntry(const std::string &block);
    void listEntries(const size_t num);
    void restore(const size_t index);

    void unpackEntries(const std::vector<char> &data);
    void writePage() const;
    void loadPage();
};


#endif // __WLCIPMGR_CLIPBOARD_HPP
