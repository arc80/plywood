enum class ID : u16 {
    Keyword,
    LangLinkage,
    Record,
    Enum_,
    TypeID,
    TypeParam,
    Ellipsis,
    Count,
};
union Storage_ {
    Keyword keyword;
    LangLinkage langLinkage;
    Record record;
    Enum_ enum_;
    TypeID type_id;
    TypeParam type_param;
    Ellipsis ellipsis;
    Storage_() {
    }
    ~Storage_() {
    }
};
SWITCH_FOOTER(DeclSpecifier, Keyword)
SWITCH_ACCESSOR(Keyword, keyword)
SWITCH_ACCESSOR(LangLinkage, langLinkage)
SWITCH_ACCESSOR(Record, record)
SWITCH_ACCESSOR(Enum_, enum_)
SWITCH_ACCESSOR(TypeID, type_id)
SWITCH_ACCESSOR(TypeParam, type_param)
SWITCH_ACCESSOR(Ellipsis, ellipsis)
PLY_SWITCH_REFLECT()
