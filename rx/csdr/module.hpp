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

#include "source.hpp"
#include "sink.hpp"
#include "complex.hpp"

#include <cstdint>
#include <mutex>

namespace Csdr {

    class UntypedModule {
        public:
            virtual ~UntypedModule() = default;
            virtual bool canProcess() = 0;
            virtual void process() = 0;
            virtual void wait(std::unique_lock<std::mutex>& lock) = 0;
            virtual void unblock() = 0;
    };

    template <typename T, typename U>
    class Module: public UntypedModule, public Sink<T>, public Source<U> {
        public:
            ~Module() override;
            void wait(std::unique_lock<std::mutex>& lock) override;
            void unblock() override;
            void setWriter(Writer<U>* writer) override;
            void setReader(Reader<T>* reader) override;
        protected:
            std::mutex processMutex;
        private:
            Reader<T>* waitingReader = nullptr;
    };

    template <typename T, typename U>
    class AnyLengthModule: public Module<T, U> {
        public:
            bool canProcess() override;
            void process() override;
        protected:
            virtual void process(T* input, U* output, size_t len) = 0;
            virtual size_t maxLength() { return SIZE_MAX; }
            size_t getWorkSize();
    };

    template <typename T, typename U>
    class FixedLengthModule: public Module<T, U> {
        public:
            bool canProcess() override;
            void process() override;
        protected:
            virtual void process(T* input, U* output) = 0;
            virtual size_t getLength() = 0;
    };
}