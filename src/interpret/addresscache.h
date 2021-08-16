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

#ifndef ADDRESSCACHE_H
#define ADDRESSCACHE_H

#include <tsl/robin_map.h>

#include <string>
#include <vector>

// #include "perfelfmap.h"

class AddressCache
{
public:
    struct AddressCacheEntry
    {
        AddressCacheEntry(int locationId = -1, bool isInterworking = false)
            : locationId(locationId)
            , isInterworking(isInterworking)
        {
        }
        bool isValid() const
        {
            return locationId >= 0;
        }
        int locationId;
        bool isInterworking;
    };
    using OffsetAddressCache = tsl::robin_map<uint64_t, AddressCacheEntry>;

    struct SymbolCacheEntry
    {
        SymbolCacheEntry(uint64_t offset = 0, uint64_t value = 0, uint64_t size = 0, const std::string& symname = {})
            : offset(offset)
            , value(value)
            , size(size)
            , symname(symname)
        {
        }

        bool isValid() const
        {
            return !symname.empty();
        }

        // adjusted/absolute st_value, see documentation of the `addr` arg in `dwfl_module_getsym_info`
        uint64_t offset;
        // unadjusted/relative st_value
        uint64_t value;
        uint64_t size;
        std::string symname;
        bool demangled = false;
    };
    using SymbolCache = std::vector<SymbolCacheEntry>;

    /// check if @c setSymbolCache was called for @p filePath already
    bool hasSymbolCache(const std::string& filePath) const;
    /// take @p cache, sort it and use it for symbol lookups in @p filePath
    void setSymbolCache(const std::string& filePath, SymbolCache cache);
    /// find the symbol that encompasses @p relAddr in @p filePath
    /// if the found symbol wasn't yet demangled, it will be demangled now
    SymbolCacheEntry findSymbol(const std::string& filePath, uint64_t relAddr);

private:
    tsl::robin_map<std::string, OffsetAddressCache> m_cache;
    tsl::robin_map<std::string, SymbolCache> m_symbolCache;
};

#endif // ADDRESSCACHE_H
