#ifndef GAME_MWBASE_NETWORK_H
#define GAME_MWBASE_NETWORK_H

#include <string>

#include <boost/asio.hpp>
#include <boost/array.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwmechanics/character.hpp"

namespace MWBase
{
    class Network
    {
        Network( const Network &); // not implemented
        void operator=( const Network &); // not implemented
    public:
        Network() {};
        virtual ~Network() {};

        virtual void update() = 0;

        /// \brief Gets the movement of a puppet in the network
        /// \param puppetName The name of the puppet to get.
        /// \param out The movement is returned.
        /// \exception std::runtime_error Throws an exception if the puppet does not exist
        virtual void getCharacterMovement(const std::string &puppetName, MWMechanics::Movement &out) const = 0;

        /// \brief Connects to a remote server.
        /// \param address a human-friendly address such as "localhost:1337" or "192.168.0.103:1337"
        /// \param slotPassword The username or password to fill. This must be open on the host machine.
        /// \exception std::exception if unable to connect, an exception will be thrown with a user-friendly reason.
        virtual void connect(const std::string &address, const std::string &slotPassword) = 0;

        /// \brief Open up to incoming connections.
        /// \param port The port to listen on. This should typically be user-provided.
        /// \param The protocol, either TCP or UDP.
        /// \exception std::exception If unable to open, an exception will be thrown with a user-friendly reason.
        virtual void openServer(int port) = 0;

        virtual void close() = 0;

        /// \brief Assigns an NPC to this network as a puppet.
        /// The NPC should also have assigned AiPuppet package, or the
        /// Network can't update it.
        /// \param secretPhrase The username or secret passphrase, possibly unknown by anyone but the server and
        ///                     the user in question, which securely associates a player to his own puppet.
        /// \param npc The NPC which is to be remotely controlled by this Network. Note that the NPC must have
        ///            the AIPuppet assigned to it, or it can't be controlled.
        /// \exception std::exception If unable to create the puppet for various reasons, an exception with a user-friendly reason will be thrown.
        virtual void createPuppet(const std::string &secretPhrase, const MWWorld::Ptr &npc) = 0;
    };
}

#endif