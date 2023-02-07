enum class ID : u16 {
    Call,
    Count,
};
union Storage_ {
    Call call;
    Storage_() {
    }
    ~Storage_() {
    }
};
SWITCH_FOOTER(Expression, Call)
SWITCH_ACCESSOR(Call, call)
PLY_SWITCH_REFLECT()
