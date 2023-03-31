// Copyright 2023 NXP

#ifndef P3COM_UTILITY_VECTOR_MAP_HPP
#define P3COM_UTILITY_VECTOR_MAP_HPP

#include "iceoryx_hoofs/cxx/vector.hpp"

namespace iox
{
namespace cxx
{
/**
 * @brief A minimal map implementation with static allocation. Useful when you need a `std::map`-like functionality
 * without the overhead, both in terms of boilerplate code and performance. A few things to keep in mind:
 *
 * 1) Keys are kept in a static vector and always linearly scanned when searching for a value. So, you dont need to
 * implement `std::hash<Key>` (like for `std::unordered_map`) or even `std::less<Key>` (like for `std::map`).
 *
 * 2) Since the keys are kept in a container separate from the values, they have very good spatial locality.
 *
 * 3) Especially if the key type is small and the number of keys is not too big, the linear scan can be faster than a
 * normal map lookup.
 */
template <typename Key, typename T, uint64_t Capacity>
class vector_map
{
  public:
    using key_type = Key;
    using mapped_type = T;
    using size_type = uint64_t;

    using iterator = mapped_type*;
    using const_iterator = const mapped_type*;

    iterator begin() noexcept;
    iterator end() noexcept;

    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;

    mapped_type& front() noexcept;
    mapped_type& back() noexcept;

    const mapped_type& front() const noexcept;
    const mapped_type& back() const noexcept;

    iterator find(const Key& key) noexcept;
    const_iterator find(const Key& key) const noexcept;

    template <typename... Targs>
    bool emplace(const Key& key, Targs&&... args) noexcept;

    void erase(iterator it) noexcept;
    void clear() noexcept;

    uint64_t size() const noexcept;

  private:
    iox::cxx::vector<Key, Capacity> m_keys;
    iox::cxx::vector<T, Capacity> m_elems;
};

} // namespace cxx
} // namespace iox

#include "p3com/internal/utility/vector_map.inl"

#endif
