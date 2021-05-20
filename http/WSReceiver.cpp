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

#include "http/WSReceiver.h"

#include "log/Logger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <endian.h> // for be16toh, be32toh, be64toh, htobe32
#include <iomanip>  // for operator<<, setfill, setw
#include <memory>   // for allocator
#include <sstream>  // for stringstream, basic_ostream, operator<<, bas...

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace http::websocket {

    void WSReceiver::receive(char* junk, std::size_t junkLen) {
        uint64_t consumed = 0;
        bool parsingError = false;

        // dumpFrame(junk, junkLen);

        while (consumed < junkLen && !parsingError) {
            switch (parserState) {
                case ParserState::BEGIN:
                    parserState = ParserState::OPCODE;
                    begin();
                    [[fallthrough]];
                case ParserState::OPCODE:
                    consumed += readOpcode(junk + consumed, junkLen - consumed);
                    break;
                case ParserState::LENGTH:
                    consumed += readLength(junk + consumed, junkLen - consumed);
                    break;
                case ParserState::ELENGTH:
                    consumed += readELength(junk + consumed, junkLen - consumed);
                    break;
                case ParserState::MASKINGKEY:
                    consumed += readMaskingKey(junk + consumed, junkLen - consumed);
                    break;
                case ParserState::PAYLOAD:
                    consumed += readPayload(junk + consumed, junkLen - consumed);
                    break;
                case ParserState::ERROR:
                    onError(errorState);
                    parsingError = true;
                    reset();
                    break;
            };
        }
    }

    uint64_t WSReceiver::readOpcode(char* junk, uint64_t junkLen) {
        uint64_t consumed = 0;
        if (junkLen > 0) {
            consumed++;

            uint8_t opCodeByte = *reinterpret_cast<uint8_t*>(junk);

            fin = opCodeByte & 0b10000000;
            opCode = opCodeByte & 0b00001111;

            if (!continuation) {
                onMessageStart(opCode);
                parserState = ParserState::LENGTH;
            } else if (opCode == 0) {
                parserState = ParserState::LENGTH;
            } else {
                parserState = ParserState::ERROR;
                errorState = 1002;
                VLOG(0) << "Error opcode in continuation frame";
            }
            continuation = !fin;
        }

        return consumed;
    }

    uint64_t WSReceiver::readLength(char* junk, uint64_t junkLen) {
        uint64_t consumed = 0;

        if (junkLen > 0) {
            uint8_t lengthByte = *reinterpret_cast<uint8_t*>(junk);

            masked = lengthByte & 0b10000000;
            length = lengthByte & 0b01111111;

            if (length > 125) {
                switch (length) {
                    case 126:
                        elengthNumBytes = 2;
                        break;
                    case 127:
                        elengthNumBytes = 8;
                        break;
                }
                parserState = ParserState::ELENGTH;
                length = 0;
            } else {
                if (masked) {
                    parserState = ParserState::MASKINGKEY;
                } else if (length > 0) {
                    parserState = ParserState::PAYLOAD;
                } else {
                    if (fin) {
                        onMessageEnd();
                    }
                    reset();
                }
            }
            consumed++;
        }

        return consumed;
    }

    uint64_t WSReceiver::readELength(char* junk, uint64_t junkLen) {
        uint64_t consumed = 0;

        elengthNumBytesLeft = (elengthNumBytesLeft == 0) ? elengthNumBytes : elengthNumBytesLeft;

        while (consumed < junkLen && elengthNumBytesLeft > 0) {
            length |= static_cast<uint64_t>(*reinterpret_cast<unsigned char*>(junk + consumed))
                      << (elengthNumBytes - elengthNumBytesLeft) * 8;

            consumed++;
            elengthNumBytesLeft--;
        }

        if (elengthNumBytesLeft == 0) {
            switch (elengthNumBytes) {
                case 2: {
                    length = be16toh(static_cast<uint16_t>(length));
                } break;
                case 8:
                    length = be64toh(length);
                    break;
            }

            if (length & static_cast<uint64_t>(0x01) << 63) {
                parserState = ParserState::ERROR;
                errorState = 1004;
            } else if (masked) {
                parserState = ParserState::MASKINGKEY;
            } else {
                parserState = ParserState::PAYLOAD;
            }
        }

        return consumed;
    }

    uint64_t WSReceiver::readMaskingKey(char* junk, uint64_t junkLen) {
        uint64_t consumed = 0;

        maskingKeyNumBytesLeft = (maskingKeyNumBytesLeft == 0) ? maskingKeyNumBytes : maskingKeyNumBytesLeft;

        while (consumed < junkLen && maskingKeyNumBytesLeft > 0) {
            maskingKey |= static_cast<uint32_t>(*reinterpret_cast<unsigned char*>(junk + consumed))
                          << (maskingKeyNumBytes - maskingKeyNumBytesLeft) * 8;
            consumed++;
            maskingKeyNumBytesLeft--;
        }

        if (maskingKeyNumBytesLeft == 0) {
            maskingKey = be32toh(maskingKey);
            if (length > 0) {
                parserState = ParserState::PAYLOAD;
            } else {
                if (fin) {
                    onMessageEnd();
                }
                reset();
            }
        }

        return consumed;
    }

    uint64_t WSReceiver::readPayload(char* junk, uint64_t junkLen) {
        uint64_t consumed = 0;

        if (junkLen > 0 && length - payloadRead > 0) {
            uint64_t numBytesToRead = (junkLen <= length - payloadRead) ? junkLen : length - payloadRead;

            MaskingKey maskingKeyAsArray = {.key = htobe32(maskingKey)};

            for (uint64_t i = 0; i < numBytesToRead; i++) {
                *(junk + i) = *(junk + i) ^ *(maskingKeyAsArray.keyAsArray + (i + payloadRead) % 4);
            }

            onFrameData(junk, numBytesToRead);

            payloadRead += numBytesToRead;
            consumed = numBytesToRead;
        }

        if (payloadRead == length) {
            if (fin) {
                onMessageEnd();
            }
            reset();
        }

        return consumed;
    }

    void WSReceiver::dumpFrame(char* frame, uint64_t frameLength) {
        int modul = 4;

        std::stringstream stringStream;

        for (std::size_t i = 0; i < frameLength; i++) {
            stringStream << std::setfill('0') << std::setw(2) << std::hex << (unsigned int) (unsigned char) frame[i] << " ";

            if ((i + 1) % modul == 0 || i == frameLength) {
                VLOG(0) << "Frame: " << stringStream.str();
                stringStream.str("");
            }
        }
    }

    void WSReceiver::reset() {
        payloadRead = 0;
        opCode = 0;
        parserState = ParserState::BEGIN;
        fin = false;
        masked = false;
        length = 0;
        elengthNumBytes = 0;
        elengthNumBytesLeft = 0;
        maskingKey = 0;
        maskingKeyNumBytes = 4;
        maskingKeyNumBytesLeft = 0;
        payloadRead = 0;
        errorState = 0;
    }

} // namespace http::websocket
