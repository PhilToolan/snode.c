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

#include "net/Config.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"
#include "utils/Config.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net {

    Config::Config(const std::string& name)
        : name(name) {
        serverSc = utils::Config::instance().add_subcommand(name, "Server configuration");
        serverSc->configurable();
    }

    void Config::finish() {
        serverConnectionSc = serverSc->add_subcommand("conn");
        serverConnectionSc->description("Options for established client connections");
        serverConnectionSc->configurable();

        serverConnectionSc->group("Sub-Options (use -h,--help on them)");

        serverConnectionReadTimeoutOpt = serverConnectionSc->add_option("-r,--read-timeout", readTimeout, "Read timeout");
        serverConnectionReadTimeoutOpt->type_name("[sec]");
        serverConnectionReadTimeoutOpt->default_val(60);
        serverConnectionReadTimeoutOpt->configurable();

        serverConnectionWriteTimeoutOpt = serverConnectionSc->add_option("-w,--write-timeout", writeTimeout, "Write timeout");
        serverConnectionWriteTimeoutOpt->type_name("[sec]");
        serverConnectionWriteTimeoutOpt->default_val(60);
        serverConnectionWriteTimeoutOpt->configurable();
    }

    const std::string& Config::getName() const {
        return name;
    }

    int Config::getReadTimeout() const {
        return readTimeout;
    }

    int Config::getWriteTimeout() const {
        return writeTimeout;
    }

} // namespace net
