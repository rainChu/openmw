#ifndef GAME_MWWORLD_NETWORK_H
#define GAME_MWWORLD_NETWORK_H

#include <string>

#include "../mwworld/ptr.hpp"
#include "../mwmechanics/character.hpp"

namespace MWWorld
{
    class Network
    {
    public:
        struct CharacterMovementPacket
        {
            int mRefNum;
            MWMechanics::CharacterState mMovementState;
            Ogre::Vector3 mVelocity;
            Ogre::Vector3 mCurrentPosition;
            Ogre::Vector3 mAngle;
        };

        void reportCharacterMovement( const CharacterMovementPacket &packet);
    };
}

#endif