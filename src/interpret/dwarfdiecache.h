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

#ifndef DWARFDIECACHE_H
#define DWARFDIECACHE_H

#include <elfutils/libdwfl.h>

#include <tsl/robin_map.h>

#include <algorithm>
#include <string>
#include <vector>

/// @return the demangled symbol name
std::string demangle(const std::string& mangledName);

struct DwarfRange
{
    Dwarf_Addr low;
    Dwarf_Addr high;

    bool contains(Dwarf_Addr addr) const
    {
        return low <= addr && addr < high;
    }
};

/// cache of dwarf ranges for a given Dwarf_Die
struct DieRanges
{
    Dwarf_Die die;
    std::vector<DwarfRange> ranges;

    bool contains(Dwarf_Addr addr) const
    {
        return std::any_of(ranges.begin(), ranges.end(),
                           [addr](const DwarfRange& range) { return range.contains(addr); });
    }
};

/// cache of sub program DIE, its ranges and the accompanying die name
class SubProgramDie
{
public:
    SubProgramDie() = default;
    SubProgramDie(Dwarf_Die die);
    ~SubProgramDie();

    bool isEmpty() const
    {
        return m_ranges.ranges.empty();
    }
    /// @p offset a bias-corrected offset
    bool contains(Dwarf_Addr offset) const
    {
        return m_ranges.contains(offset);
    }
    Dwarf_Die* die()
    {
        return &m_ranges.die;
    }

private:
    DieRanges m_ranges;
};

/// cache of dwarf ranges for a CU DIE and child sub programs
class CuDieRangeMapping
{
public:
    CuDieRangeMapping() = default;
    CuDieRangeMapping(Dwarf_Die cudie, Dwarf_Addr bias);
    ~CuDieRangeMapping();

    bool isEmpty() const
    {
        return m_cuDieRanges.ranges.empty();
    }
    bool contains(Dwarf_Addr addr) const
    {
        return m_cuDieRanges.contains(addr);
    }
    Dwarf_Addr bias()
    {
        return m_bias;
    }
    Dwarf_Die* cudie()
    {
        return &m_cuDieRanges.die;
    }

    /// On first call this will visit the CU DIE to cache all subprograms
    /// @return the DW_TAG_subprogram DIE that contains @p offset
    /// @p offset a bias-corrected address to find a subprogram for
    SubProgramDie* findSubprogramDie(Dwarf_Addr offset);

    /// @return a fully qualified, demangled symbol name for @p die
    const std::string &dieName(Dwarf_Die* die);

private:
    void addSubprograms();

    Dwarf_Addr m_bias = 0;
    DieRanges m_cuDieRanges;
    std::vector<SubProgramDie> m_subPrograms;
    tsl::robin_map<Dwarf_Off, std::string> m_dieNameCache;
};

/**
 * @return all DW_TAG_inlined_subroutine DIEs that contain @p offset
 * @p subprogram DIE sub tree that should be traversed to look for inlined scopes
 * @p offset bias-corrected address that is checked against the dwarf ranges of the DIEs
 */
std::vector<Dwarf_Die> findInlineScopes(Dwarf_Die* subprogram, Dwarf_Addr offset);

/**
 * This cache makes it easily possible to find a CU DIE (i.e. Compilation Unit Debugging Information Entry)
 * based on a
 */
class DwarfDieCache
{
public:
    DwarfDieCache(Dwfl_Module* mod = nullptr);
    ~DwarfDieCache();

    /// @p addr absolute address, not bias-corrected
    CuDieRangeMapping* findCuDie(Dwarf_Addr addr);

public:
    std::vector<CuDieRangeMapping> m_cuDieRanges;
};

#endif // DWARFDIECACHE_H
