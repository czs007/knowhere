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

#include <cstring>
#include <string>
#include <memory>
#include "Bitset2.h"
#include "BitsetView.h"

namespace faiss {

ConcurrentBitset2&
ConcurrentBitset2::operator&=(const ConcurrentBitset2& bitset) {
	auto u8_1 = mutable_data();
	auto u8_2 = bitset.data();
	auto u64_1 = reinterpret_cast<uint64_t*>(u8_1);
	auto u64_2 = reinterpret_cast<const uint64_t*>(u8_2);

	size_t n64 = bitset_.size();
//	size_t n64 = n8 / 8;

	for (size_t i = 0; i < n64; i++) {
	    u64_1[i] &= u64_2[i];
	}

	return *this;
}

ConcurrentBitset2&
ConcurrentBitset2::operator&=(const BitsetView& view) {
	auto u8_1 = mutable_data();
	auto u8_2 = view.data();
	auto u64_1 = reinterpret_cast<uint64_t*>(u8_1);
	auto u64_2 = reinterpret_cast<const uint64_t*>(u8_2);

	size_t n64 = bitset_.size();

	for (size_t i = 0; i < n64; i++) {
	    u64_1[i] &= u64_2[i];
	}

	return *this;
}

std::shared_ptr<ConcurrentBitset2>
ConcurrentBitset2::operator&(const ConcurrentBitset2& bitset) const {
auto result_bitset = std::make_shared<ConcurrentBitset2>(bitset.size());

//auto result_8 = 
auto result_64 = reinterpret_cast<uint64_t*>(result_bitset->mutable_data());

auto u64_1 = reinterpret_cast<const uint64_t*>(data());
auto u64_2 = reinterpret_cast<const uint64_t*>(bitset.data());

size_t n64 = bitset_.size();

for (size_t i = 0; i < n64; i++) {
    result_64[i] = u64_1[i] & u64_2[i];
}

return result_bitset;
}

std::shared_ptr<ConcurrentBitset2>
ConcurrentBitset2::operator&(const BitsetView& view) const {
auto result_bitset = std::make_shared<ConcurrentBitset2>(view.size());

auto result_64 = reinterpret_cast<uint64_t*>(result_bitset->mutable_data());

auto u64_1 = reinterpret_cast<const uint64_t*>(data());
auto u64_2 = reinterpret_cast<const uint64_t*>(view.data());

size_t n64 = bitset_.size();

for (size_t i = 0; i < n64; i++) {
    result_64[i] = u64_1[i] & u64_2[i];
}

return result_bitset;
}


ConcurrentBitset2&
ConcurrentBitset2::operator|=(const ConcurrentBitset2& bitset) {
auto u64_1 = reinterpret_cast<uint64_t*>(mutable_data());
auto u64_2 = reinterpret_cast<const uint64_t*>(bitset.data());

size_t n64 = bitset_.size();

for (size_t i = 0; i < n64; i++) {
    u64_1[i] |= u64_2[i];
}

return *this;
}

ConcurrentBitset2&
ConcurrentBitset2::operator|=(const BitsetView& view) {
auto u64_1 = reinterpret_cast<uint64_t*>(mutable_data());
auto u64_2 = reinterpret_cast<const uint64_t*>(view.data());

size_t n64 = bitset_.size();

for (size_t i = 0; i < n64; i++) {
    u64_1[i] |= u64_2[i];
}

return *this;
}


std::shared_ptr<ConcurrentBitset2>
ConcurrentBitset2::operator|(const ConcurrentBitset2& bitset) const {
auto result_bitset = std::make_shared<ConcurrentBitset2>(bitset.size());

auto result_64 = reinterpret_cast<uint64_t*>(result_bitset->mutable_data());

auto u64_1 = reinterpret_cast<const uint64_t*>(data());
auto u64_2 = reinterpret_cast<const uint64_t*>(bitset.data());

size_t n64 = bitset_.size();

for (size_t i = 0; i < n64; i++) {
    result_64[i] = u64_1[i] | u64_2[i];
}

return result_bitset;
}

std::shared_ptr<ConcurrentBitset2>
ConcurrentBitset2::operator|(const BitsetView& view) const {
auto result_bitset = std::make_shared<ConcurrentBitset2>(view.size());

auto result_64 = reinterpret_cast<uint64_t*>(result_bitset->mutable_data());

auto u64_1 = reinterpret_cast<const uint64_t*>(data());
auto u64_2 = reinterpret_cast<const uint64_t*>(view.data());

size_t n64 = bitset_.size();

for (size_t i = 0; i < n64; i++) {
    result_64[i] = u64_1[i] | u64_2[i];
}

return result_bitset;
}


ConcurrentBitset2&
ConcurrentBitset2::negate() {
auto u64_1 = reinterpret_cast<uint64_t*>(mutable_data());

size_t n64 = bitset_.size();

for (size_t i = 0; i < n64; i++) {
    u64_1[i] = ~u64_1[i];
}

return *this;
}

size_t
ConcurrentBitset2::count() const {
size_t ret = 0;
auto p_data = reinterpret_cast<const uint64_t *>(data());

auto len = size() >> 3;
auto popcount8 = [&](uint8_t x) -> int{
    x = (x & 0x55) + ((x >> 1) & 0x55);
    x = (x & 0x33) + ((x >> 2) & 0x33);
    x = (x & 0x0F) + ((x >> 4) & 0x0F);
    return x;
};

for (size_t i = 0; i < len; ++i) {
    ret += __builtin_popcountl(*p_data);
    p_data++;
}

auto p_byte = data() + (len << 3);
for (auto i = (len << 3); i < size(); ++i) {
    ret += popcount8(*p_byte);
    p_byte++;
}
return ret;
}

ConcurrentBitset2::operator std::string() const { 
    const char one = '1';
    const char zero = '0';
    const size_t len = size();
    std::string s;
    s.assign (len, zero);

    for (size_t i = 0; i < len; ++i) {
        if (test(id_type_t(i)))
            s.assign(len - 1 - i, one);
    }
    return s;
}

bool operator==(const ConcurrentBitset2& lhs, const ConcurrentBitset2& rhs) {
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

bool operator!=(const ConcurrentBitset2& lhs, const ConcurrentBitset2& rhs){ 
    return !(lhs == rhs); 
}

std::ostream& operator<<(std::ostream& os, const ConcurrentBitset2& bitset)
{
    os << std::string(bitset);
    return os;
}


}  // namespace faiss
