#ifndef GAME_MWNETWORK_PACKET_H
#define GAME_MWNETWORK_PACKET_H

#include "../mwmechanics/character.hpp"

namespace MWNetwork
{
    /// \fixme Don't expose Packet types as public, just use them internally.
    struct CharacterMovementPayload
    {
        /// \fixme can't we identify by endpoint instead?
        char password[32];

        MWMechanics::CharacterState movementState;

        ESM::Position movement;

        /// \todo make these into an ESM::Position
        float currentPosition[3];
        float angle[3];
    };

    struct PuppetInfo
    {
        char password[32];

        /// \fixme use my own struct, movement isn't an ESM::Position anyways.
        ESM::Position movement;

        bool isExterior;

        /// \fixme find out the actual max length of a cell name
        char cellName[64];
        ESM::Position position;

        // Don't try to substute numbers to make the strings shorter. This packet
        // Isn't sent often, and assuming the mesh name will obey a format just
        // makes some mods adding new hairstyles, etc. incompatible with me.
        char model[48];
        char head[48];
        char hair[48];

        char race[32];
        char npcClass[32];
    };

    /// \fixme Don't need a union. All packets don't have to be huge just because some are.
    /// \fixme don't assume ESM::Position won't change, create a packed struct for both movement settings and position

    struct Packet
    {
        enum MessageCode
        {
            /// There is no puppet on the server with the given name
            WrongPassword
        };

        enum Type
        {
            CharacterMovement,
            NewClient,
            AcceptClient,
            OtherMessage
        };
        Type type;

        /// \brief The clock() of the sending computer.
        /// Never compare these against different machines.
        clock_t timestamp;

        /// \fixme don't use a union, no reason to make all packets the
        /// same size.
        union
        {
            /// Sent on a typical client update.
            CharacterMovementPayload characterMovement;

            /// Sent when a new player arrives
            PuppetInfo newPuppet;
                
            /// Sent from the client to request connection
            struct
            {
                /// \todo all this char stuff is nasty! Serialize it.
                char password[32];
            } newClient;

            /// Sent from the server when a new client is accepted.
            struct
            {
                PuppetInfo hostPuppet;
                PuppetInfo clientPuppet;
            } acceptClient;

                

            MessageCode messageCode;
        } payload;
    };
}

#endif