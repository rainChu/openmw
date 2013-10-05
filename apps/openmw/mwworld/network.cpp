#include "network.hpp"

#include <boost/date_time/posix_time/posix_time.hpp>

#include <components/misc/stringops.hpp>

#include "../mwbase/environment.hpp"

#include "../mwworld/worldimp.hpp"
#include "../mwworld/player.hpp"
//#include "../mwworld/a"

namespace MWWorld
{
    class Network::Service
    {
        Service( const Service &); // Not Implemented
        void operator=( const Service &); // Not Implemented
    public:
        Service(Network &network) :
            mIoService(new boost::asio::io_service),
            mNetwork(&network)
        {}
        virtual ~Service(){}

        virtual void update();

    protected:
        boost::asio::io_service *mIoService;
        Network *mNetwork;
    };

    class Network::UdpServer :
        public Network::Service
    {
        UdpServer(const UdpServer &); // not implemented
        void operator=(const UdpServer &); // not implemented
    public:
        UdpServer(Network &network, int port);
        ~UdpServer();

        void onReceive( const boost::system::error_code &e,
                        size_t bytesTransferred);
        void onSend( const boost::system::error_code &e,
                     size_t bytesTransferred);

    private:
        boost::asio::ip::udp::socket *mSocket;

        // using a boost array allows the size to be known,
        // so there's never a buffer overrun.
        boost::array<char, sizeof Packet> mBuffer;

        boost::asio::ip::udp::endpoint mClientEndpoint;

        void listenForOne();
    };

    class Network::UdpClient :
        public Network::Service
    {
        UdpClient(const UdpClient &); // not implemented
        void operator=(const UdpClient &); // not implemented
    public:
        /// \brief Maintains a UDP connection to a UdpServer.
        /// \param address The endpoint url packets should be addressed to.
        /// \param contactRequest A packet of NewClient type to make the first contact with.
        UdpClient(Network &network, const std::string &address, const Packet &contactRequest);
        ~UdpClient();

        void onReceiveAccept( const boost::system::error_code &e,
                              size_t bytesTransferred,
                              boost::asio::deadline_timer *timeout);

    private:
        boost::asio::ip::udp::socket *mSocket;

        boost::asio::ip::udp::endpoint mServerEndpoint;
        boost::asio::ip::udp::endpoint mMyEndpoint;

        boost::array<char, sizeof Packet> mBuffer;

        bool mAccepted;
    };


    Network::Network() :
        mService( NULL )
    {
    }

    Network::~Network()
    {
        if (mService)
            delete mService;
    }

    void Network::update()
    {
        if (mService)
            mService->update();
    }

    void Network::reportCharacterMovement(const CharacterMovementPayload &payload)
    {
        //std::cout << "Moving " << packet.mRefNum << std::endl;
    }

    void Network::connect(const std::string &address, const std::string &slotPassword)
    {
        if (mService)
        {
            if (mIsServer)
                throw std::exception("Already in Server mode. Use NetworkClose first to join as a client.");
            else
                throw std::exception("Already connected. Use NetworkClose if you're sure you want to leave.");
        }
        
        if (slotPassword.length() > 31)
            throw std::exception("Secret Phrase max length is 31 characters.");

        Packet packet;
        packet.type = Packet::NewClient;

        // Gotta copy the data myself.
        strcpy( packet.payload.newClient.mPassword, slotPassword.c_str() );

        mService = new UdpClient(*this, address, packet );
        mIsServer = false;
    }

    void Network::openServer(int port, const std::string &protocol)
    {
        if (mService)
        {
            if (mIsServer)
                throw std::exception("Server is already running, stop it first with NetworkClose if you want to restart it.");
            else
                throw std::exception("Already connected as a client. Use NetworkClose if you're sure you want to leave.");
        }

        std::string normalizedProtocol(protocol);
        Misc::StringUtils::toLower(normalizedProtocol);

        if (protocol == "udp")
            mService = new UdpServer(*this, port);
        else if (protocol == "tcp")
            throw std::exception("TCP Servers aer not implemented yet, sorry!");
        else
            throw std::exception("Protocol must be either \"TCP\" or \"UDP\".");

        mIsServer = true;
    }

    void Network::close()
    {
        if (!mService)
            throw std::exception("Network is already closed.");

        delete mService;
        mService = NULL;
    }

    void Network::createPuppet(const std::string &secretPhrase, const Ptr &npc)
    {
        if (!mService)
            throw std::exception("The network isn't open.");

        std::map<std::string, Ptr>::const_iterator iter = mPuppets.find(secretPhrase);
        if (iter != mPuppets.end())
            throw std::exception("A user with that Secret Phrase already exists.");

        mPuppets[secretPhrase] = npc;
    }

    void Network::Service::update()
    {
        // TODO fire off packets here, too.
        mIoService->poll();
    }

    Network::UdpServer::UdpServer(Network &network, int port) :
        Service(network),
        mSocket( new boost::asio::ip::udp::socket(*mIoService, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port)))
    {
        listenForOne();
    }

    Network::UdpServer::~UdpServer()
    {
        // Finish all callbacks to let them handle a server shutdown state.
        mSocket->shutdown(boost::asio::ip::udp::socket::shutdown_both);;
        mSocket->cancel();
        mIoService->run();

        delete mSocket;
        delete mIoService;
    }

    void Network::UdpServer::listenForOne()
    {
        using namespace boost::asio::ip;

        mSocket->async_receive_from(
            boost::asio::buffer(mBuffer),
            mClientEndpoint,
            boost::bind(&UdpServer::onReceive, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }

    void Network::UdpServer::onReceive( const boost::system::error_code &e,
                                        size_t bytesTransferred)
    {
        if( e == boost::asio::error::operation_aborted )
            return; // Just cleaning up

        Packet *receivedPacket = (Packet *)(mBuffer.c_array());

        std::cout << "New client with Secret Phrase of '";
        std::cout << receivedPacket->payload.newClient.mPassword;
        std::cout << "'." << std::endl;

        std::map<std::string, Ptr>::const_iterator iter = mNetwork->mPuppets.find(receivedPacket->payload.newClient.mPassword);

        Packet returnPacket;

        // Puppet exists
        if (iter !=  mNetwork->mPuppets.end())
        {
            MWWorld::Ptr puppet = iter->second;
            returnPacket.type = Packet::AcceptClient;

            MWBase::World *world = MWBase::Environment::get().getWorld();

            const ESM::Cell *cell = puppet.getCell()->mCell;
            if ( cell->isExterior() )
                returnPacket.payload.acceptClient.isExterior = true;
            else
            {
                returnPacket.payload.acceptClient.isExterior = false;

                assert(world->getCurrentCellName().length() < 64);
                strcpy(returnPacket.payload.acceptClient.cellName, world->getCurrentCellName().c_str());
            }

            ESM::Position *position = &puppet.getRefData().getPosition();
            for (size_t i = 0; i < 3; ++i)
            {
                returnPacket.payload.acceptClient.position.pos[i] = position->pos[i];
                returnPacket.payload.acceptClient.position.rot[i] = position->rot[i];
            }
        }

        // Puppet doesn't exist
        else
        {
            returnPacket.type = Packet::OtherMessage;
            returnPacket.payload.messageCode = Packet::WrongPassword;
        }

        // Raw copy the packet
        memcpy(mBuffer.c_array(), &returnPacket, sizeof Packet);

        // Fire it off
        mSocket->async_send_to(boost::asio::buffer(mBuffer), mClientEndpoint,
            boost::bind(&UdpServer::onSend, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));

        // listen for another.
        listenForOne();
    }

    void Network::UdpServer::onSend( const boost::system::error_code &e,
                                        size_t bytesTransferred)
    {
    }

    Network::UdpClient::UdpClient(Network &network, const std::string &address, const Packet &contactRequest) :
        Service(network),
        mSocket(new boost::asio::ip::udp::socket(*mIoService)),
        mAccepted(false)
    {
        using namespace boost::asio::ip;

        udp::resolver resolver(*mIoService);
        udp::resolver::query query(boost::asio::ip::udp::v4(), address, "5121");
        mServerEndpoint = *resolver.resolve(query);

        mSocket->open(udp::v4());

        memcpy(mBuffer.c_array(), &contactRequest, sizeof Packet);

        mSocket->send_to(boost::asio::buffer(mBuffer), mServerEndpoint);

        boost::asio::deadline_timer timeout(*mIoService, boost::posix_time::seconds(1) );

        mSocket->async_receive(
            boost::asio::buffer(mBuffer),
            boost::bind(&UdpClient::onReceiveAccept, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred, &timeout));

        for ( int i = 5; i > 0; ++i )
        {
            std::cout << "Connecting " << i;
            mSocket->send_to(boost::asio::buffer(mBuffer), mServerEndpoint);
            timeout.wait();

            mIoService->run_one();

            if (mAccepted)
                break;

            timeout.expires_at( timeout.expires_at() + boost::posix_time::seconds(1) );
        }

        if (!mAccepted)
            throw std::exception(("Could not connect to " + address).c_str());
    }

    Network::UdpClient::~UdpClient()
    {
        // Finish all callbacks to let them handle a server shutdown state.
        mSocket->shutdown(boost::asio::ip::udp::socket::shutdown_both);;
        mSocket->cancel();
        mIoService->run();

        delete mSocket;
        delete mIoService;
    }

    void Network::UdpClient::onReceiveAccept( const boost::system::error_code &e,
            size_t bytesTransferred,
            boost::asio::deadline_timer *timeout)
    {
        // TODO Once properly checking for different errors, this is good code.
        //if( e == boost::asio::error::operation_aborted )
            //return; // Just cleaning up

        if (e == 0)
        {
            Packet *packet= (Packet *)(mBuffer.c_array());
            switch (packet->type)
            {
            case Packet::AcceptClient:
                std::cout << "The server acknowledged me. Maintaining Connection" << std::endl;

                mAccepted = true;
                timeout->cancel();

                if (packet->payload.acceptClient.isExterior)
                {
                    MWBase::World *world = MWBase::Environment::get().getWorld();
                    world->changeToExteriorCell(packet->payload.acceptClient.position);
                }
                else
                {
                    // If the cell name somehow got corrupted undetected, ending the string
                    // ensures that there won't be a buffer overrun.
                    /// \fixme Is this needed or not? Can someone with experience look into it?
                    packet->payload.acceptClient.cellName[63] = '\0';

                    MWBase::World *world = MWBase::Environment::get().getWorld();
                    world->changeToInteriorCell(packet->payload.acceptClient.cellName,
                                                packet->payload.acceptClient.position);
                }
            default:
                break;
            }
        }
    }
}