#ifndef GAME_MWNETWORK_CLIENT_H
#define GAME_MWNETWORK_CLIENT_H

#include "service.hpp"

namespace MWNetwork
{
    class Client :
        public Service
    {
        Client(const Client &); // not implemented
        void operator=(const Client &); // not implemented
    public:
        /// \brief Maintains a UDP connection to a Server.
        /// \param address The endpoint url packets should be addressed to.
        /// \param contactRequest A packet of NewClient type to make the first contact with.
        Client(Network &network, const std::string &address, const Packet &contactRequest);
        ~Client();

        void onReceiveAccept( const boost::system::error_code &e,
                              size_t bytesTransferred,
                              boost::asio::deadline_timer *timeout);

    private:
        boost::asio::ip::udp::endpoint mMyEndpoint;

        bool mAccepted;

        std::string mPassword;

        void interpretNewClientPacket(const Packet &packet){}

        void sendPacket(Packet &packet);
        void sendPlayerMovementPacket();
        void createPuppetNpc(const PuppetInfo &puppetInfo);
    };
}

#endif