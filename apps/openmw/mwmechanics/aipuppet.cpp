#include "aipuppet.hpp"

#include "../mwbase/environment.hpp"
#include "../mwworld/worldimp.hpp"
#include "../mwworld/network.hpp"
#include "../mwworld/class.hpp"
#include "../mwmechanics/movement.hpp"

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
        MWWorld::Network &network = MWBase::Environment::get().getWorld()->getNetwork();
        network.getCharacterMovement(mName, actor.getClass().getMovementSettings(actor));

        return false;
    }

    int AiPuppet::getTypeId() const
    {
        return 5;
    }
}