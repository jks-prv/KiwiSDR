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

#include "complex.hpp"

#include <cstdlib>
#include <mutex>

namespace Csdr {

    // container class for template-agnostic storage
    class UntypedWriter {
        public:
            virtual ~UntypedWriter() = default;
            virtual size_t writeable() = 0;
            virtual void advance(size_t how_much) = 0;
    };

    template <typename T>
    class Writer: public UntypedWriter {
        public:
            virtual T* getWritePointer() = 0;
    };

    template <typename T>
    class StdoutWriter: public Writer<T> {
        public:
            StdoutWriter();
            StdoutWriter(size_t buffer_size);
            ~StdoutWriter();
            size_t writeable() override;
            T* getWritePointer() override;
            void advance(size_t how_much) override;
        private:
            size_t buffer_size;
            T* buffer;
    };

    template <typename T>
    class VoidWriter: public Writer<T> {
        public:
            explicit VoidWriter(size_t buffer_size);
            VoidWriter();
            ~VoidWriter();
            size_t writeable() override;
            T* getWritePointer() override;
            void advance(__attribute__((unused)) size_t how_much) override {}
        private:
            size_t buffer_size;
            T* data;
    };

}