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
#include <atomic>
#include <memory>
#include <iostream>
#include <string.h>
#include <vector>

namespace faiss {

class BitsetView;

class ConcurrentBitset2 {

 friend
 bool operator==(const ConcurrentBitset2& lhs, const ConcurrentBitset2& rhs);

 friend
 bool operator!=(const ConcurrentBitset2& lhs, const ConcurrentBitset2& rhs);

 friend
 std::ostream& operator<<(std::ostream& os, const ConcurrentBitset2& bitset);

 public:
    using id_type_t = int64_t;

    explicit ConcurrentBitset2(size_t size, uint8_t init_value = 0)
    : size_(size), bitset_(((size + 64 - 1) >> 6)) {
        if (init_value) {
            memset(mutable_data(), init_value, (size_ + 64 - 1) >> 6);
        }
    }

    explicit ConcurrentBitset2(size_t size, const uint8_t* data) : size_(size), bitset_(((size + 64 - 1) >> 6)) {
        memcpy(mutable_data(), data, (size_ + 64 - 1) >> 6);
    }

    ConcurrentBitset2&
    operator&=(const ConcurrentBitset2& bitset);

    ConcurrentBitset2&
    operator&=(const BitsetView& view);

    std::shared_ptr<ConcurrentBitset2>
    operator&(const ConcurrentBitset2& bitset) const;

    std::shared_ptr<ConcurrentBitset2>
    operator&(const BitsetView& view) const;

    ConcurrentBitset2&
    operator|=(const ConcurrentBitset2& bitset);

    ConcurrentBitset2&
    operator|=(const BitsetView& view);

    std::shared_ptr<ConcurrentBitset2>
    operator|(const ConcurrentBitset2& bitset) const;

    std::shared_ptr<ConcurrentBitset2>
    operator|(const BitsetView& view) const;

    ConcurrentBitset2&
    negate();

    inline bool
    test(id_type_t id) const {
        uint64_t mask = (uint8_t)(0x01) << (id & 0x3f);
        return (bitset_[id >> 6] & mask);
    }

    inline void
    set(id_type_t id) {
        uint64_t mask = (uint8_t)(0x01) << (id & 0x3f);
        bitset_[id >> 6] |= (mask);
    }

    inline void
    clear(id_type_t id) {
        uint64_t mask = (uint8_t)(0x01) << (id & 0x3f);
        bitset_[id >> 6] &= ~mask;
    }

    size_t
    count() const ;

    inline size_t
    size() const {
        return size_;
    }

    inline size_t
    byte_size() const {
        return ((size_ + 64 - 1) >> 6);
    }

    inline const uint8_t*
    data() const {
        return reinterpret_cast<const uint8_t*>(bitset_.data());
    }

    inline uint8_t*
    mutable_data() {
        return reinterpret_cast<uint8_t*>(bitset_.data());
    }

    operator std::string() const;

 private:
    size_t size_; // number of bits
    std::vector<std::atomic<uint64_t>> bitset_;
};

bool operator==(const ConcurrentBitset2& lhs, const ConcurrentBitset2& rhs);
bool operator!=(const ConcurrentBitset2& lhs, const ConcurrentBitset2& rhs);
std::ostream& operator<<(std::ostream& os, const ConcurrentBitset2& bitset);

using ConcurrentBitset2Ptr = std::shared_ptr<ConcurrentBitset2>;

}  // namespace faiss
