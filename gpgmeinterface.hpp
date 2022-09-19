#ifndef __WLCLIPMGR_GPGME_AGENT_HPP
#define __WLCLIPMGR_GPGME_AGENT_HPP

#include <gpgme++/context.h>
#include <gpgme++/data.h>
#include <gpgme++/key.h>
#include <gpgme++/keylistresult.h>
#include <gpgme++/engineinfo.h>

class GpgMEInterface
{
    void getKey(const std::string &userName = "");
    bool findUserNameInIDs(const std::vector<GpgME::UserID> &userIDs,
            const std::string &userName) const noexcept;
    void throwIfError(const GpgME::Error &err, const std::string &msg) const;

    public:
    GpgMEInterface();

    std::vector<char> encrypt(const char *buf, const size_t size) const;
    std::vector<char> decrypt(const char *buf, const size_t size) const;

    protected:
    std::unique_ptr<GpgME::Context> context;
    GpgME::Key key;
};

#endif // __WLCLIPMGR_GPGME_AGENT_HPP
