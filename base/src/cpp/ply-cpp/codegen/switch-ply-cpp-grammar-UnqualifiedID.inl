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
    TemplateID template_id;
    DeclType decl_type;
    Destructor destructor;
    OperatorFunc operator_func;
    ConversionFunc conversion_func;
    Storage_() {
    }
    ~Storage_() {
    }
};
SWITCH_FOOTER(UnqualifiedID, Empty)
SWITCH_ACCESSOR(Empty, empty)
SWITCH_ACCESSOR(Identifier, identifier)
SWITCH_ACCESSOR(TemplateID, template_id)
SWITCH_ACCESSOR(DeclType, decl_type)
SWITCH_ACCESSOR(Destructor, destructor)
SWITCH_ACCESSOR(OperatorFunc, operator_func)
SWITCH_ACCESSOR(ConversionFunc, conversion_func)
PLY_SWITCH_REFLECT()
