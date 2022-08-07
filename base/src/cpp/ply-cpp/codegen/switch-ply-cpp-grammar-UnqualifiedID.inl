enum class ID : u16 {
    Empty,
    Identifier,
    TemplateID,
    DeclType,
    Destructor,
    OperatorFunc,
    ConversionFunc,
    Count,
};
union Storage_ {
    Empty empty;
    Identifier identifier;
    TemplateID templateID;
    DeclType declType;
    Destructor destructor;
    OperatorFunc operatorFunc;
    ConversionFunc conversionFunc;
    PLY_INLINE Storage_() {}
    PLY_INLINE ~Storage_() {}
};
SWITCH_FOOTER(UnqualifiedID, Empty)
SWITCH_ACCESSOR(Empty, empty)
SWITCH_ACCESSOR(Identifier, identifier)
SWITCH_ACCESSOR(TemplateID, templateID)
SWITCH_ACCESSOR(DeclType, declType)
SWITCH_ACCESSOR(Destructor, destructor)
SWITCH_ACCESSOR(OperatorFunc, operatorFunc)
SWITCH_ACCESSOR(ConversionFunc, conversionFunc)
PLY_SWITCH_REFLECT()
