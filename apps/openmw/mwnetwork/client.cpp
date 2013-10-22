#include "client.hpp"

#include "../mwbase/environment.hpp"

#include "../mwmechanics/aipuppet.hpp"
#include "../mwmechanics/creaturestats.hpp"

#include "../mwworld/manualref.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/worldimp.hpp"

#include "networkimp.hpp"

namespace MWNetwork
{
    Client::Client(Network &network, const std::string &address, const Packet &contactRequest) :
        Service(network),
        mAccepted(false),
        mPassword(contactRequest.payload.newClient.password)
    {
        using namespace boost::asio::ip;

        mSocket = new udp::socket(*mIoService);

        // port number
        std::string port;
        std::string addressOnly;
        size_t found = address.find(':');

        if (found != std::string::npos)
        {
            port = address.substr(found+1);
            addressOnly = address.substr(0, found);
        }
        else
        {
            port = "5121";
            addressOnly = address;
        }

        udp::resolver resolver(*mIoService);
        udp::resolver::query query(boost::asio::ip::udp::v4(), addressOnly, port);
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

    Client::~Client()
    {
        // Finish all callbacks to let them handle a server shutdown state.
        mSocket->shutdown(boost::asio::ip::udp::socket::shutdown_both);;
        mSocket->cancel();
        mIoService->run();
    }

    void Client::onReceiveAccept( const boost::system::error_code &e,
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

    void Client::sendPacket(Packet &packet)
    {
        packet.timestamp = clock();

        // Raw copy the packet
        memcpy(mSendBuffer.c_array(), &packet, sizeof Packet);

        mSocket->async_send_to(boost::asio::buffer(mSendBuffer), mEndpoint,
            boost::bind(&Service::doNothing, this));
    }

    void Client::sendPlayerMovementPacket()
    {
        Packet packet;
        makePlayerMovementPacket(packet, mPassword);
        sendPacket(packet);
    }

    void Client::createPuppetNpc(const PuppetInfo &puppetInfo)
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