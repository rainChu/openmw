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

        struct Packet
        {
            enum Type
            {
                CharacterMovement
            };
            Type type;

            union
            {
                struct
                {
                    int mRefNum;
                    MWMechanics::CharacterState mMovementState;
                    Ogre::Vector3 mVelocity;
                    Ogre::Vector3 mCurrentPosition;
                    Ogre::Vector3 mAngle;
                } characterMovement;

                int dummy; // TODO remove me when there's another type
            } payload;
        };


        void reportCharacterMovement( const CharacterMovementPacket &packet);

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
        void closeServer();

    private:

        boost::asio::io_service mIoService;

        class Server{};

        class UdpServer :
            public Server
        {
        public:
            UdpServer(boost::asio::io_service &ioService, int port);

            void onReceive( const boost::system::error_code &e,
                            size_t bytesTransferred,
                            boost::shared_ptr<boost::asio::ip::udp::endpoint> remote);
        private:
            boost::asio::ip::udp::socket mSocket;

            // using a boost array allows the size to be known,
            // so there's never a buffer overrun.
            boost::array<char, sizeof Packet> mBuffer;

            void listenForOne();
        };

        Server *mServer;
    };
}

#endif