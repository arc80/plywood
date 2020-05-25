enum class ID : u16 {
    Expression,
    TypeID,
    Count,
};
union Storage_ {
    Expression expression;
    TypeID typeID;
    PLY_INLINE Storage_() {}
    PLY_INLINE ~Storage_() {}
};
SWITCH_FOOTER(AssignmentType, Expression)
SWITCH_ACCESSOR(Expression, expression)
SWITCH_ACCESSOR(TypeID, typeID)
PLY_SWITCH_REFLECT()
