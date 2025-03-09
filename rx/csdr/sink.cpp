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

#include "sink.hpp"

using namespace Csdr;

template <typename T>
void Sink<T>::setReader(Reader<T>* reader) {
    if (reader == this->reader) return;
    auto oldReader = this->reader;
    this->reader = reader;
    // if we had a reader before, there's a chance we're still wait()ing on it in a thread.
    // this makes sure we start reading on the new reader immediately.
    if (oldReader != nullptr) oldReader->unblock();
}

template <typename T>
Reader<T>* Sink<T>::getReader() {
    return reader;
}

template<typename T>
bool Sink<T>::hasReader() {
    return reader != nullptr;
}

namespace Csdr {
    template class Sink<short>;
    template class Sink<float>;
    template class Sink<complex<short>>;
    template class Sink<complex<float>>;
    template class Sink<unsigned char>;
    template class Sink<complex<unsigned char>>;
}