#include "aipuppet.hpp"

#include <iostream> // cout

namespace MWMechanics
{
    AiPuppet::AiPuppet()
    {
        // TODO
    }

    AiPuppet *AiPuppet::clone() const
    {
        return new AiPuppet(*this);
    }

    bool AiPuppet::execute (const MWWorld::Ptr& actor)
    {
        std::cout << "Puppeting..." << std::endl;
        return false;
    }

    int AiPuppet::getTypeId() const
    {
        return 5;
    }
}