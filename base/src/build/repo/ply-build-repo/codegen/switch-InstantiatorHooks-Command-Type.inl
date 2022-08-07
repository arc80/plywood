enum class ID : u16 {
    Target,
    External,
    Count,
};
union Storage_ {
    Target target;
    External external;
    PLY_INLINE Storage_() {}
    PLY_INLINE ~Storage_() {}
};
SWITCH_FOOTER(Type, Target)
SWITCH_ACCESSOR(Target, target)
SWITCH_ACCESSOR(External, external)
