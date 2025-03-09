/*
Copyright (c) 2021 Jakob Ketterl <jakob.ketterl@gmx.de>

This file is part of libcsdr.

libcsdr is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

libcsdr is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with libcsdr.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "writer.hpp"

#include <arpa/inet.h>
#include <stdexcept>
#include <thread>

namespace Csdr {

    class UntypedSource {
        public:
            virtual ~UntypedSource() = default;
            virtual bool hasWriter() = 0;
    };

    template <typename T>
    class Source: public UntypedSource {
        public:
            virtual void setWriter(Writer<T>* writer);
            virtual Writer<T>* getWriter();
            bool hasWriter() override;
        protected:
            Writer<T>* writer = nullptr;
    };

    class NetworkException: public std::runtime_error {
        public:
            NetworkException(const std::string& reason): std::runtime_error(reason) {}
    };

#ifdef KIWISDR
#else
    template <typename T>
    class TcpSource: public Source<T> {
        public:
            // TcpSource(std::string remote);
            TcpSource(in_addr_t ip, unsigned short port);
            ~TcpSource();
            void setWriter(Writer<T>* writer) override;
            void stop();
        private:
            void loop();
            int sock;
            bool run = true;
            std::thread* thread = nullptr;
    };
#endif
}