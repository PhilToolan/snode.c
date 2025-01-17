/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#ifndef NET_UN_STREAM_SOCKETCLIENT_H
#define NET_UN_STREAM_SOCKETCLIENT_H

#include "net/stream/SocketClient.h"    // IWYU pragma: export
#include "net/un/stream/ClientSocket.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string> // IWYU pragma: export

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::un::stream {

    template <typename ConfigT>
    class SocketClient : public net::stream::SocketClient<ConfigT, net::un::stream::ClientSocket> {
    private:
        using Super = net::stream::SocketClient<ConfigT, net::un::stream::ClientSocket>;

    protected:
        explicit SocketClient(const std::string& name);

    public:
        using Super::connect;

        using Super::Super;

        void connect(const std::string& sunPath, const std::function<void(const SocketAddress&, int)>& onError);
        void connect(const std::string& remoteSunPath,
                     const std::string& localSunPath,
                     const std::function<void(const SocketAddress&, int)>& onError);
    };

} // namespace net::un::stream

#endif // NET_UN_STREAM_SOCKETCLIENT_H
