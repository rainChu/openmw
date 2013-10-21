#ifndef GAME_MWNETWORK_NETWORKIMP_H
#define GAME_MWNETWORK_NETWORKIMP_H

#include "../mwbase/network.hpp"

#include <string>

#include <boost/asio.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwmechanics/character.hpp"

namespace MWNetwork
{
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

        /// \fixme Don't expose Packet types as public, just use them internally.
        struct CharacterMovementPayload
        {
            /// \fixme can't we identify by endpoint instead?
            char password[32];

            MWMechanics::CharacterState movementState;

            ESM::Position movement;

            /// \todo make these into an ESM::Position
            float currentPosition[3];
            float angle[3];
        };

        struct PuppetInfo
        {
            char password[32];

            /// \fixme use my own struct, movement isn't an ESM::Position anyways.
            ESM::Position movement;

            bool isExterior;

            /// \fixme find out the actual max length of a cell name
            char cellName[64];
            ESM::Position position;

            // Don't try to substute numbers to make the strings shorter. This packet
            // Isn't sent often, and assuming the mesh name will obey a format just
            // makes some mods adding new hairstyles, etc. incompatible with me.
            char model[48];
            char head[48];
            char hair[48];

            char race[32];
            char npcClass[32];
        };

        /// \fixme Don't need a union. All packets don't have to be huge just because some are.
        /// \fixme don't assume ESM::Position won't change, create a packed struct for both movement settings and position

        struct Packet
        {
            enum MessageCode
            {
                /// There is no puppet on the server with the given name
                WrongPassword
            };

            enum Type
            {
                CharacterMovement,
                NewClient,
                AcceptClient,
                OtherMessage
            };
            Type type;

            /// \brief The clock() of the sending computer.
            /// Never compare these against different machines.
            clock_t timestamp;

            /// \fixme don't use a union, no reason to make all packets the
            /// same size.
            union
            {
                /// Sent on a typical client update.
                CharacterMovementPayload characterMovement;

                /// Sent when a new player arrives
                PuppetInfo newPuppet;
                
                /// Sent from the client to request connection
                struct
                {
                    /// \todo all this char stuff is nasty! Serialize it.
                    char password[32];
                } newClient;

                /// Sent from the server when a new client is accepted.
                struct
                {
                    PuppetInfo hostPuppet;
                    PuppetInfo clientPuppet;
                } acceptClient;

                

                MessageCode messageCode;
            } payload;
        };

        void getCharacterMovement(const std::string &puppetName, MWMechanics::Movement &out) const;
        void connect(const std::string &address, const std::string &slotPassword);
        void openServer(int port, const std::string &protocol);
        void close();
        void createPuppet(const std::string &secretPhrase, const MWWorld::Ptr &npc);

    private:

        class Service;
        class Server;
        class Client;

        friend class Server;
        friend class Client;

        bool     mIsServer;
        Service *mService;

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