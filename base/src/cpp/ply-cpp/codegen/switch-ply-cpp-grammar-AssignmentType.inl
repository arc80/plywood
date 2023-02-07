enum class ID : u16 {
    Expression,
    TypeID,
    Count,
};
union Storage_ {
    Expression expression;
    TypeID type_id;
    Storage_() {
    }
    ~Storage_() {
    }
};
SWITCH_FOOTER(AssignmentType, Expression)
SWITCH_ACCESSOR(Expression, expression)
SWITCH_ACCESSOR(TypeID, type_id)
PLY_SWITCH_REFLECT()
