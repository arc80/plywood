enum class ID : u16 {
    Module,
    External,
    Count,
};
union Storage_ {
    Module module;
    External external;
    PLY_INLINE Storage_() {}
    PLY_INLINE ~Storage_() {}
};
SWITCH_FOOTER(Type, Module)
SWITCH_ACCESSOR(Module, module)
SWITCH_ACCESSOR(External, external)
