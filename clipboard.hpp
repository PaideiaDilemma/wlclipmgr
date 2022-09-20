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
#define MIN_SIZE_SET_MIME 0x1000
#define OUTPUT_LINE_TRUNCATE_AFTER 0x20

class ClipboardEntry
{
    std::vector<char> buffer;
    size_t size;
    std::string mime;

    ClipboardEntry(const std::vector<char> &input, const size_t inputSize) :
        buffer{input}, size{inputSize}
    {
    }

    friend std::ostream &operator<<(std::ostream &os,
            const ClipboardEntry &obj);
    friend std::ofstream &operator<<(std::ofstream &os,
            const ClipboardEntry &obj);
    friend class Clipboard;

    public:
    ClipboardEntry() = default;

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

    public:
    Clipboard(const fs::path &pagePath, const fs::path &tmpFilePath,
            const std::string &gpgUserName) :
        pagePath{pagePath}, tmpFilePath{tmpFilePath}, gpgUserName{gpgUserName}
    {
    }

    ~Clipboard() = default;

    void addEntry(const std::string &block);
    void listEntries(const size_t num);
    void restore(const size_t index);

    void encryptPage() const noexcept;
    void decryptPage(const std::vector<char> &data, bool write = true) noexcept;

    void unpackEntries(const std::vector<char> &data);
    void writePage() const;
    void readPage();
};


#endif // __WLCIPMGR_CLIPBOARD_HPP
