enum class ID : u16 {
    Call,
    Count,
};
union Storage_ {
    Call call;
    PLY_INLINE Storage_() {}
    PLY_INLINE ~Storage_() {}
};
SWITCH_FOOTER(Expression, Call)
SWITCH_ACCESSOR(Call, call)
PLY_SWITCH_REFLECT()
