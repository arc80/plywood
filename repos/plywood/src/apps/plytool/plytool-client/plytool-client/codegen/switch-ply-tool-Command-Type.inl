enum class ID : u16 {
    Run,
    Count,
};
union Storage_ {
    Run run;
    PLY_INLINE Storage_() {}
    PLY_INLINE ~Storage_() {}
};
SWITCH_FOOTER(Type, Run)
SWITCH_ACCESSOR(Run, run)
PLY_SWITCH_REFLECT()
