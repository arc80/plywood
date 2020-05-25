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
    TypeID typeID;
    TypeParam typeParam;
    Ellipsis ellipsis;
    PLY_INLINE Storage_() {}
    PLY_INLINE ~Storage_() {}
};
SWITCH_FOOTER(DeclSpecifier, Keyword)
SWITCH_ACCESSOR(Keyword, keyword)
SWITCH_ACCESSOR(LangLinkage, langLinkage)
SWITCH_ACCESSOR(Record, record)
SWITCH_ACCESSOR(Enum_, enum_)
SWITCH_ACCESSOR(TypeID, typeID)
SWITCH_ACCESSOR(TypeParam, typeParam)
SWITCH_ACCESSOR(Ellipsis, ellipsis)
PLY_SWITCH_REFLECT()
