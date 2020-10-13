/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef NET_SOCKET_SOCK_STREAM_TLS_SOCKETREADER_H
#define NET_SOCKET_SOCK_STREAM_TLS_SOCKETREADER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <openssl/ssl.h>
#include <sys/types.h> // for ssize_t

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/sock_stream/SocketReader.h"

// IWYU pragma: no_forward_declare tls::Socket

namespace net::socket::stream::tls {

    template <typename SocketT>
    class SocketReader : public socket::stream::SocketReader<SocketT> {
    protected:
        using Socket = SocketT;

        using socket::stream::SocketReader<Socket>::SocketReader;

        ssize_t read(char* junk, size_t junkLen) override {
            return ::SSL_read(ssl, junk, junkLen);
        }

    protected:
        SSL* ssl = nullptr;
    };

}; // namespace net::socket::stream::tls

#endif // NET_SOCKET_SOCK_STREAM_TLS_SOCKETREADER_H
