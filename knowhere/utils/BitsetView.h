// Copyright (C) 2019-2020 Zilliz. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance
// with the License. You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied. See the License for the specific language governing permissions and limitations under the License.

#pragma once

#include <assert.h>
#include <memory>
#include <sstream>
#include <string>

namespace faiss {

inline void
set_bit(uint8_t* data, size_t idx) {
    data[idx / 8] |= 0x1 << (idx % 8);
}

inline void
clear_bit(uint8_t* data, size_t idx) {
    data[idx / 8] &= ~(0x1 << (idx % 8));
}

class BitsetView {
 public:
    BitsetView() = default;
    ~BitsetView() = default;

    BitsetView(const uint8_t* data, size_t num_bits) : bits_(data), num_bits_(num_bits) {
    }

    BitsetView(const std::nullptr_t value): BitsetView() {
        assert(value == nullptr);
    }

    bool
    empty() const {
        return num_bits_ == 0;
    }

    size_t
    size() const {
        return num_bits_;
    }

    size_t
    byte_size() const {
        return (num_bits_ + 8 - 1) / 8;
    }

    const uint8_t*
    data() const {
        return bits_;
    }

    bool
    test(int64_t idx) const {
        return bits_[idx / 8] & (0x1 << (idx % 8));
    }

    size_t
    count() const {
        size_t ret = 0;
        auto len_uint8 = byte_size();
        auto len_uint64 = len_uint8 / 8;

        auto popcount8 = [&](uint8_t x) -> int {
            x = (x & 0x55) + ((x >> 1) & 0x55);
            x = (x & 0x33) + ((x >> 2) & 0x33);
            x = (x & 0x0F) + ((x >> 4) & 0x0F);
            return x;
        };

        for (int64_t i = 0; i < len_uint64; i++) {
            ret += __builtin_popcountl(*(uint64_t*)(bits_ + i * 8));
        }
        for (int64_t i = len_uint64 * 8; i < len_uint8; i++) {
            ret += popcount8(*(bits_ + i));
        }
        return ret;
    }

    std::string
    to_string() const {
        std::stringbuf buf;
        for (size_t i = 0; i < size(); i++) {
            buf.sputc(test(i) ? '1' : '0');
        }
        return buf.str();
    }

 private:
    const uint8_t* bits_ = nullptr;
    size_t num_bits_ = 0;
};

using BitsetViewPtr = std::shared_ptr<BitsetView>;

}  // namespace faiss
