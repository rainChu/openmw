#include "service.hpp"

#include "../mwbase/environment.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwmechanics/movement.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/worldimp.hpp"

#include "networkimp.hpp"

namespace MWNetwork
{
    Service::Service(Network &network) :
            mIoService(new boost::asio::io_service),
            mNetwork(&network),
            mTicks(0)
    {
    }

    Service::~Service()
    {
        delete mSocket;
        delete mIoService;
    }

    void Service::update()
    {
        mIoService->poll();
        if (mTicks++ > sTickRate)
        {
            mTicks = 0;
            sendPlayerMovementPacket();
        }
    }

    void Service::doNothing()
    {
    }

    
    void Service::listenForOne()
    {
        using namespace boost::asio::ip;

        mSocket->async_receive_from(
            boost::asio::buffer(mReceiveBuffer),
            mEndpoint,
            boost::bind(&Service::onReceive, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }

    void Service::interpretCharacterMovementPacket(const Packet &packet)
    {
        std::map<std::string, Network::ClientInfo>::iterator iter = mNetwork->mClients.find(packet.payload.characterMovement.password);

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

    void Service::repositionPuppet(MWWorld::Ptr &puppet, const CharacterMovementPayload &payload) const
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

    void Service::makePlayerMovementPacket(Packet &out, const std::string &password) const
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

    void Service::fillPuppetInfo(MWWorld::Ptr ptr, const std::string &secretPhrase, PuppetInfo &out) const
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

    void Service::onReceive( const boost::system::error_code &e,
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
}