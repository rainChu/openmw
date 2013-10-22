#ifndef GAME_MWNETWORK_NETWORKIMP_H
#define GAME_MWNETWORK_NETWORKIMP_H

#include "../mwbase/network.hpp"

#include <string>

#include <boost/asio.hpp>

namespace MWNetwork
{
    class Service;
    class Server;
    class Client;

    /// \brief Network manager and handler.
    /// In general, if a member function can fail multiple ways,
    /// that function will throw an exception instead of simply returning
    /// false. This allows for error messages without needing a separate "errorMessage"
    /// function.
    ///
    /// It should be noted that though there are no usernames, a "secret phrase" is used. A
    /// player wishing to connect specifies this as a password, and it's never revealed
    /// to other players. His character name is what the players see. This secret username
    /// prevents bored players from trolling their friends by connecting as someone else's
    /// character, for fun and/or profit.
    class Network : public MWBase::Network
    {
        Network( const Network &); // not implemented
        void operator=( const Network &); // not implemented
    public:
        Network();
        ~Network();

        void update();

        void getCharacterMovement(const std::string &puppetName, MWMechanics::Movement &out) const;
        void connect(const std::string &address, const std::string &slotPassword);
        void openServer(int port, const std::string &protocol);
        void close();
        void createPuppet(const std::string &secretPhrase, const MWWorld::Ptr &npc);

    private:

        friend class Service;
        friend class Server;
        friend class Client;

        bool     mIsServer;
        Service *mService;

        /// \todo make private, add interface for it.
        struct ClientInfo
        {
            std::string                    refId;
            clock_t                        lastUpdate;
            ESM::Position                  currentMovement;
            boost::asio::ip::udp::endpoint endpoint;
            
        };

        std::map<std::string, ClientInfo> mClients;
    };
}

#endif