#pragma once

#include <dwarf.h>
#include <libdwarf.h>

#include <ostream>

namespace cppdwarf {

enum class access {
    public_ = DW_ACCESS_public,
    protected_ = DW_ACCESS_protected,
    private_ = DW_ACCESS_private,
};

inline std::ostream &operator<<(std::ostream &os, access a)
{
    const char *name = nullptr;
    int res = dwarf_get_ACCESS_name(static_cast<Dwarf_Half>(a), &name);
    if (res != DW_DLV_OK) {
        os << "<bogus access>";
    }
    else {
        os << name;
    }
    return os;
}

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

enum class tag {
    array_type = DW_TAG_array_type,
    class_type = DW_TAG_class_type,
    entry_point = DW_TAG_entry_point,
    enumeration_type = DW_TAG_enumeration_type,
    formal_parameter = DW_TAG_formal_parameter,
    imported_declaration = DW_TAG_imported_declaration,
    label = DW_TAG_label,
    lexical_block = DW_TAG_lexical_block,
    member = DW_TAG_member,
    pointer_type = DW_TAG_pointer_type,
    reference_type = DW_TAG_reference_type,
    compile_unit = DW_TAG_compile_unit,
    string_type = DW_TAG_string_type,
    structure_type = DW_TAG_structure_type,
    subroutine_type = DW_TAG_subroutine_type,
    typedef_ = DW_TAG_typedef,
    union_type = DW_TAG_union_type,
    unspecified_parameters = DW_TAG_unspecified_parameters,
    variant = DW_TAG_variant,
    common_block = DW_TAG_common_block,
    common_inclusion = DW_TAG_common_inclusion,
    inheritance = DW_TAG_inheritance,
    inlined_subroutine = DW_TAG_inlined_subroutine,
    module = DW_TAG_module,
    ptr_to_member_type = DW_TAG_ptr_to_member_type,
    set_type = DW_TAG_set_type,
    subrange_type = DW_TAG_subrange_type,
    with_stmt = DW_TAG_with_stmt,
    access_declaration = DW_TAG_access_declaration,
    base_type = DW_TAG_base_type,
    catch_block = DW_TAG_catch_block,
    const_type = DW_TAG_const_type,
    constant = DW_TAG_constant,
    enumerator = DW_TAG_enumerator,
    file_type = DW_TAG_file_type,
    friend_ = DW_TAG_friend,
    namelist = DW_TAG_namelist,
    namelist_item = DW_TAG_namelist_item,
    namelist_items = DW_TAG_namelist_items,
    packed_type = DW_TAG_packed_type,
    subprogram = DW_TAG_subprogram,
    template_type_parameter = DW_TAG_template_type_parameter,
    template_type_param = DW_TAG_template_type_param,
    template_value_parameter = DW_TAG_template_value_parameter,
    template_value_param = DW_TAG_template_value_param,
    thrown_type = DW_TAG_thrown_type,
    try_block = DW_TAG_try_block,
    variant_part = DW_TAG_variant_part,
    variable = DW_TAG_variable,
    volatile_type = DW_TAG_volatile_type,
    dwarf_procedure = DW_TAG_dwarf_procedure,
    restrict_type = DW_TAG_restrict_type,
    interface_type = DW_TAG_interface_type,
    namespace_ = DW_TAG_namespace,
    imported_module = DW_TAG_imported_module,
    unspecified_type = DW_TAG_unspecified_type,
    partial_unit = DW_TAG_partial_unit,
    imported_unit = DW_TAG_imported_unit,
    mutable_type = DW_TAG_mutable_type,
    condition = DW_TAG_condition,
    shared_type = DW_TAG_shared_type,
    type_unit = DW_TAG_type_unit,
    rvalue_reference_type = DW_TAG_rvalue_reference_type,
    template_alias = DW_TAG_template_alias,
    coarray_type = DW_TAG_coarray_type,
    generic_subrange = DW_TAG_generic_subrange,
    dynamic_type = DW_TAG_dynamic_type,
    atomic_type = DW_TAG_atomic_type,
    call_site = DW_TAG_call_site,
    call_site_parameter = DW_TAG_call_site_parameter,
    skeleton_unit = DW_TAG_skeleton_unit,
    immutable_type = DW_TAG_immutable_type,
    TI_far_type = DW_TAG_TI_far_type,
    lo_user = DW_TAG_lo_user,
    MIPS_loop = DW_TAG_MIPS_loop,
    TI_near_type = DW_TAG_TI_near_type,
    TI_assign_register = DW_TAG_TI_assign_register,
    TI_ioport_type = DW_TAG_TI_ioport_type,
    TI_restrict_type = DW_TAG_TI_restrict_type,
    TI_onchip_type = DW_TAG_TI_onchip_type,
    HP_array_descriptor = DW_TAG_HP_array_descriptor,
    format_label = DW_TAG_format_label,
    function_template = DW_TAG_function_template,
    class_template = DW_TAG_class_template,
    GNU_BINCL = DW_TAG_GNU_BINCL,
    GNU_EINCL = DW_TAG_GNU_EINCL,
    GNU_template_template_parameter = DW_TAG_GNU_template_template_parameter,
    GNU_template_template_param = DW_TAG_GNU_template_template_param,
    GNU_template_parameter_pack = DW_TAG_GNU_template_parameter_pack,
    GNU_formal_parameter_pack = DW_TAG_GNU_formal_parameter_pack,
    GNU_call_site = DW_TAG_GNU_call_site,
    GNU_call_site_parameter = DW_TAG_GNU_call_site_parameter,
    SUN_function_template = DW_TAG_SUN_function_template,
    SUN_class_template = DW_TAG_SUN_class_template,
    SUN_struct_template = DW_TAG_SUN_struct_template,
    SUN_union_template = DW_TAG_SUN_union_template,
    SUN_indirect_inheritance = DW_TAG_SUN_indirect_inheritance,
    SUN_codeflags = DW_TAG_SUN_codeflags,
    SUN_memop_info = DW_TAG_SUN_memop_info,
    SUN_omp_child_func = DW_TAG_SUN_omp_child_func,
    SUN_rtti_descriptor = DW_TAG_SUN_rtti_descriptor,
    SUN_dtor_info = DW_TAG_SUN_dtor_info,
    SUN_dtor = DW_TAG_SUN_dtor,
    SUN_f90_interface = DW_TAG_SUN_f90_interface,
    SUN_fortran_vax_structure = DW_TAG_SUN_fortran_vax_structure,
    SUN_hi = DW_TAG_SUN_hi,
    ALTIUM_circ_type = DW_TAG_ALTIUM_circ_type,
    ALTIUM_mwa_circ_type = DW_TAG_ALTIUM_mwa_circ_type,
    ALTIUM_rev_carry_type = DW_TAG_ALTIUM_rev_carry_type,
    ALTIUM_rom = DW_TAG_ALTIUM_rom,
    LLVM_annotation = DW_TAG_LLVM_annotation,
    ghs_namespace = DW_TAG_ghs_namespace,
    ghs_using_namespace = DW_TAG_ghs_using_namespace,
    ghs_using_declaration = DW_TAG_ghs_using_declaration,
    ghs_template_templ_param = DW_TAG_ghs_template_templ_param,
    upc_shared_type = DW_TAG_upc_shared_type,
    upc_strict_type = DW_TAG_upc_strict_type,
    upc_relaxed_type = DW_TAG_upc_relaxed_type,
    PGI_kanji_type = DW_TAG_PGI_kanji_type,
    PGI_interface_block = DW_TAG_PGI_interface_block,
    BORLAND_property = DW_TAG_BORLAND_property,
    BORLAND_Delphi_string = DW_TAG_BORLAND_Delphi_string,
    BORLAND_Delphi_dynamic_array = DW_TAG_BORLAND_Delphi_dynamic_array,
    BORLAND_Delphi_set = DW_TAG_BORLAND_Delphi_set,
    BORLAND_Delphi_variant = DW_TAG_BORLAND_Delphi_variant,
    hi_user = DW_TAG_hi_user,
};

inline std::ostream &operator<<(std::ostream &os, tag tag)
{
    const char *name = nullptr;
    int res = dwarf_get_TAG_name(static_cast<Dwarf_Half>(tag), &name);
    if (res != DW_DLV_OK) {
        os << "<bogus tag>";
    }
    else {
        os << name;
    }
    return os;
}

enum class virtuality {
    none = DW_VIRTUALITY_none,
    virtual_ = DW_VIRTUALITY_virtual,
    pure_virtual = DW_VIRTUALITY_pure_virtual,
};

inline std::ostream &operator<<(std::ostream &os, virtuality v)
{
    const char *name = nullptr;
    int res = dwarf_get_VIRTUALITY_name(static_cast<Dwarf_Half>(v), &name);
    if (res != DW_DLV_OK) {
        os << "<bogus virtuality>";
    }
    else {
        os << name;
    }
    return os;
}

} // namespace cppdwarf
