/*****************************************************************************
 *
 * Copyright 2022 NXP
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only
 * be used strictly in accordance with the applicable license terms. By
 * expressly accepting such terms or by downloading, installing, activating
 * and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If
 * you do not agree to be bound by the applicable license terms, then you may
 * not retain, install, activate or otherwise use the software.
 *
 ****************************************************************************/

#ifndef P3COM_UTILITY_VECTOR_MAP_INL
#define P3COM_UTILITY_VECTOR_MAP_INL

template <typename Key, typename T, uint64_t Capacity>
inline typename iox::cxx::vector_map<Key, T, Capacity>::const_iterator
iox::cxx::vector_map<Key, T, Capacity>::find(const Key& key) const noexcept
{
    const auto it = std::find(m_keys.begin(), m_keys.end(), key);
    if (it == m_keys.end())
    {
        return end();
    }

    const auto index = std::distance(m_keys.begin(), it);
    return begin() + index;
}

template <typename Key, typename T, uint64_t Capacity>
inline typename iox::cxx::vector_map<Key, T, Capacity>::iterator
iox::cxx::vector_map<Key, T, Capacity>::find(const Key& key) noexcept
{
    const auto it = std::find(m_keys.begin(), m_keys.end(), key);
    if (it == m_keys.end())
    {
        return end();
    }

    const auto index = std::distance(m_keys.begin(), it);
    return begin() + index;
}

template <typename Key, typename T, uint64_t Capacity>
template <typename... Targs>
inline bool iox::cxx::vector_map<Key, T, Capacity>::emplace(const Key& key, Targs&&... args) noexcept
{
    const bool emplaced = m_keys.push_back(key) && m_elems.emplace_back(std::forward<Targs>(args)...);
    return emplaced;
}

template <typename Key, typename T, uint64_t Capacity>
inline void iox::cxx::vector_map<Key, T, Capacity>::erase(iterator it) noexcept
{
    const auto index = std::distance(m_elems.begin(), it);
    m_keys.erase(m_keys.begin() + index);
    m_elems.erase(m_elems.begin() + index);
}

template <typename Key, typename T, uint64_t Capacity>
inline void iox::cxx::vector_map<Key, T, Capacity>::clear() noexcept
{
    m_keys.clear();
    m_elems.clear();
}

template <typename Key, typename T, uint64_t Capacity>
inline typename iox::cxx::vector_map<Key, T, Capacity>::iterator
iox::cxx::vector_map<Key, T, Capacity>::begin() noexcept
{
    return m_elems.begin();
}

template <typename Key, typename T, uint64_t Capacity>
inline typename iox::cxx::vector_map<Key, T, Capacity>::iterator iox::cxx::vector_map<Key, T, Capacity>::end() noexcept
{
    return m_elems.end();
}

template <typename Key, typename T, uint64_t Capacity>
inline typename iox::cxx::vector_map<Key, T, Capacity>::const_iterator
iox::cxx::vector_map<Key, T, Capacity>::begin() const noexcept
{
    return m_elems.begin();
}

template <typename Key, typename T, uint64_t Capacity>
inline typename iox::cxx::vector_map<Key, T, Capacity>::const_iterator
iox::cxx::vector_map<Key, T, Capacity>::end() const noexcept
{
    return m_elems.end();
}

template <typename Key, typename T, uint64_t Capacity>
inline typename iox::cxx::vector_map<Key, T, Capacity>::mapped_type&
iox::cxx::vector_map<Key, T, Capacity>::front() noexcept
{
    return m_elems.front();
}

template <typename Key, typename T, uint64_t Capacity>
inline typename iox::cxx::vector_map<Key, T, Capacity>::mapped_type&
iox::cxx::vector_map<Key, T, Capacity>::back() noexcept
{
    return m_elems.back();
}

template <typename Key, typename T, uint64_t Capacity>
inline const typename iox::cxx::vector_map<Key, T, Capacity>::mapped_type&
iox::cxx::vector_map<Key, T, Capacity>::front() const noexcept
{
    return m_elems.front();
}

template <typename Key, typename T, uint64_t Capacity>
inline const typename iox::cxx::vector_map<Key, T, Capacity>::mapped_type&
iox::cxx::vector_map<Key, T, Capacity>::back() const noexcept
{
    return m_elems.back();
}

template <typename Key, typename T, uint64_t Capacity>
inline uint64_t iox::cxx::vector_map<Key, T, Capacity>::size() const noexcept
{
    return m_elems.size();
}

#endif
