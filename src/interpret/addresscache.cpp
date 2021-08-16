/*
 * Copyright 2021 Milian Wolff <mail@milianw.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "addresscache.h"

#include "dwarfdiecache.h"

#include <algorithm>

static bool operator<(const AddressCache::SymbolCacheEntry& lhs, const AddressCache::SymbolCacheEntry& rhs)
{
    return lhs.offset < rhs.offset;
}

static bool operator==(const AddressCache::SymbolCacheEntry& lhs, const AddressCache::SymbolCacheEntry& rhs)
{
    return lhs.offset == rhs.offset && lhs.size == rhs.size;
}

static bool operator<(const AddressCache::SymbolCacheEntry& lhs, uint64_t addr)
{
    return lhs.offset < addr;
}

bool AddressCache::hasSymbolCache(const std::string& filePath) const
{
    return m_symbolCache.contains(filePath);
}

AddressCache::SymbolCacheEntry AddressCache::findSymbol(const std::string& filePath, uint64_t relAddr)
{
    auto& symbols = m_symbolCache[filePath];
    auto it = std::lower_bound(symbols.begin(), symbols.end(), relAddr);

    // demangle symbols on demand instead of demangling all symbols directly
    // hopefully most of the symbols we won't ever encounter after all
    auto lazyDemangle = [](AddressCache::SymbolCacheEntry& entry) {
        if (!entry.demangled) {
            entry.symname = demangle(entry.symname);
            entry.demangled = true;
        }
        return entry;
    };

    if (it != symbols.end() && it->offset == relAddr)
        return lazyDemangle(*it);
    if (it == symbols.begin())
        return {};

    --it;

    if (it->offset <= relAddr && (it->offset + it->size > relAddr || (it->size == 0))) {
        return lazyDemangle(*it);
    }
    return {};
}

void AddressCache::setSymbolCache(const std::string& filePath, SymbolCache cache)
{
    /*
     * use stable_sort to produce results that are comparable to what addr2line would
     * return when we have entries like this in the symtab:
     *
     * 000000000045a130 l     F .text  0000000000000033 .hidden __memmove_avx_unaligned
     * 000000000045a180 l     F .text  00000000000003d8 .hidden __memmove_avx_unaligned_erms
     * 000000000045a180 l     F .text  00000000000003d8 .hidden __memcpy_avx_unaligned_erms
     * 000000000045a130 l     F .text  0000000000000033 .hidden __memcpy_avx_unaligned
     *
     * here, addr2line would always find the first entry. we want to do the same
     */

    std::stable_sort(cache.begin(), cache.end());
    cache.erase(std::unique(cache.begin(), cache.end()), cache.end());
    m_symbolCache[filePath] = std::move(cache);
}
