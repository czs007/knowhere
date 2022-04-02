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
#include <string.h>
#include <vector>

namespace faiss {

class BitsetView;

class ConcurrentBitset {
 public:
    using id_type_t = int64_t;

    explicit ConcurrentBitset(size_t count, uint8_t init_value = 0)
    : count_(count), bitset_(((count + 8 - 1) >> 3)) {
        if (init_value) {
            memset(mutable_data(), init_value, (count + 8 - 1) >> 3);
        }
    }

    explicit ConcurrentBitset(size_t count, const uint8_t* data) : count_(count), bitset_(((count + 8 - 1) >> 3)) {
        memcpy(mutable_data(), data, (count + 8 - 1) >> 3);
    }

    ConcurrentBitset&
    operator&=(const ConcurrentBitset& bitset);

    ConcurrentBitset&
    operator&=(const BitsetView& view);

    std::shared_ptr<ConcurrentBitset>
    operator&(const ConcurrentBitset& bitset) const;

    std::shared_ptr<ConcurrentBitset>
    operator&(const BitsetView& view) const;

    ConcurrentBitset&
    operator|=(const ConcurrentBitset& bitset);

    ConcurrentBitset&
    operator|=(const BitsetView& view);

    std::shared_ptr<ConcurrentBitset>
    operator|(const ConcurrentBitset& bitset) const;

    std::shared_ptr<ConcurrentBitset>
    operator|(const BitsetView& view) const;

    ConcurrentBitset&
    negate();

    inline bool
    test(id_type_t id) {
        unsigned char mask = (unsigned char)(0x01) << (id & 0x07);
        return (bitset_[id >> 3].load() & mask);
    }

    inline void
    set(id_type_t id) {
        unsigned char mask = (unsigned char)(0x01) << (id & 0x07);
        bitset_[id >> 3].fetch_or(mask);
    }

    inline void
    clear(id_type_t id) {
        unsigned char mask = (unsigned char)(0x01) << (id & 0x07);
        bitset_[id >> 3].fetch_and(~mask);
    }

    inline size_t
    count() const {
        return count_;
    }

    inline size_t
    size() const {
        return ((count_ + 8 - 1) >> 3);
    }

    inline const uint8_t*
    data() const {
        return reinterpret_cast<const uint8_t*>(bitset_.data());
    }

    inline uint8_t*
    mutable_data() {
        return reinterpret_cast<uint8_t*>(bitset_.data());
    }

    uint64_t
    count_1();

 private:
    size_t count_;
    std::vector<std::atomic<uint8_t>> bitset_;
};

using ConcurrentBitsetPtr = std::shared_ptr<ConcurrentBitset>;

class BitsetView {
 public:
    BitsetView() = default;

    BitsetView(const uint8_t* blocks, int64_t count) : blocks_(blocks), count_(count) {
    }

    explicit BitsetView(const ConcurrentBitset& bitset) : blocks_(bitset.data()), count_(bitset.count()) {
    }

    BitsetView(const ConcurrentBitsetPtr& bitset_ptr) {
        if (bitset_ptr) {
            *this = BitsetView(*bitset_ptr);
        }
    }

    BitsetView(const std::nullptr_t nullptr_value): BitsetView() {
        assert(nullptr_value == nullptr);
    }

    inline bool
    empty() const {
        return count_ == 0;
    }

    // return count of all bits
    inline int64_t
    count() const {
        return count_;
    }

    // return count of all bits
    inline int64_t
    size() const {
        return count_;
    }


    // return sizeof bitmap in bytes
    inline int64_t
    u8size() const {
        return (count_ + 8 - 1) >> 3;
    }

    inline const uint8_t*
    data() const {
        return blocks_;
    }

    inline uint8_t*
    mutable_data() {
        return const_cast<uint8_t*>(blocks_);
    }


    inline bool
    test(int64_t index) const{
	auto block_id = index >> 3;
	auto block_offset = index & 0x7;
	return (blocks_[block_id] >> block_offset) & 0x1;
    }


    operator bool() const {
        return !empty();
    }

    uint64_t
    count_1() const;

 private:
    const uint8_t* blocks_ = nullptr;
    int64_t count_ = 0;  // count of bits
};

}  // namespace faiss
