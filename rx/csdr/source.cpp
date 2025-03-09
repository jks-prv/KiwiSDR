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

#include "source.hpp"

#include <cstring>
#include <unistd.h>
#include <poll.h>

using namespace Csdr;

template <typename T>
void Source<T>::setWriter(Writer<T>* writer) {
    this->writer = writer;
}

template <typename T>
Writer<T>* Source<T>::getWriter() {
    return writer;
}

template<typename T>
bool Source<T>::hasWriter() {
    return writer != nullptr;
}

#ifdef KIWISDR
#else
template<typename T>
TcpSource<T>::~TcpSource() {
    ::close(sock);
}

template <typename T>
TcpSource<T>::TcpSource(in_addr_t ip, unsigned short port) {
    sockaddr_in remote{};
    std::memset(&remote, 0, sizeof(remote));

    remote.sin_family = AF_INET;
    remote.sin_port = htons(port);
    remote.sin_addr.s_addr = ip;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        throw NetworkException("unable to create socket");
    }

    if (connect(sock, (struct sockaddr*)&remote, sizeof(remote)) < 0) {
        ::close(sock);
        throw NetworkException("connection failed");
    }

}

template <typename T>
void TcpSource<T>::setWriter(Writer<T> *writer) {
    Source<T>::setWriter(writer);
    if (thread == nullptr) {
        #ifdef KIWISDR
            // no pthreads
        #else
            thread = new std::thread( [this] () { loop(); });
        #endif
    }
}

template <typename T>
void TcpSource<T>::loop() {
    int read_bytes;
    int available;
    int offset = 0;
    pollfd pfd = {
        .fd = sock,
        .events = POLLIN
    };
    int rc;

    while (run) {
        rc = poll(&pfd, 1, 10000);
        if (rc == -1) {
            run = false;
        } else if (pfd.revents & POLLERR) {
            run = false;
        } else if (pfd.revents & POLLIN) {
            available = std::min(this->writer->writeable(), (size_t) 1024) * sizeof(T) - offset;
            read_bytes = recv(sock, ((char*) this->writer->getWritePointer()) + offset, available, 0);
            if (read_bytes <= 0) {
                run = false;
            } else {
                this->writer->advance((offset + read_bytes) / sizeof(T));
                offset = (offset + read_bytes) % sizeof(T);
            }
        }
    }
}

template <typename T>
void TcpSource<T>::stop() {
    run = false;
    if (thread != nullptr) {
        auto t = thread;
        thread = nullptr;
        t->join();
        delete(t);
    }
}
#endif

namespace Csdr {
    template class Source<float>;
    template class Source<short>;
    template class Source<complex<short>>;
    template class Source<complex<float>>;
    template class Source<unsigned char>;
    template class Source<complex<unsigned char>>;

#ifdef KIWISDR
#else
    template class TcpSource<unsigned char>;
    template class TcpSource<short>;
    template class TcpSource<float>;
    template class TcpSource<complex<short>>;
    template class TcpSource<complex<float>>;
    template class TcpSource<complex<unsigned char>>;
#endif
}