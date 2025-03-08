/*
Copyright (c) 2021-2022 Jakob Ketterl <jakob.ketterl@gmx.de>

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

#include <complex>

namespace Csdr {

    template <typename T>
    class complex: public std::complex<T> {
        public:
            complex(const T& i = T(), const T& q = T()): std::complex<T>(i, q) {}
            complex(const std::complex<T>& c): complex(c.real(), c.imag()) {}
            // in-phase
            T i() const { return std::complex<T>::real(); }
            void i(T value) { std::complex<T>::real(value); }
            // quadrature
            T q() const { return std::complex<T>::imag(); }
            void q(T value) { std::complex<T>::imag(value); }
    };

}
