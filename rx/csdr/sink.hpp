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

namespace Csdr {

    class UntypedSink {
        public:
            virtual ~UntypedSink() = default;
            virtual bool hasReader() = 0;
    };

    template <typename T>
    class Sink: public UntypedSink {
        public:
            virtual void setReader(Reader<T>* reader);
            virtual Reader<T>* getReader();
            bool hasReader() override;
        protected:
            Reader<T>* reader = nullptr;
    };


}