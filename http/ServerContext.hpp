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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Logger.h"
#include "ServerContext.h"
#include "http_utils.h"
#include "socket/sock_stream/SocketConnectionBase.h"

namespace http {

    template <typename Request, typename Response>
    ServerContext<Request, Response>::ServerContext(SocketConnection* socketConnection,
                                                    const std::function<void(Request& req, Response& res)>& onRequestReady,
                                                    const std::function<void(Request& req, Response& res)>& onRequestCompleted)
        : socketConnection(socketConnection)
        , onRequestReady(onRequestReady)
        , onRequestCompleted(onRequestCompleted)
        , parser(
              [this](void) -> void {
                  VLOG(3) << "++ BEGIN:";

                  requestContexts.emplace_back(RequestContext(this));
              },
              [this](const std::string& method,
                     const std::string& url,
                     const std::string& httpVersion,
                     const std::map<std::string, std::string>& queries) -> void {
                  VLOG(3) << "++ Request: " << method << " " << url << " " << httpVersion;

                  Request& request = requestContexts.back().request;

                  request.method = method;
                  request.url = url;
                  request.queries = &queries;
                  request.httpVersion = httpVersion;
              },
              [this](const std::map<std::string, std::string>& header, const std::map<std::string, std::string>& cookies) -> void {
                  Request& request = requestContexts.back().request;

                  VLOG(3) << "++ Header:";
                  request.headers = &header;

                  VLOG(3) << "++ Cookies";
                  request.cookies = &cookies;

                  for (auto [field, value] : header) {
                      if (field == "connection" && value == "keep-alive") {
                          request.keepAlive = true;
                      }
                  }
              },
              [this](char* content, size_t contentLength) -> void {
                  VLOG(3) << "++ Content: " << contentLength;

                  Request& request = requestContexts.back().request;

                  request.body = content;
                  request.contentLength = contentLength;
              },
              [this, onRequestReady]() -> void {
                  VLOG(3) << "++ Parsed ++";

                  RequestContext& requestContext = requestContexts.back();

                  requestContext.request.extend();
                  requestContext.ready = true;

                  requestParsed();
              },
              [this](int status, const std::string& reason) -> void {
                  VLOG(3) << "++ Error: " << status << " : " << reason;

                  RequestContext& requestContext = requestContexts.back();

                  requestContext.status = status;
                  requestContext.reason = reason;
                  requestContext.ready = true;

                  requestParsed();
              }) {
    }

    template <typename Request, typename Response>
    ServerContext<Request, Response>::~ServerContext() {
        if (requestInProgress) {
            RequestContext& requestContext = requestContexts.front();

            onRequestCompleted(requestContext.request, requestContext.response);
        }
    }

    template <typename Request, typename Response>
    void ServerContext<Request, Response>::receiveRequestData(const char* junk, size_t junkLen) {
        parser.parse(junk, junkLen);
    }

    template <typename Request, typename Response>
    void ServerContext<Request, Response>::onReadError(int errnum) {
        if (errnum != 0 && errnum != ECONNRESET) {
            PLOG(ERROR) << "Connection read: " << errnum;
            reset();
        }
    }

    template <typename Request, typename Response>
    void ServerContext<Request, Response>::sendResponseData(const char* buf, size_t len) {
        socketConnection->enqueue(buf, len);
    }

    template <typename Request, typename Response>
    void ServerContext<Request, Response>::onWriteError(int errnum) {
        if (errnum != 0 && errnum != ECONNRESET) {
            PLOG(ERROR) << "Connection write: " << errnum;
            reset();
        }
    }

    template <typename Request, typename Response>
    void ServerContext<Request, Response>::requestParsed() {
        if (requestContexts.size() == 1) {
            RequestContext& requestContext = requestContexts.front();

            requestInProgress = true;
            if (requestContext.status == 0) {
                onRequestReady(requestContext.request, requestContext.response);
            } else {
                requestContext.response.status(requestContext.status).send(requestContext.reason);
                terminateConnection();
            }
        }
    }

    template <typename Request, typename Response>
    void ServerContext<Request, Response>::responseCompleted() {
        RequestContext& requestContext = requestContexts.front();

        onRequestCompleted(requestContext.request, requestContext.response);

        if (!requestContext.request.keepAlive && requestContext.response.keepAlive) {
            terminateConnection();
        } else {
            reset();

            requestContexts.pop_front();

            if (!requestContexts.empty() && requestContexts.front().ready) {
                RequestContext& requestContext = requestContexts.front();

                requestInProgress = true;
                if (requestContext.status == 0) {
                    onRequestReady(requestContext.request, requestContext.response);
                } else {
                    requestContext.response.status(requestContext.status).send(requestContext.reason);
                    terminateConnection();
                }
            }
        }
    }

    template <typename Request, typename Response>
    void ServerContext<Request, Response>::reset() {
        if (!requestContexts.empty()) {
            RequestContext& requestContext = requestContexts.front();
            requestContext.request.reset();
            requestContext.response.reset();
        }

        requestInProgress = false;
    }

    template <typename Request, typename Response>
    void ServerContext<Request, Response>::terminateConnection() {
        if (!connectionTerminated) {
            socketConnection->close();
            connectionTerminated = true;
        }

        reset();
    }

} // namespace http
