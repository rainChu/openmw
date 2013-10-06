#ifndef GAME_MWMECHANICS_AIPUPPET_H
#define GAME_MWMECHANICS_AIPUPPET_H

#include "aipackage.hpp"
#include <string>

namespace MWMechanics
{
    class AiPuppet : public AiPackage
    {
    public:
        AiPuppet( const std::string &name);
        AiPuppet *clone() const;

        bool execute (const MWWorld::Ptr& actor);
                ///< \return Package completed?

        int getTypeId() const;

    private:
        std::string mName;
    };
}
#endif