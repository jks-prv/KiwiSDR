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

#include "reader.hpp"
#include "writer.hpp"

#include <cstdlib>
#include <stdexcept>
#include <unistd.h>
#include <mutex>
#include <condition_variable>
#include <set>

namespace Csdr {

    class BufferError: public std::runtime_error {
        public:
            explicit BufferError(const std::string& err): std::runtime_error(err) {}
    };

    template <typename T>
    class RingbufferReader;

    template <typename T>
    class Ringbuffer: public Writer<T> {
        public:
            explicit Ringbuffer<T>(size_t size);
            ~Ringbuffer() override;
            size_t writeable() override;
            T* getWritePointer() override;
            T* getPointer(size_t pos);
            void advance(size_t how_much) override;
            void advance(size_t& what, size_t how_much);
            size_t available(size_t read_pos);
            size_t getWritePos();
            void wait();
            void unblock();
            void addReader(RingbufferReader<T>* reader);
            void removeReader(RingbufferReader<T>* reader);
        private:
            T* allocate_mirrored(size_t size);
            T* data = nullptr;
            size_t size;
            size_t write_pos = 0;
            std::mutex mutex;
            std::condition_variable condition;
            std::set<RingbufferReader<T>*> readers = {};
    };

    template <typename T>
    class RingbufferReader: public Reader<T> {
        public:
            explicit RingbufferReader<T>(Ringbuffer<T>* buffer);
            ~RingbufferReader();
            size_t available() override;
            T* getReadPointer() override;
            void advance(size_t how_much) override;
            void wait() override;
            void unblock() override;
            void onBufferDelete();
        private:
            Ringbuffer<T>* buffer;
            size_t read_pos;
    };

}