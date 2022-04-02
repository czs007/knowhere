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
#include <iostream>
#include <atomic>
#include <memory>
#include <string.h>
#include <vector>

#include "Bitset.h"

namespace faiss {

class BitsetView {

 friend
 bool operator==(const BitsetView& lhs, const BitsetView& rhs);

 friend
 bool operator!=(const BitsetView& lhs, const BitsetView& rhs);

 friend
 std::ostream& operator<<(std::ostream& os, const BitsetView& view);

 public:
    BitsetView() = default;

    BitsetView(const uint8_t* blocks, int64_t size) : blocks_(blocks), size_(size) {
    }

    explicit BitsetView(const ConcurrentBitset& bitset) : blocks_(bitset.data()), size_(bitset.size()) {
    }

    BitsetView(const ConcurrentBitsetPtr& bitset_ptr) {
        if (bitset_ptr) {
            *this = BitsetView(*bitset_ptr);
        }
    }

    BitsetView(const std::nullptr_t nullptr_value): BitsetView() {
        assert(nullptr_value == nullptr);
    }

    bool
    empty() const;

    // return count of all 1-bits
    size_t
    count() const;

    // return count of all bits
    size_t
    size() const;

    // return sizeof bitmap in bytes
    size_t
    byte_size() const;

    const uint8_t*
    data() const;

    uint8_t*
    mutable_data();

    bool
    test(int64_t index) const;

    operator bool() const;
    operator std::string() const;

 private:
    const uint8_t* blocks_ = nullptr;
    size_t size_ = 0;  // count of bits
};

bool operator==(const BitsetView& lhs, const BitsetView& rhs);
bool operator!=(const BitsetView& lhs, const BitsetView& rhs);
std::ostream& operator<<(std::ostream& os, const BitsetView& view);


}  // namespace faiss
