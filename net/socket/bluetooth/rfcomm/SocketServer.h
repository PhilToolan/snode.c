/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NET_SOCKET_BLUETOOTH_RFCOMM_SOCKETSERVER_H
#define NET_SOCKET_BLUETOOTH_RFCOMM_SOCKETSERVER_H

#include "net/socket/bluetooth/rfcomm/Socket.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <functional>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::socket::bluetooth::rfcomm {

    template <template <typename SocketT, typename SocketContextFactoryT> typename ConcreteSocketServer, typename SocketContextFactoryT>
    class SocketServer : public ConcreteSocketServer<rfcomm::Socket, SocketContextFactoryT> {
        using ConcreteSocketServer<rfcomm::Socket, SocketContextFactoryT>::ConcreteSocketServer;

    public:
        using SocketAddress = typename ConcreteSocketServer<rfcomm::Socket, SocketContextFactoryT>::SocketAddress;

        using ConcreteSocketServer<rfcomm::Socket, SocketContextFactoryT>::listen;

        void listen(uint8_t channel, int backlog, const std::function<void(int)>& onError) {
            listen(SocketAddress(channel), backlog, onError);
        }

        void listen(const std::string& address, int backlog, const std::function<void(int)>& onError) {
            listen(SocketAddress(address), backlog, onError);
        }

        void listen(const std::string& address, uint8_t channel, int backlog, const std::function<void(int)>& onError) {
            listen(SocketAddress(address, channel), backlog, onError);
        }
    };

} // namespace net::socket::bluetooth::rfcomm

#endif // NET_SOCKET_BLUETOOTH_RFCOMM_SOCKETSERVER_H
