#pragma once

#include <dwarf.h>

namespace cppdwarf {
enum class form {
    addr = DW_FORM_addr,
    block2 = DW_FORM_block2,
    block4 = DW_FORM_block4,
    data2 = DW_FORM_data2,
    data4 = DW_FORM_data4,
    data8 = DW_FORM_data8,
    string = DW_FORM_string,
    block = DW_FORM_block,
    block1 = DW_FORM_block1,
    data1 = DW_FORM_data1,
    flag = DW_FORM_flag,
    sdata = DW_FORM_sdata,
    strp = DW_FORM_strp,
    udata = DW_FORM_udata,
    ref_addr = DW_FORM_ref_addr,
    ref1 = DW_FORM_ref1,
    ref2 = DW_FORM_ref2,
    ref4 = DW_FORM_ref4,
    ref8 = DW_FORM_ref8,
    ref_udata = DW_FORM_ref_udata,
    indirect = DW_FORM_indirect,
    sec_offset = DW_FORM_sec_offset,
    exprloc = DW_FORM_exprloc,
    flag_present = DW_FORM_flag_present,
    strx = DW_FORM_strx,
    addrx = DW_FORM_addrx,
    ref_sup4 = DW_FORM_ref_sup4,
    strp_sup = DW_FORM_strp_sup,
    data16 = DW_FORM_data16,
    line_strp = DW_FORM_line_strp,
    ref_sig8 = DW_FORM_ref_sig8,
    implicit_const = DW_FORM_implicit_const,
    loclistx = DW_FORM_loclistx,
    rnglistx = DW_FORM_rnglistx,
    ref_sup8 = DW_FORM_ref_sup8,
    strx1 = DW_FORM_strx1,
    strx2 = DW_FORM_strx2,
    strx3 = DW_FORM_strx3,
    strx4 = DW_FORM_strx4,
    addrx1 = DW_FORM_addrx1,
    addrx2 = DW_FORM_addrx2,
    addrx3 = DW_FORM_addrx3,
    addrx4 = DW_FORM_addrx4,
    GNU_addr_index = DW_FORM_GNU_addr_index,
    GNU_str_index = DW_FORM_GNU_str_index,
    GNU_ref_alt = DW_FORM_GNU_ref_alt,
    GNU_strp_alt = DW_FORM_GNU_strp_alt,
    LLVM_addrx_offset = DW_FORM_LLVM_addrx_offset,
};

inline std::ostream &operator<<(std::ostream &os, form f)
{
    const char *name = nullptr;
    int res = dwarf_get_FORM_name(static_cast<Dwarf_Half>(f), &name);
    if (res != DW_DLV_OK) {
        os << "<bogus form>";
    }
    else {
        os << name;
    }
    return os;
}

} // namespace cppdwarf