#include "server.hpp"

#include "../mwbase/environment.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/worldimp.hpp"

#include "networkimp.hpp"

namespace MWNetwork
{
    Server::Server(Network &network, int port) :
        Service(network)
    {
        mSocket = new boost::asio::ip::udp::socket(*mIoService, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port));
        listenForOne();
    }

    Server::~Server()
    {
        // Finish all callbacks to let them handle a server shutdown state.
        mSocket->shutdown(boost::asio::ip::udp::socket::shutdown_both);;
        mSocket->cancel();
        mIoService->run();
    }

    void Server::interpretNewClientPacket(const Packet &packet)
    {
        std::cout << "New client with Secret Phrase of '";
        std::cout << packet.payload.newClient.password;
        std::cout << "'." << std::endl;

        std::map<std::string, Network::ClientInfo>::iterator iter = mNetwork->mClients.find(packet.payload.newClient.password);

        // Puppet exists
        if (iter !=  mNetwork->mClients.end())
        {
            iter->second.endpoint = mEndpoint;
            
            MWWorld::Ptr puppet = MWBase::Environment::get().getWorld()->getPtr(iter->second.refId, false);
            acknowledgeClient(puppet);
        }
    }


    void Server::sendPacketToAll(Packet &packet)
    {
        packet.timestamp = clock();

        // Raw copy the packet
        memcpy(mSendBuffer.c_array(), &packet, sizeof Packet);

        std::map<std::string, Network::ClientInfo>::const_iterator iter = mNetwork->mClients.begin();

        boost::asio::mutable_buffers_1 buffer = boost::asio::buffer(mSendBuffer);
        for (; iter != mNetwork->mClients.end(); ++iter)
        {
            mSocket->async_send_to(buffer, iter->second.endpoint,
                boost::bind(&Service::doNothing, this));
        }
    }

    void Server::sendPacketToOne(Packet &packet, const boost::asio::ip::udp::endpoint &endpoint)
    {
        packet.timestamp = clock();

        // Raw copy the packet
        memcpy(mSendBuffer.c_array(), &packet, sizeof Packet);

        mSocket->async_send_to(boost::asio::buffer(mSendBuffer), endpoint,
                boost::bind(&Service::doNothing, this));
    }

    void Server::sendPlayerMovementPacket()
    {
        Packet packet;
        makePlayerMovementPacket(packet, "host");
        sendPacketToAll(packet);
    }

    void Server::acknowledgeClient(MWWorld::Ptr puppet)
    {
        Packet packet;
        packet.type = Packet::AcceptClient;

        MWBase::World *world = MWBase::Environment::get().getWorld();
        MWWorld::Ptr player = world->getPlayer().getPlayer();

        fillPuppetInfo(puppet, "",     packet.payload.acceptClient.clientPuppet);
        fillPuppetInfo(player, "host", packet.payload.acceptClient.hostPuppet);

        // Send to only the person who sent to me
        sendPacketToOne(packet, mEndpoint);
    }
}