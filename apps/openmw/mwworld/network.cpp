#include "network.hpp"

#include <components/misc/stringops.hpp>

namespace MWWorld
{
    Network::Network() :
        mServer( NULL )
    {
    }
    Network::~Network()
    {
        if (mServer)
            delete mServer;
    }
    void Network::reportCharacterMovement( const CharacterMovementPacket &packet)
    {
        //std::cout << "Moving " << packet.mRefNum << std::endl;
    }

    void Network::connect(const std::string &address, const std::string &slotPassword)
    {
        std::cout << "I was called to connect to <" << address;
        std::cout << "> with the password '" << slotPassword << "'." << std::endl;
    }

    void Network::openServer(int port, const std::string &protocol)
    {
        if (mServer)
            throw std::exception("Server is already running, stop it first with NetworkClose if you want to restart it.");

        std::string normalizedProtocol(protocol);
        Misc::StringUtils::toLower(normalizedProtocol);

        if (protocol == "udp")
            mServer = new UdpServer(mIoService, port);
        else if (protocol == "tcp")
            throw std::exception("TCP Servers aer not implemented yet, sorry!");
        else
            throw std::exception("Protocol must be either \"TCP\" or \"UDP\".");

        mIoService.run();
    }

    void Network::closeServer()
    {
        if (!mServer)
            throw std::exception("Server is not running.");

        delete mServer;
        mServer = NULL;

        // mIoService will stop now, because there's no more work.
    }

    Network::UdpServer::UdpServer(boost::asio::io_service &ioService, int port) :
        mSocket(ioService, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port) )
    {
        listenForOne();
    }

    void Network::UdpServer::listenForOne()
    {
        using namespace boost::asio::ip;

        boost::shared_ptr<udp::endpoint> remoteEndpoint(new udp::endpoint);

        // Once something's recieved, onRecieve will do work for that
        // endpoint and call itself again.
        mSocket.async_receive_from(
            boost::asio::buffer(mBuffer),
            *remoteEndpoint,
            boost::bind(&UdpServer::onReceive, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred, remoteEndpoint));

        // listen for another.
        listenForOne();
    }

    void Network::UdpServer::onReceive( const boost::system::error_code &e,
                                        size_t bytesTransferred,
                                        boost::shared_ptr<boost::asio::ip::udp::endpoint> remote)
    {
        std::cout << "I got data!" << std::endl;
    }
}