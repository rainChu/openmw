#include "network.hpp"

namespace MWWorld
{
    void Network::reportCharacterMovement( const CharacterMovementPacket &packet)
    {
        std::cout << "Moving " << packet.mRefNum << std::endl;
    }

    bool Network::connect(const std::string &address, const std::string &slotPassword)
    {
        std::cout << "I was called to connect to <" << address;
        std::cout << "> with the password '" << slotPassword << "'." << std::endl;

        return false;
    }
}