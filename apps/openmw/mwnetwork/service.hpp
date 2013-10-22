#ifndef GAME_MWNETWORK_SERVICE_H
#define GAME_MWNETWORK_SERVICE_H

#include <boost/asio.hpp>
#include <boost/array.hpp>

#include "packet.hpp"

namespace MWNetwork
{
    class Network;

    class Service
    {
        Service( const Service &); // Not Implemented
        void operator=( const Service &); // Not Implemented
    public:
        Service(Network &network);
        virtual ~Service();

        virtual void update();

        void doNothing();

        void onReceive( const boost::system::error_code &e,
                size_t bytesTransferred);

    protected:
        boost::asio::io_service      *mIoService;
        boost::asio::ip::udp::socket *mSocket;

        Network *mNetwork;

        unsigned char mTicks;
        const static unsigned char sTickRate = 2;

        boost::asio::ip::udp::endpoint mEndpoint;

        // using a boost array allows the size to be known,
        // so there's never a buffer overrun.
        boost::array<char, sizeof Packet> mSendBuffer;
        boost::array<char, sizeof Packet> mReceiveBuffer;

        void listenForOne();

        virtual void interpretNewClientPacket(const Packet &packet) = 0;
        void interpretCharacterMovementPacket(const Packet &packet);

        virtual void sendPlayerMovementPacket() = 0;
        void repositionPuppet(MWWorld::Ptr &puppet, const CharacterMovementPayload &payload) const;
        void makePlayerMovementPacket(Packet &out, const std::string &password) const;

        void fillPuppetInfo(MWWorld::Ptr ptr, const std::string &secretPhrase, PuppetInfo &out) const;
    };
}

#endif