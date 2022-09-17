#ifndef __Clipboard_hpp
#define __Clipboard_hpp

#include <iostream>
#include <fstream>
#include <vector>
#include <deque>

#include <filesystem>
namespace fs = std::filesystem;

#include <msgpack.hpp>

#define MAX_SIZE_CLIPBOARD_ENTRY 0x100000
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
        if (inputSize > 0x10) {} // Mime
    }

    friend std::ostream &operator<<(std::ostream &os,
            const ClipboardEntry &obj);
    friend std::ofstream &operator<<(std::ofstream &os,
            const ClipboardEntry &obj);
    friend bool operator==(ClipboardEntry &a,
            const ClipboardEntry &b);
    friend class Clipboard;

    public:
    MSGPACK_DEFINE(buffer, size, mime)
    ClipboardEntry() = default;
    const ClipboardEntry &setMimeType();
};

class Clipboard
{
    const fs::path pagePath;
    const fs::path tmpFilePath;
    std::deque<ClipboardEntry> entries;

    public:
    Clipboard(const fs::path pagePath, const fs::path tmpFilePath) :
        pagePath{pagePath}, tmpFilePath{tmpFilePath}
    {
    }

    ~Clipboard() = default;

    void addEntry(const std::string &block);
    void listEntries(const size_t num);
    void restore(const size_t index);

    void writePage() const;
    void readPage();
};


#endif // __Clipboard_hpp
