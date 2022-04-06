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

#include <assert.h>
#include <cstring>
#include <atomic>
#include <memory>
#include <vector>
#include "BitsetView.h"

namespace faiss {

    bool
    BitsetView::empty() const {
        return size_ == 0;
    }

    // return count of all bits
    size_t
    BitsetView::size() const {
        return size_;
    }


    // return sizeof bitmap in bytes
    size_t
    BitsetView::byte_size() const {
        return (size_ + 8 - 1) >> 3;
    }

    const uint8_t*
    BitsetView::data() const {
        return blocks_;
    }

    uint8_t*
    BitsetView::mutable_data() {
        return const_cast<uint8_t*>(blocks_);
    }

    bool
    BitsetView::test(int64_t index) const {
	auto block_id = index >> 3;
	auto block_offset = index & 0x7;
	return (blocks_[block_id] >> block_offset) & 0x1;
    }

    BitsetView::operator bool() const {
        return !empty();
    }

    size_t
    BitsetView::count() const {
	size_t ret = 0;
	auto p_data = reinterpret_cast<const uint64_t *>(blocks_);
	auto len = size_ >> 6;
	//auto remainder = size() % 8;
	auto popcount8 = [&](uint8_t x) -> int{
	    x = (x & 0x55) + ((x >> 1) & 0x55);
	    x = (x & 0x33) + ((x >> 2) & 0x33);
	    x = (x & 0x0F) + ((x >> 4) & 0x0F);
	    return x;
	};
	for (int64_t i = 0; i < len; ++i) {
	    ret += __builtin_popcountl(*p_data);
	    p_data++;
	}
	auto p_byte = blocks_ + (len << 3);
	for (auto i = (len << 3); i < byte_size(); ++i) {
	    ret += popcount8(*p_byte);
	    p_byte++;
	}
	return ret;
     }


BitsetView::operator std::string() const { 
    const char one = '1';
    const char zero = '0';
    const size_t len = size();
    std::string s;
    s.assign(len, zero);

    for (size_t i = 0; i < len; ++i) {
        if (test(i))
		s[len - 1 -i] = one;
    }
    return s;
}

bool operator==(const BitsetView& lhs, const BitsetView& rhs) {
    if (std::addressof(lhs) == std::addressof(rhs)){
	return true;
    }

    if (lhs.size() != rhs.size()){
	return false;
    }

    if (lhs.byte_size() != rhs.byte_size()){
	return false;
    }


    auto ret = std::memcmp(lhs.data(), rhs.data(), lhs.byte_size());
    return ret == 0;
}

bool operator!=(const BitsetView& lhs, const BitsetView& rhs){ 
    return !(lhs == rhs); 
}

std::ostream& operator<<(std::ostream& os, const BitsetView& bitset)
{
    os << std::string(bitset);
    return os;
}


}  // namespace faiss
