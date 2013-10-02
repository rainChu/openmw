#include "network.hpp"

namespace MWWorld
{
    void Network::reportCharacterMovement( const CharacterMovementPacket &packet)
    {
        std::cout << "Moving " << packet.mRefNum << std::endl;
    }
}