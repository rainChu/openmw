#ifndef GAME_MWNETWORK_SERVER_H
#define GAME_MWNETWORK_SERVER_H

#include "service.hpp"

namespace MWNetwork
{
    class Server :
        public Service
    {
        Server(const Server &); // not implemented
        void operator=(const Server &); // not implemented
    public:
        Server(Network &network, int port);
        ~Server();

    private:
        void interpretNewClientPacket(const Packet &packet);

        void sendPacketToAll(Packet &packet);
        void sendPacketToOne(Packet &packet, const boost::asio::ip::udp::endpoint &endpoint);
        void sendPlayerMovementPacket();
        void acknowledgeClient(MWWorld::Ptr puppet);
    };
}

#endif