#include "aipuppet.hpp"

#include "../mwbase/environment.hpp"
#include "../mwworld/worldimp.hpp"
#include "../mwworld/class.hpp"

#include "../mwmechanics/movement.hpp"

#include "../mwnetwork/networkimp.hpp"

namespace MWMechanics
{
    AiPuppet::AiPuppet(const std::string &name) :
        mName(name)
    {
    }

    AiPuppet *AiPuppet::clone() const
    {
        return new AiPuppet(*this);
    }

    bool AiPuppet::execute (const MWWorld::Ptr& actor)
    {
        MWBase::Network *network = MWBase::Environment::get().getNetwork();
        network->getCharacterMovement(mName, actor.getClass().getMovementSettings(actor));

        return false;
    }

    int AiPuppet::getTypeId() const
    {
        return 5;
    }
}