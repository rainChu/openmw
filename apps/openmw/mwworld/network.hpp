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
    class Network
    {
        Network( const Network &); // not implemented
        void operator=( const Network &); // not implemented
    public:
        Network();
        ~Network();

        void update();

        struct CharacterMovementPayload
        {
            int mRefNum;
            MWMechanics::CharacterState mMovementState;

            // Can't use Ogre::Vector3 in a union
            float mVelocity[3];
            float mCurrentPosition[3];
            float mAngle[3];
        };

        struct Packet
        {
            enum Type
            {
                CharacterMovement,
                NewClient
            };
            Type type;

            union
            {
                CharacterMovementPayload characterMovement;
                
                struct
                {
                    char mPassword[32];
                } newClient;
            } payload;
        };

        void reportCharacterMovement( const CharacterMovementPayload &payload);

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

    private:

        class Service;
        class UdpServer;
        class UdpClient;

        bool     mIsServer;
        Service *mService;
    };
}

#endif