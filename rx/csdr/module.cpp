/*
Copyright (c) 2021-2023 Jakob Ketterl <jakob.ketterl@gmx.de>

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

#include "module.hpp"

#include <algorithm>

using namespace Csdr;

template <typename T, typename U>
Module<T, U>::~Module() {
    std::lock_guard<std::mutex> lock(processMutex);
}

template <typename T, typename U>
void Module<T, U>::wait(std::unique_lock<std::mutex>& lock) {
    waitingReader = this->getReader();

    // we are in a consistent state, so we can unlock during the blocking op
    lock.unlock();
    waitingReader->wait();
    lock.lock();

    waitingReader = nullptr;
}

template <typename T, typename U>
void Module<T, U>::unblock() {
    auto r = waitingReader;
    if (r != nullptr) {
        r->unblock();
    }
}

template <typename T, typename U>
void Module<T, U>::setWriter(Writer<U> *writer) {
    std::lock_guard<std::mutex> lock(processMutex);
    Source<U>::setWriter(writer);
}

template <typename T, typename U>
void Module<T, U>::setReader(Reader<T> *reader) {
    std::lock_guard<std::mutex> lock(processMutex);
    Sink<T>::setReader(reader);
    if (reader != waitingReader) waitingReader = nullptr;
}

template <typename T, typename U>
bool AnyLengthModule<T, U>::canProcess() {
    std::lock_guard<std::mutex> lock(this->processMutex);
    return getWorkSize() > 0;
}

template <typename T, typename U>
size_t AnyLengthModule<T, U>::getWorkSize() {
    return std::min({this->reader->available(), this->writer->writeable(), maxLength()});
}

template <typename T, typename U>
void AnyLengthModule<T, U>::process() {
    std::lock_guard<std::mutex> lock(this->processMutex);
    size_t available = getWorkSize();
    process(this->reader->getReadPointer(), this->writer->getWritePointer(), available);
    this->reader->advance(available);
    this->writer->advance(available);
}

template <typename T, typename U>
bool FixedLengthModule<T, U>::canProcess() {
    std::lock_guard<std::mutex> lock(this->processMutex);
    size_t length = getLength();
    return (this->reader->available() > length && this->writer->writeable() > length);
}

template <typename T, typename U>
void FixedLengthModule<T, U>::process () {
    std::lock_guard<std::mutex> lock(this->processMutex);
    size_t length = getLength();
    process(this->reader->getReadPointer(), this->writer->getWritePointer());
    this->reader->advance(length);
    this->writer->advance(length);
}

namespace Csdr {
    template class Module<short, short>;
    template class Module<float, float>;
    template class Module<complex<float>, float>;
    template class Module<short, float>;
    template class Module<float, short>;
    template class Module<complex<float>, complex<float>>;
    template class Module<short, unsigned char>;
    template class Module<unsigned char, short>;
    template class Module<float, unsigned char>;
    template class Module<unsigned char, float>;
    template class Module<complex<float>, unsigned char>;
    template class Module<unsigned char, unsigned char>;
    template class Module<complex<float>, complex<short>>;
    template class Module<complex<short>, complex<float>>;
    template class Module<complex<short>, short>;
    template class Module<complex<short>, unsigned char>;
    template class Module<complex<float>, complex<unsigned char>>;
    template class Module<complex<unsigned char>, complex<float>>;
    template class Module<complex<unsigned char>, short>;

    template class AnyLengthModule<short, short>;
    template class AnyLengthModule<float, float>;
    template class AnyLengthModule<complex<float>, float>;
    template class AnyLengthModule<short, float>;
    template class AnyLengthModule<float, short>;
    template class AnyLengthModule<unsigned char, float>;
    template class AnyLengthModule<float, unsigned char>;
    template class AnyLengthModule<complex<float>, complex<float>>;
    template class AnyLengthModule<complex<float>, unsigned char>;
    template class AnyLengthModule<complex<float>, complex<short>>;
    template class AnyLengthModule<complex<short>, complex<float>>;
    template class AnyLengthModule<complex<float>, complex<unsigned char>>;
    template class AnyLengthModule<complex<unsigned char>, complex<float>>;

    template class FixedLengthModule<float, float>;
    template class FixedLengthModule<complex<float>, complex<float>>;
}
