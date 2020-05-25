enum class ID : u16 {
    Namespace_,
    Template_,
    Simple,
    AccessSpecifier,
    StaticAssert,
    UsingDirective,
    Alias,
    Linkage,
    Empty,
    Count,
};
union Storage_ {
    Namespace_ namespace_;
    Template_ template_;
    Simple simple;
    AccessSpecifier accessSpecifier;
    StaticAssert staticAssert;
    UsingDirective usingDirective;
    Alias alias;
    Linkage linkage;
    Empty empty;
    PLY_INLINE Storage_() {}
    PLY_INLINE ~Storage_() {}
};
SWITCH_FOOTER(Declaration, Namespace_)
SWITCH_ACCESSOR(Namespace_, namespace_)
SWITCH_ACCESSOR(Template_, template_)
SWITCH_ACCESSOR(Simple, simple)
SWITCH_ACCESSOR(AccessSpecifier, accessSpecifier)
SWITCH_ACCESSOR(StaticAssert, staticAssert)
SWITCH_ACCESSOR(UsingDirective, usingDirective)
SWITCH_ACCESSOR(Alias, alias)
SWITCH_ACCESSOR(Linkage, linkage)
SWITCH_ACCESSOR(Empty, empty)
PLY_SWITCH_REFLECT()
