enum class ID : u16 {
    Invalid,
    Object,
    Array,
    Text,
    Count,
};
union Storage_ {
    Invalid invalid;
    Object object;
    Array array;
    Text text;
    PLY_INLINE Storage_() {}
    PLY_INLINE ~Storage_() {}
};
SWITCH_FOOTER(Type, Invalid)
SWITCH_ACCESSOR(Invalid, invalid)
SWITCH_ACCESSOR(Object, object)
SWITCH_ACCESSOR(Array, array)
SWITCH_ACCESSOR(Text, text)
