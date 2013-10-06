#include "network.hpp"

#include <boost/date_time/posix_time/posix_time.hpp>

#include <components/misc/stringops.hpp>

#include "../mwbase/environment.hpp"

#include "../mwworld/worldimp.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/class.hpp"
#include "../mwmechanics/movement.hpp"

namespace MWWorld
{
    class Network::Service
    {
        Service( const Service &); // Not Implemented
        void operator=( const Service &); // Not Implemented
    public:
        Service(Network &network) :
            mIoService(new boost::asio::io_service),
            mNetwork(&network),
            mTicks(0)
        {}
        virtual ~Service(){}

        virtual void update();

        void doNothing(){}

    protected:
        boost::asio::io_service *mIoService;
        Network *mNetwork;

        unsigned char mTicks;
        const static unsigned char sTickRate = 2;

        void repositionPuppet(Ptr &puppet, const CharacterMovementPayload &payload) const;
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

    private:
        boost::asio::ip::udp::socket *mSocket;

        // using a boost array allows the size to be known,
        // so there's never a buffer overrun.
        boost::array<char, sizeof Packet> mBuffer;

        boost::asio::ip::udp::endpoint mClientEndpoint;

        void listenForOne();
        void sendPacket(Packet &packet);
        void acknowledgeClient(Ptr puppet);
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

        void update();

    private:
        boost::asio::ip::udp::socket *mSocket;

        boost::asio::ip::udp::endpoint mServerEndpoint;
        boost::asio::ip::udp::endpoint mMyEndpoint;

        boost::array<char, sizeof Packet> mBuffer;

        bool mAccepted;

        std::string mPassword;

        void sendPacket(Packet &packet);
        void sendPlayerMovementPacket();
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

    bool Network::getCharacterMovement(const std::string &puppetName, ESM::Position &out) const
    {
        std::map<std::string, PuppetInfo>::const_iterator iter = mPuppets.find(puppetName);

        if ( iter == mPuppets.end() )
            return false;

        // Asignment operator has no override in the POD struct.
        for (size_t i = 0; i < 3; ++i)
        {
            out.pos[i] = iter->second.currentMovement.pos[i];
            out.rot[i] = iter->second.currentMovement.rot[i];
        }

        return true;
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

        std::map<std::string, PuppetInfo>::const_iterator iter = mPuppets.find(secretPhrase);
        if (iter != mPuppets.end())
            throw std::exception("A user with that Secret Phrase already exists.");

        PuppetInfo puppetInfo;
        puppetInfo.ptr = npc;
        puppetInfo.lastUpdate = 0; // Should be okay on all systems I hope
        puppetInfo.currentMovement.pos[0] =
        puppetInfo.currentMovement.pos[1] =
        puppetInfo.currentMovement.pos[2] = 0;

        puppetInfo.currentMovement.rot[0] =
        puppetInfo.currentMovement.rot[1] =
        puppetInfo.currentMovement.rot[2] = 0;

        mPuppets[secretPhrase] = puppetInfo;
    }

    void Network::Service::repositionPuppet(Ptr &puppet, const CharacterMovementPayload &payload) const
    {
        /// \todo deal with changing cells

        ESM::Position &refpos = puppet.getRefData().getPosition();
        for (size_t i = 0; i < 3; ++i)
        {
            refpos.rot[i] = payload.mAngle[i];
            puppet.getClass().getMovementSettings(puppet).mPosition[i];
            refpos.pos[i] = payload.mCurrentPosition[i];
        }
    }

    void Network::Service::update()
    {
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

    void Network::UdpServer::onReceive( const boost::system::error_code &e,
                                        size_t bytesTransferred)
    {
        if( e == boost::asio::error::operation_aborted )
            return; // Just cleaning up

        Packet *receivedPacket = (Packet *)(mBuffer.c_array());

        switch (receivedPacket->type)
        {
        case Packet::NewClient:
            std::cout << "New client with Secret Phrase of '";
            std::cout << receivedPacket->payload.newClient.mPassword;
            std::cout << "'." << std::endl;

            {
                std::map<std::string, PuppetInfo>::const_iterator iter = mNetwork->mPuppets.find(receivedPacket->payload.newClient.mPassword);

                // Puppet exists
                if (iter !=  mNetwork->mPuppets.end())
                    acknowledgeClient(iter->second.ptr);
            }
            break;

        case Packet::CharacterMovement:
            {
                std::map<std::string, PuppetInfo>::iterator iter = mNetwork->mPuppets.find(receivedPacket->payload.characterMovement.mPassword);

                // Puppet exists
                if (iter !=  mNetwork->mPuppets.end())
                {
                    if (receivedPacket->timestamp > iter->second.lastUpdate ) //The packet isn't stale
                    {
                        if ( memcmp(&receivedPacket->payload.characterMovement.mMovement, &iter->second.currentMovement, sizeof ESM::Position) != 0) // movement has changed
                        {
                            repositionPuppet(iter->second.ptr, receivedPacket->payload.characterMovement);

                            iter->second.lastUpdate = receivedPacket->timestamp;
                        }

                        // Remember the movement
                        for (size_t i = 0; i < 3; ++i)
                        {
                            iter->second.currentMovement.pos[i] = receivedPacket->payload.characterMovement.mMovement.pos[i];
                            iter->second.currentMovement.rot[i] = receivedPacket->payload.characterMovement.mMovement.rot[i];
                        }
                    }
                }
            }

        default:
            break;
        }

        // listen for another.
        listenForOne();
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

    void Network::UdpServer::sendPacket(Packet &packet)
    {
        packet.timestamp = clock();

        // Raw copy the packet
        memcpy(mBuffer.c_array(), &packet, sizeof Packet);

        mSocket->async_send_to(boost::asio::buffer(mBuffer), mClientEndpoint,
            boost::bind(&Service::doNothing, this));
    }

    void Network::UdpServer::acknowledgeClient(Ptr puppet)
    {
        Packet packet;
        packet.type = Packet::AcceptClient;

        MWBase::World *world = MWBase::Environment::get().getWorld();

        const ESM::Cell *cell = puppet.getCell()->mCell;
        if ( cell->isExterior() )
            packet.payload.acceptClient.isExterior = true;
        else
        {
            packet.payload.acceptClient.isExterior = false;

            assert(world->getCurrentCellName().length() < 64);
            strcpy(packet.payload.acceptClient.cellName, world->getCurrentCellName().c_str());
        }

        ESM::Position *position = &puppet.getRefData().getPosition();
        for (size_t i = 0; i < 3; ++i)
        {
            packet.payload.acceptClient.position.pos[i] = position->pos[i];
            packet.payload.acceptClient.position.rot[i] = position->rot[i];
        }

        sendPacket(packet);
    }

    Network::UdpClient::UdpClient(Network &network, const std::string &address, const Packet &contactRequest) :
        Service(network),
        mSocket(new boost::asio::ip::udp::socket(*mIoService)),
        mAccepted(false),
        mPassword(contactRequest.payload.newClient.mPassword)
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

    void Network::UdpClient::update()
    {
        if (mTicks++ > sTickRate)
        {
            mTicks = 0;
            sendPlayerMovementPacket();
            mIoService->poll();
        }
    }

    void Network::UdpClient::sendPacket(Packet &packet)
    {
        packet.timestamp = clock();

        // Raw copy the packet
        memcpy(mBuffer.c_array(), &packet, sizeof Packet);

        mSocket->async_send_to(boost::asio::buffer(mBuffer), mServerEndpoint,
            boost::bind(&Service::doNothing, this));
    }

    void Network::UdpClient::sendPlayerMovementPacket()
    {
        Packet packet;
        packet.type = Packet::CharacterMovement;

        assert(mPassword.length() < 32);
        strcpy(packet.payload.characterMovement.mPassword, mPassword.c_str());

        Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();

        const ESM::Position &refpos = player.getRefData().getPosition();
        for (size_t i = 0; i < 3; ++i)
        {
            packet.payload.characterMovement.mMovement.pos[i]        = player.getClass().getMovementSettings(player).mPosition[i];
            packet.payload.characterMovement.mMovement.rot[i]        = player.getClass().getMovementSettings(player).mRotation[i];
            packet.payload.characterMovement.mCurrentPosition[i]     = refpos.pos[i];
            packet.payload.characterMovement.mAngle[i]               = refpos.rot[i];
        }

        sendPacket(packet);
    }
}