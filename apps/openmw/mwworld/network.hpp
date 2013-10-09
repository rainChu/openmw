#ifndef GAME_MWWORLD_NETWORK_H
#define GAME_MWWORLD_NETWORK_H

#include <string>

#include <boost/asio.hpp>
#include <boost/array.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwmechanics/character.hpp"

namespace MWWorld
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
    class Network
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

        struct NewPuppetPayload
        {
            char password[32];
            ESM::Position position;
            ESM::Position movement;
        };

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

            union
            {
                /// Sent on a typical client update.
                CharacterMovementPayload characterMovement;

                /// Sent when a new player arrives
                NewPuppetPayload newPuppet;
                
                /// Sent from the client to request connection
                struct
                {
                    char password[32];
                } newClient;

                /// Sent from the server when a new client is accepted.
                struct
                {
                    bool isExterior;
                    /// \fixme find out the actual max length of a cell name
                    char cellName[64];
                    ESM::Position position;

                    NewPuppetPayload hostPuppet;
                } acceptClient;

                

                MessageCode messageCode;
            } payload;
        };

        /// \brief Gets the movement of a puppet in the network
        /// \param puppetName The name of the puppet to get.
        /// \param out The movement is returned.
        /// \exception std::runtime_error Throws an exception if the puppet does not exist
        void getCharacterMovement(const std::string &puppetName, MWMechanics::Movement &out) const;

        /// \brief Connects to a remote server.
        /// \param address a human-friendly address such as "localhost:1337" or "192.168.0.103:1337"
        /// \param slotPassword The username or password to fill. This must be open on the host machine.
        /// \exception std::exception if unable to connect, an exception will be thrown with a user-friendly reason.
        void connect(const std::string &address, const std::string &slotPassword);

        /// \brief Open up to incoming connections.
        /// \param port The port to listen on. This should typically be user-provided.
        /// \param The protocol, either TCP or UDP.
        /// \exception std::exception If unable to open, an exception will be thrown with a user-friendly reason.
        void openServer(int port, const std::string &protocol);

        void close();

        /// \brief Assigns an NPC to this network as a puppet.
        /// The NPC should also have assigned AiPuppet package, or the
        /// Network can't update it.
        /// \param secretPhrase The username or secret passphrase, possibly unknown by anyone but the server and
        ///                     the user in question, which securely associates a player to his own puppet.
        /// \param npc The NPC which is to be remotely controlled by this Network. Note that the NPC must have
        ///            the AIPuppet assigned to it, or it can't be controlled.
        /// \exception std::exception If unable to create the puppet for various reasons, an exception with a user-friendly reason will be thrown.
        void createPuppet(const std::string &secretPhrase, const Ptr &npc);

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