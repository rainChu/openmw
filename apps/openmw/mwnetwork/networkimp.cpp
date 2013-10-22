#include "networkimp.hpp"


#include <boost/date_time/posix_time/posix_time.hpp>

#include <components/misc/stringops.hpp>

#include "../mwbase/environment.hpp"

#include "../mwworld/manualref.hpp"
#include "../mwworld/worldimp.hpp"

#include "../mwmechanics/aipuppet.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/movement.hpp"

#include "client.hpp"
#include "server.hpp"

namespace MWNetwork
{
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

    void Network::openServer(int port)
    {
        if (mService)
        {
            if (mIsServer)
                throw std::exception("Server is already running, stop it first with NetworkClose if you want to restart it.");
            else
                throw std::exception("Already connected as a client. Use NetworkClose if you're sure you want to leave.");
        }

        mService = new Server(*this, port);
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
}