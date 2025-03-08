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

#include "ringbuffer.hpp"
#include "complex.hpp"

#include <sys/mman.h>

using namespace Csdr;

template <typename T>
Ringbuffer<T>::Ringbuffer(size_t size) {
    data = allocate_mirrored(size);
    if (data == nullptr) {
        throw BufferError("unable to allocate ringbuffer memory");
    }
}

template <typename T>
T* Ringbuffer<T>::allocate_mirrored(size_t size) {

#ifdef PAGESIZE
	static constexpr unsigned int PAGE_SIZE = PAGESIZE;
#else
	static const unsigned int PAGE_SIZE = ::sysconf(_SC_PAGESIZE);
#endif

    size_t bytes = ((sizeof(T) * size + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
    if (bytes % sizeof(T)) {
        throw BufferError("unable to align buffer with page size");
    }
    this->size = bytes / sizeof(T);

    #ifdef __APPLE__
        // no mremap() etc
    #else
        int counter = 10;
        while (counter-- > 0) {
            auto addr = static_cast<unsigned char*>(::mmap(NULL, 2 * bytes, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    
            if (addr == MAP_FAILED) {
                continue;
            }
    
            addr = static_cast<unsigned char*>(::mremap(addr, 2 * bytes, bytes, 0));
            if (addr == MAP_FAILED) {
                continue;
            }
    
            auto addr2 = static_cast<unsigned char*>(::mremap(addr, 0, bytes, MREMAP_FIXED | MREMAP_MAYMOVE, addr + bytes));
            if (addr2 == MAP_FAILED) {
                ::munmap(addr, bytes);
                continue;
            }
    
            if (addr2 != addr + bytes) {
                ::munmap(addr, bytes);
                ::munmap(addr2, bytes);
                continue;
            }
    
            return (T*) addr;
        }
    #endif

    return nullptr;
}

template <typename T>
Ringbuffer<T>::~Ringbuffer() {
    for (RingbufferReader<T>* reader : readers) {
        reader->onBufferDelete();
    }
    if (data != nullptr) {
        auto addr = (unsigned char*) data;
        size_t bytes = this->size * sizeof(T);
        ::munmap(addr, bytes);
        ::munmap(addr + bytes, bytes);
        data = nullptr;
    }
    unblock();
}

template <typename T>
size_t Ringbuffer<T>::writeable() {
    return size - 1;
}

template<typename T>
T* Ringbuffer<T>::getPointer(size_t pos) {
    return data + pos;
}

template <typename T>
T* Ringbuffer<T>::getWritePointer() {
    return getPointer(write_pos);
}

template <typename T>
void Ringbuffer<T>::advance(size_t& what, size_t how_much) {
    what = (what + how_much) % size;
}

template <typename T>
void Ringbuffer<T>::advance(size_t how_much) {
    advance(write_pos, how_much);
    unblock();
}

template <typename T>
size_t Ringbuffer<T>::available(size_t read_pos) {
    return (size + write_pos - read_pos) % size;
}

template<typename T>
size_t Ringbuffer<T>::getWritePos() {
    return write_pos;
}

template <typename T>
void Ringbuffer<T>::wait() {
    if (data == nullptr) {
        throw BufferError("Buffer is not initialized or shutting down, cannot wait()");
    }
    std::unique_lock<std::mutex> lk(mutex);
    printf("ringbuffer: wait\n");
    condition.wait(lk);
}

template <typename T>
void Ringbuffer<T>::unblock() {
    std::lock_guard<std::mutex> lk(mutex);
    //printf("ringbuffer: unblock\n");
    condition.notify_all();
}

template <typename T>
void Ringbuffer<T>::addReader(RingbufferReader<T> *reader) {
    if (readers.find(reader) != readers.end()) {
        // already in set
        return;
    }
    readers.insert(reader);
}

template <typename T>
void Ringbuffer<T>::removeReader(RingbufferReader<T> *reader) {
    auto position = readers.find(reader);
    if (position == readers.end()) {
        // not in set
        return;
    }
    readers.erase(position);
}

template <typename T>
RingbufferReader<T>::RingbufferReader(Ringbuffer<T>* buffer):
    buffer(buffer),
    read_pos(buffer->getWritePos())
{
    buffer->addReader(this);
}

template<typename T>
RingbufferReader<T>::~RingbufferReader() {
    if (buffer != nullptr) {
        buffer->removeReader(this);
    }
}

template <typename T>
size_t RingbufferReader<T>::available() {
    if (buffer == nullptr) {
        throw BufferError("Buffer no longer available");
    }
    return buffer->available(read_pos);
}

template <typename T>
T* RingbufferReader<T>::getReadPointer() {
    if (buffer == nullptr) {
        throw BufferError("Buffer no longer available");
    }
    return buffer->getPointer(read_pos);
}

template <typename T>
void RingbufferReader<T>::advance(size_t how_much) {
    if (buffer == nullptr) {
        throw BufferError("Buffer no longer available");
    }
    buffer->advance(read_pos, how_much);
}

template <typename T>
void RingbufferReader<T>::wait() {
    if (buffer == nullptr) {
        throw BufferError("Buffer no longer available");
    }
    buffer->wait();
}

template <typename T>
void RingbufferReader<T>::unblock() {
    if (buffer == nullptr) return;
    buffer->unblock();
}

template <typename T>
void RingbufferReader<T>::onBufferDelete() {
    buffer = nullptr;
}

namespace Csdr {
    // compile templates for all the possible variations
    template class Ringbuffer<char>;
    template class RingbufferReader<char>;

    template class Ringbuffer<unsigned char>;
    template class RingbufferReader<unsigned char>;

    template class Ringbuffer<short>;
    template class RingbufferReader<short>;

    template class Ringbuffer<float>;
    template class RingbufferReader<float>;

    template class Ringbuffer<complex<unsigned char>>;
    template class RingbufferReader<complex<unsigned char>>;

    template class Ringbuffer<complex<short>>;
    template class RingbufferReader<complex<short>>;

    template class Ringbuffer<complex<float>>;
    template class RingbufferReader<complex<float>>;
}