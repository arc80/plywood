/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeDescriptor.h>
#include <ply-reflect/PersistRead.h>
#include <unordered_map>

namespace ply {

//--------------------------------------------------------------------
// Write
//

PLY_DLL_ENTRY void writeAsset(OutStream* out, TypedPtr obj);

//--------------------------------------------------------------------
// Read
//

PLY_DLL_ENTRY TypedPtr readAsset(InStream* in, PersistentTypeResolver* resolver);

class ExpectedTypeResolver : public PersistentTypeResolver {
public:
    TypeDescriptor* m_expected;

    PLY_DLL_ENTRY ExpectedTypeResolver(TypeDescriptor* expected) : m_expected(expected) {
    }

    virtual TypeDescriptor* getType(FormatDescriptor* formatDesc) override;
};

inline TypedPtr readExpectedAsset(InStream* in, TypeDescriptor* expected) {
    ExpectedTypeResolver expectedResolver{expected};
    return readAsset(in, &expectedResolver);
}

template <typename T>
inline TypedPtr readExpectedAsset(InStream* in) {
    return readExpectedAsset(in, TypeResolver<T>::get());
}

// FIXME: This was added to support wire protocols (eg. between RemoteCooker and CookEXE).
// It duplicates some of the functionality of g_TypeSynthRegistry (which, in turn, is used by
// TypeSynthesizer). Might be best to phase out g_TypeSynthRegistry and replace its existing
// usage with this, or a variant that also incorporates TypeSynthesizer:
class RegistryTypeResolver : public PersistentTypeResolver {
public:
    struct HashMapTraits {
        using Key = StringView;
        struct Item {
            StringView name;
            TypeDescriptor* type = nullptr;
        };
        static PLY_INLINE bool match(const Item& item, Key key) {
            return item.name == key;
        }
    };

    HashMap<HashMapTraits> m_nameToBuiltInType;

    void add(TypeDescriptor* builtInTypeDesc) {
        TypeDescriptor_Struct* structDesc = builtInTypeDesc->cast<TypeDescriptor_Struct>();
        auto insertCursor = m_nameToBuiltInType.insertOrFind(structDesc->name);
        PLY_ASSERT(!insertCursor.wasFound());
        insertCursor->name = structDesc->name;
        insertCursor->type = structDesc;
    }

    virtual TypeDescriptor* getType(FormatDescriptor* formatDesc) override {
        PLY_ASSERT(formatDesc->formatKey == (u8) FormatKey::Struct);
        auto foundCursor =
            m_nameToBuiltInType.find(static_cast<FormatDescriptor_Struct*>(formatDesc)->name);
        PLY_ASSERT(foundCursor.wasFound());
        return foundCursor->type;
    }
};

} // namespace ply
