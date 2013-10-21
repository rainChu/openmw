#include "networkimp.hpp"

#include <boost/array.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <components/misc/stringops.hpp>

#include "../mwbase/environment.hpp"

#include "../mwworld/worldimp.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/manualref.hpp"

#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/aipuppet.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"

namespace MWNetwork
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
        virtual ~Service()
        {
            delete mSocket;
            delete mIoService;
        }

        virtual void update();

        void doNothing(){}

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

    class Network::Server :
        public Network::Service
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

    class Network::Client :
        public Network::Service
    {
        Client(const Client &); // not implemented
        void operator=(const Client &); // not implemented
    public:
        /// \brief Maintains a UDP connection to a Server.
        /// \param address The endpoint url packets should be addressed to.
        /// \param contactRequest A packet of NewClient type to make the first contact with.
        Client(Network &network, const std::string &address, const Packet &contactRequest);
        ~Client();

        void onReceiveAccept( const boost::system::error_code &e,
                              size_t bytesTransferred,
                              boost::asio::deadline_timer *timeout);

    private:
        boost::asio::ip::udp::endpoint mMyEndpoint;

        bool mAccepted;

        std::string mPassword;

        void interpretNewClientPacket(const Packet &packet){}

        void sendPacket(Packet &packet);
        void sendPlayerMovementPacket();
        void createPuppetNpc(const PuppetInfo &puppetInfo);
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

    void Network::getCharacterMovement(const std::string &puppetName, MWMechanics::Movement &out) const
    {
        std::map<std::string, ClientInfo>::const_iterator iter = mClients.find(puppetName);

        if ( iter == mClients.end() )
            throw std::runtime_error("Puppet does not exist");

        for (size_t i = 0; i < 3; ++i)
        {
            out.mPosition[i] = iter->second.currentMovement.pos[i];
            out.mRotation[i] = iter->second.currentMovement.rot[i];
        }
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
        strcpy( packet.payload.newClient.password, slotPassword.c_str() );

        mService = new Client(*this, address, packet );
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
            mService = new Server(*this, port);
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

    void Network::createPuppet(const std::string &secretPhrase, const MWWorld::Ptr &npc)
    {
        std::map<std::string, ClientInfo>::const_iterator iter = mClients.find(secretPhrase);
        if (iter != mClients.end())
            throw std::exception("A user with that Secret Phrase already exists.");

        ClientInfo clientInfo;

        clientInfo.lastUpdate = 0; // Should be okay on all systems I hope
        clientInfo.currentMovement.pos[0] =
        clientInfo.currentMovement.pos[1] =
        clientInfo.currentMovement.pos[2] = 0;

        clientInfo.currentMovement.rot[0] =
        clientInfo.currentMovement.rot[1] =
        clientInfo.currentMovement.rot[2] = 0;

        clientInfo.refId = npc.getCellRef().mRefID;

        mClients[secretPhrase] = clientInfo;
    }

    void Network::Service::listenForOne()
    {
        using namespace boost::asio::ip;

        mSocket->async_receive_from(
            boost::asio::buffer(mReceiveBuffer),
            mEndpoint,
            boost::bind(&Service::onReceive, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }

    void Network::Service::interpretCharacterMovementPacket(const Packet &packet)
    {
        std::map<std::string, ClientInfo>::iterator iter = mNetwork->mClients.find(packet.payload.characterMovement.password);

        // Puppet exists
        if (iter !=  mNetwork->mClients.end())
        {
            bool hasPtr;
            MWWorld::Ptr puppet;
            try
            {
                puppet = MWBase::Environment::get().getWorld()->getPtr(iter->second.refId, false);
                hasPtr = true;
            }
            catch (std::runtime_error)
            {
                hasPtr = false;
            }

            if (hasPtr && packet.timestamp > iter->second.lastUpdate) //The packet isn't stale
            {
                if ( memcmp(&packet.payload.characterMovement.movement, &iter->second.currentMovement, sizeof ESM::Position) != 0) // movement has changed
                {
                    repositionPuppet(puppet, packet.payload.characterMovement);
                    iter->second.lastUpdate = packet.timestamp;

                    // Remember the movement
                    iter->second.currentMovement = packet.payload.characterMovement.movement;
                }
            }
        }
    }

    void Network::Service::repositionPuppet(MWWorld::Ptr &puppet, const CharacterMovementPayload &payload) const
    {
        /// \todo deal with changing exterior/interior

        ESM::Position &refpos = puppet.getRefData().getPosition();
        for (size_t i = 0; i < 3; ++i)
        {
            refpos.rot[i] = payload.angle[i];
            puppet.getClass().getMovementSettings(puppet).mPosition[i];
            refpos.pos[i] = payload.currentPosition[i];
        }
    }

    void Network::Service::makePlayerMovementPacket(Packet &out, const std::string &password) const
    {
        out.type = Packet::CharacterMovement;

        assert(password.length() < 32);
        strcpy(out.payload.characterMovement.password, password.c_str());

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();

        const ESM::Position &refpos = player.getRefData().getPosition();
        for (size_t i = 0; i < 3; ++i)
        {
            out.payload.characterMovement.movement.pos[i]        = player.getClass().getMovementSettings(player).mPosition[i];
            out.payload.characterMovement.movement.rot[i]        = player.getClass().getMovementSettings(player).mRotation[i];
            out.payload.characterMovement.currentPosition[i]     = refpos.pos[i];
            out.payload.characterMovement.angle[i]               = refpos.rot[i];
        }
    }

    void Network::Service::fillPuppetInfo(MWWorld::Ptr ptr, const std::string &secretPhrase, PuppetInfo &out) const
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();

        // Password
        if (secretPhrase.length() >= 32)
            throw std::runtime_error("Secret Phrase must be < 32 characters in length");
        strcpy(out.password, secretPhrase.c_str());

        // Cell
        const ESM::Cell *cell = ptr.getCell()->mCell;

        if ( cell->isExterior() )
            out.isExterior = true;
        else
        {
            out.isExterior = false;

            if (world->getCurrentCellName().length() >= 64)
                throw std::runtime_error("Cell name must be 63 characters or less");

            strcpy( out.cellName, world->getCurrentCellName().c_str() );
        }

        // Position
        out.position = ptr.getRefData().getPosition();

        // Movement
        MWMechanics::Movement &movement =  MWWorld::Class::get(ptr).getMovementSettings(ptr);
        for( size_t i = 0; i < 3; ++i )
        {
            out.movement.pos[i] = movement.mPosition[i];
            out.movement.rot[i] = movement.mRotation[i];
        }

        // NPC Info
        std::string id = MWWorld::Class::get(ptr).getId(ptr);
        const ESM::NPC *npc = world->getStore().get<ESM::NPC>().search( id );
        assert( npc ); // the puppet uses this NPC, if it doesnt' exist then something's gone haywire.

        // Hair
        if ( npc->mHair.length() >= 48 )
            throw std::runtime_error("Hair model is incompatible, model name must be < 48 characters in length");
        strcpy(out.hair, npc->mHair.c_str());

        // Head
        if (npc->mHead.length() >= 48 )
            throw std::runtime_error("Hair model is incompatible, model name must be < 48 characters in length");
        strcpy(out.head, npc->mHead.c_str());

        // Model
        if (npc->mModel.length() >= 48 )
            throw std::runtime_error("Model is incompatible, model name must be < 48 characters in length");
        strcpy(out.model, npc->mModel.c_str());

        // Class
        if (npc->mClass.length() >= 32 )
            throw std::runtime_error("Class is incompatible, model name must be < 32 characters in length");
        strcpy(out.npcClass, npc->mClass.c_str());

        // Race
        if (npc->mRace.length() >= 32 )
            throw std::runtime_error("Race is incompatible, model name must be < 32 characters in length");
        strcpy(out.race, npc->mRace.c_str());
    }

    void Network::Service::update()
    {
        mIoService->poll();
        if (mTicks++ > sTickRate)
        {
            mTicks = 0;
            sendPlayerMovementPacket();
        }
    }

    void Network::Service::onReceive( const boost::system::error_code &e,
                                      size_t bytesTransferred)
    {
        if (!e)
        {
            Packet *receivedPacket = (Packet *)(mReceiveBuffer.c_array());

            switch (receivedPacket->type)
            {
            case Packet::NewClient:
                interpretNewClientPacket(*receivedPacket);
                break;

            case Packet::CharacterMovement:
                interpretCharacterMovementPacket(*receivedPacket);

            default:
                break;
            }
        }

        // listen for another if not aborting
        if (e != boost::asio::error::operation_aborted)
            listenForOne();
    }

    Network::Server::Server(Network &network, int port) :
        Service(network)
    {
        mSocket = new boost::asio::ip::udp::socket(*mIoService, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port));
        listenForOne();
    }

    Network::Server::~Server()
    {
        // Finish all callbacks to let them handle a server shutdown state.
        mSocket->shutdown(boost::asio::ip::udp::socket::shutdown_both);;
        mSocket->cancel();
        mIoService->run();
    }

    void Network::Server::interpretNewClientPacket(const Packet &packet)
    {
        std::cout << "New client with Secret Phrase of '";
        std::cout << packet.payload.newClient.password;
        std::cout << "'." << std::endl;

        std::map<std::string, ClientInfo>::iterator iter = mNetwork->mClients.find(packet.payload.newClient.password);

        // Puppet exists
        if (iter !=  mNetwork->mClients.end())
        {
            iter->second.endpoint = mEndpoint;
            
            MWWorld::Ptr puppet = MWBase::Environment::get().getWorld()->getPtr(iter->second.refId, false);
            acknowledgeClient(puppet);
        }
    }


    void Network::Server::sendPacketToAll(Packet &packet)
    {
        packet.timestamp = clock();

        // Raw copy the packet
        memcpy(mSendBuffer.c_array(), &packet, sizeof Packet);

        std::map<std::string, ClientInfo>::const_iterator iter = mNetwork->mClients.begin();

        boost::asio::mutable_buffers_1 buffer = boost::asio::buffer(mSendBuffer);
        for (; iter != mNetwork->mClients.end(); ++iter)
        {
            mSocket->async_send_to(buffer, iter->second.endpoint,
                boost::bind(&Service::doNothing, this));
        }
    }

    void Network::Server::sendPacketToOne(Packet &packet, const boost::asio::ip::udp::endpoint &endpoint)
    {
        packet.timestamp = clock();

        // Raw copy the packet
        memcpy(mSendBuffer.c_array(), &packet, sizeof Packet);

        mSocket->async_send_to(boost::asio::buffer(mSendBuffer), endpoint,
                boost::bind(&Service::doNothing, this));
    }

    void Network::Server::sendPlayerMovementPacket()
    {
        Packet packet;
        makePlayerMovementPacket(packet, "host");
        sendPacketToAll(packet);
    }

    void Network::Server::acknowledgeClient(MWWorld::Ptr puppet)
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

    Network::Client::Client(Network &network, const std::string &address, const Packet &contactRequest) :
        Service(network),
        mAccepted(false),
        mPassword(contactRequest.payload.newClient.password)
    {
        using namespace boost::asio::ip;

        mSocket = new udp::socket(*mIoService);

        udp::resolver resolver(*mIoService);
        udp::resolver::query query(boost::asio::ip::udp::v4(), address, "5121");
        mEndpoint = *resolver.resolve(query);

        mSocket->open(udp::v4());

        memcpy(mSendBuffer.c_array(), &contactRequest, sizeof Packet);
        mSocket->send_to(boost::asio::buffer(mSendBuffer), mEndpoint);

        boost::asio::deadline_timer timeout(*mIoService, boost::posix_time::seconds(3) );

        mSocket->async_receive(
            boost::asio::buffer(mReceiveBuffer),
            boost::bind(&Client::onReceiveAccept, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred, &timeout));

        mIoService->poll();

        for ( int i = 1; i <= 5; ++i )
        {
            std::cout << "Connecting (Attempt " << i << "/5)" << std::endl;
            mSocket->send_to(boost::asio::buffer(mSendBuffer), mEndpoint);
            timeout.wait();

            mIoService->poll();

            if (mAccepted)
                break;

            timeout.expires_at( timeout.expires_at() + boost::posix_time::seconds(3) );
        }

        if (!mAccepted)
            throw std::exception(("Could not connect to " + address).c_str());
    }

    Network::Client::~Client()
    {
        // Finish all callbacks to let them handle a server shutdown state.
        mSocket->shutdown(boost::asio::ip::udp::socket::shutdown_both);;
        mSocket->cancel();
        mIoService->run();
    }

    void Network::Client::onReceiveAccept( const boost::system::error_code &e,
            size_t bytesTransferred,
            boost::asio::deadline_timer *timeout)
    {
        if (e != 0)
            return;

        Packet *packet= (Packet *)(mReceiveBuffer.c_array());
        if (packet->type == Packet::AcceptClient)
        {
            std::cout << "The server acknowledged me. Maintaining Connection" << std::endl;

            mAccepted = true;
            timeout->cancel();

            // Move to the client puppet
            if (packet->payload.acceptClient.clientPuppet.isExterior)
            {
                MWBase::World *world = MWBase::Environment::get().getWorld();
                world->changeToExteriorCell(packet->payload.acceptClient.clientPuppet.position);
            }
            else
            {
                // If the cell name somehow got corrupted undetected, ending the string
                // ensures that there won't be a buffer overrun.
                /// \fixme Is this needed or not? Can someone with experience look into it?
                packet->payload.acceptClient.clientPuppet.cellName[63] = '\0';

                MWBase::World *world = MWBase::Environment::get().getWorld();
                world->changeToInteriorCell(packet->payload.acceptClient.clientPuppet.cellName,
                                            packet->payload.acceptClient.clientPuppet.position);
            }

            createPuppetNpc(packet->payload.acceptClient.hostPuppet);

            // Start listening for regular updates
            listenForOne();
        }
    }

    void Network::Client::sendPacket(Packet &packet)
    {
        packet.timestamp = clock();

        // Raw copy the packet
        memcpy(mSendBuffer.c_array(), &packet, sizeof Packet);

        mSocket->async_send_to(boost::asio::buffer(mSendBuffer), mEndpoint,
            boost::bind(&Service::doNothing, this));
    }

    void Network::Client::sendPlayerMovementPacket()
    {
        Packet packet;
        makePlayerMovementPacket(packet, mPassword);
        sendPacket(packet);
    }

    void Network::Client::createPuppetNpc(const PuppetInfo &puppetInfo)
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();

        ESM::NPC newRecord;
        const ESM::NPC *record;

        newRecord.blank();
        newRecord.mHead  = puppetInfo.head;
        newRecord.mHair  = puppetInfo.hair;
        newRecord.mModel = puppetInfo.model;
        newRecord.mRace  = puppetInfo.race;
        newRecord.mClass = puppetInfo.npcClass;

        newRecord.mNpdtType = 52;

        // TODO do stats properly
        newRecord.mNpdt52.mHealth = 50;
        newRecord.mNpdt52.mLevel  = 1;
        newRecord.mNpdt52.mSpeed  = 50;
        newRecord.mNpdt52.mAgility = 50;
        newRecord.mNpdt52.mDisposition = 50;
        newRecord.mNpdt52.mEndurance = 50;
        newRecord.mNpdt52.mFatigue = 50;
        newRecord.mNpdt52.mIntelligence = 50;
        newRecord.mNpdt52.mLuck = 50;
        newRecord.mNpdt52.mMana = 50;
        newRecord.mNpdt52.mPersonality = 50;
        newRecord.mNpdt52.mStrength = 50;
        newRecord.mNpdt52.mWillpower = 50;

        newRecord.mName = "Derp";

        record = world->createRecord(newRecord);

        MWWorld::ManualRef ref(world->getStore(), record->mId);
        MWWorld::Ptr puppet = ref.getPtr();

        puppet.getCellRef().mPos = puppetInfo.position;
        puppet.getRefData().setCount(1);

        MWMechanics::AiPuppet aiPackage(puppetInfo.password);
        puppet.getClass().getCreatureStats(puppet).getAiSequence().stack(aiPackage);

        MWWorld::CellStore* store;

        if (puppetInfo.isExterior)
        {
            int cx, cy;
            world->positionToIndex(puppetInfo.position.pos[0], puppetInfo.position.pos[1], cx, cy);
            store = world->getExterior(cx, cy);
        }
        else
            store = world->getInterior(puppetInfo.cellName);

        world->safePlaceObject(puppet, *store, puppetInfo.position );

        for (size_t i = 0; i < 3; ++i)
        {
            puppet.mRef->mRef.mPos.pos[i] = puppetInfo.position.pos[i];
            puppet.mRef->mRef.mPos.rot[i] = puppetInfo.position.rot[i];
        }

        mNetwork->createPuppet(puppetInfo.password, puppet);
    }
}