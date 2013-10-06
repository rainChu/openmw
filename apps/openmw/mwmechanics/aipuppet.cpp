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
        ESM::Position position;
        MWBase::Environment::get().getWorld()->getNetwork().getCharacterMovement(mName, position);

        for (size_t i = 0; i < 3; ++i)
        {
            actor.getClass().getMovementSettings(actor).mPosition[i] = position.pos[i];
            actor.getClass().getMovementSettings(actor).mRotation[i] = position.rot[i];
        }

        return false;
    }

    int AiPuppet::getTypeId() const
    {
        return 5;
    }
}