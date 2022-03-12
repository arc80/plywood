/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeKey.h>

namespace ply {

struct TypeDescriptor;
struct ObjectSaver;
struct ObjectLoader;
class SchemaSaver;
struct FormatDescriptor;

//--------------------------------------------------------
// TypeDescriptors are created for non-aggregate C++ types using the
// PLY_DECLARE/DEFINE_TYPE_DESCRIPTOR() macros. Each time getTypeDescriptor<>() is instantiated for
// a specific type during the compilation process, a TypeDescriptor corresponding to that type will
// be made available at runtime.
//--------------------------------------------------------
template <typename T>
struct TypeDescriptorSpecializer {
    static PLY_INLINE TypeDescriptor* get() {
        return T::bindTypeDescriptor();
    }
};

template <typename T>
PLY_INLINE TypeDescriptor* getTypeDescriptor(T* = nullptr) {
    return TypeDescriptorSpecializer<T>::get();
}

#define PLY_DECLARE_TYPE_DESCRIPTOR(typeName, ...) \
    template <> \
    struct ply::TypeDescriptorSpecializer<typeName> { \
        static __VA_ARGS__ ply::TypeDescriptor* get(); \
    };

#define PLY_DEFINE_TYPE_DESCRIPTOR(typeName) \
    PLY_NO_INLINE ply::TypeDescriptor* ply::TypeDescriptorSpecializer<typeName>::get()

//-----------------------------------------------------------------------
// TypeKey
//-----------------------------------------------------------------------
PLY_DLL_ENTRY extern TypeKey TypeKey_Bool;
PLY_DLL_ENTRY extern TypeKey TypeKey_S8;
PLY_DLL_ENTRY extern TypeKey TypeKey_S16;
PLY_DLL_ENTRY extern TypeKey TypeKey_S32;
PLY_DLL_ENTRY extern TypeKey TypeKey_S64;
PLY_DLL_ENTRY extern TypeKey TypeKey_U8;
PLY_DLL_ENTRY extern TypeKey TypeKey_U16;
PLY_DLL_ENTRY extern TypeKey TypeKey_U32;
PLY_DLL_ENTRY extern TypeKey TypeKey_U64;
PLY_DLL_ENTRY extern TypeKey TypeKey_Float;
PLY_DLL_ENTRY extern TypeKey TypeKey_Double;
PLY_DLL_ENTRY extern TypeKey TypeKey_String;
PLY_DLL_ENTRY extern TypeKey TypeKey_FixedArray;
PLY_DLL_ENTRY extern TypeKey TypeKey_Array;
PLY_DLL_ENTRY extern TypeKey TypeKey_Owned;
PLY_DLL_ENTRY extern TypeKey TypeKey_Reference;
PLY_DLL_ENTRY extern TypeKey TypeKey_Struct;
PLY_DLL_ENTRY extern TypeKey TypeKey_Switch;
PLY_DLL_ENTRY extern TypeKey TypeKey_Enum;
PLY_DLL_ENTRY extern TypeKey TypeKey_RawPtr;
PLY_DLL_ENTRY extern TypeKey TypeKey_EnumIndexedArray;
PLY_DLL_ENTRY extern TypeKey TypeKey_TypedArray;
PLY_DLL_ENTRY extern TypeKey TypeKey_SavedTypedPtr;

//-----------------------------------------------------------------------
// NativeBindings
//-----------------------------------------------------------------------
struct NativeBindings {
    TypedPtr (*create)(TypeDescriptor* typeDesc);
    void (*destroy)(TypedPtr obj);
    void (*construct)(TypedPtr obj);
    void (*destruct)(TypedPtr obj);
    void (*move)(TypedPtr dst, TypedPtr src);
    void (*copy)(TypedPtr dst, const TypedPtr src);

    template <typename T>
    static NativeBindings make() {
        return {
            // create
            [](TypeDescriptor* typeDesc) -> TypedPtr {
                PLY_ASSERT(typeDesc == TypeDescriptorSpecializer<T>::get());
                return {subst::createByMember<T>(), typeDesc};
            },
            // destroy
            [](TypedPtr obj) {
                PLY_ASSERT(obj.type == TypeDescriptorSpecializer<T>::get());
                subst::destroyByMember((T*) obj.ptr);
            },
            // construct
            [](TypedPtr obj) {
                PLY_ASSERT(obj.type == TypeDescriptorSpecializer<T>::get());
                subst::unsafeConstruct((T*) obj.ptr);
            },
            // destruct
            [](TypedPtr obj) {
                PLY_ASSERT(obj.type == TypeDescriptorSpecializer<T>::get());
                subst::destructByMember((T*) obj.ptr);
            },
            // move
            [](TypedPtr dst, TypedPtr src) {
                PLY_ASSERT(dst.type == TypeDescriptorSpecializer<T>::get());
                PLY_ASSERT(src.type == TypeDescriptorSpecializer<T>::get());
                subst::unsafeMove((T*) dst.ptr, (T*) src.ptr);
            },
            // copy
            [](TypedPtr dst, const TypedPtr src) {
                PLY_ASSERT(dst.type == TypeDescriptorSpecializer<T>::get());
                PLY_ASSERT(src.type == TypeDescriptorSpecializer<T>::get());
                subst::unsafeCopy((T*) dst.ptr, (const T*) src.ptr);
            },
        };
    }
};

//-----------------------------------------------------------------------
// TypeDescriptor
//-----------------------------------------------------------------------
struct TypeDescriptor {
    TypeKey* typeKey;
    u32 fixedSize;
    NativeBindings bindings;
    // FIXME: Add getAlignment(), make sure it's enforced by all dynamic allocations

    TypeDescriptor(TypeKey* typeKey, u32 fixedSize, const NativeBindings& bindings)
        : typeKey(typeKey), fixedSize(fixedSize), bindings(bindings) {
    }
    TypeDescriptor(TypeDescriptor&& other)
        : typeKey(other.typeKey), fixedSize(other.fixedSize), bindings(other.bindings) {
    }
    // Built-in TypeDescriptors exist for the entire process lifetime, including those defined
    // by PLY_STRUCT_BEGIN. The only TypeDescriptors that ever get destroyed are ones that are
    // synthesized at runtime. The lifetime of those TypeDescriptors is managed by a
    // TypeDescriptorOwner, which knows how to correctly destruct them.
    template <class T>
    T* cast() {
        PLY_ASSERT(typeKey == T::typeKey);
        return static_cast<T*>(this);
    }
    template <class T>
    const T* cast() const {
        PLY_ASSERT(typeKey == T::typeKey);
        return static_cast<const T*>(this);
    }

    PLY_INLINE bool isEquivalentTo(const TypeDescriptor* other) const {
        if (this == other)
            return true;
        if (typeKey != other->typeKey)
            return false;
        if (!typeKey->equalDescriptors)
            return false;
        return typeKey->equalDescriptors(this, other);
    }
};

PLY_INLINE Hasher& operator<<(Hasher& hasher, const TypeDescriptor* typeDesc) {
    hasher << typeDesc->typeKey;
    typeDesc->typeKey->hashDescriptor(hasher, typeDesc);
    return hasher;
}

//-----------------------------------------------------------------------
// Built-in TypeDescriptors
//-----------------------------------------------------------------------
PLY_DECLARE_TYPE_DESCRIPTOR(bool, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(s8, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(s16, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(s32, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(s64, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(u8, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(u16, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(u32, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(u64, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(float, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(double, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(String, PLY_DLL_ENTRY)

//-----------------------------------------------------------------------
// TypeDescriptorSpecializer for const types
//-----------------------------------------------------------------------
template <typename T>
struct TypeDescriptorSpecializer<const T> {
    static PLY_INLINE auto* get() {
        return TypeDescriptorSpecializer<T>::get();
    }
};

//-----------------------------------------------------------------------
// TypeDescriptorSpecializer for C-style arrays
//-----------------------------------------------------------------------
PLY_DLL_ENTRY NativeBindings& getNativeBindings_FixedArray();

struct TypeDescriptor_FixedArray : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* typeKey;
    TypeDescriptor* itemType;
    u32 numItems;
    u32 stride;

    PLY_INLINE TypeDescriptor_FixedArray(TypeDescriptor* itemType, u32 numItems)
        : TypeDescriptor{&TypeKey_FixedArray, itemType->fixedSize * numItems,
                         getNativeBindings_FixedArray()},
          itemType{itemType}, numItems{numItems}, stride{itemType->fixedSize} {
    }
    PLY_INLINE TypeDescriptor_FixedArray(TypeDescriptor* itemType, u32 numItems, u32 stride)
        : TypeDescriptor{&TypeKey_FixedArray, stride * numItems, getNativeBindings_FixedArray()},
          itemType{itemType}, numItems{numItems}, stride{stride} {
        PLY_ASSERT(stride >= itemType->fixedSize);
    }
};

template <typename T, int numItems>
struct TypeDescriptorSpecializer<T[numItems]> {
    static PLY_NO_INLINE TypeDescriptor_FixedArray* get() {
        static TypeDescriptor_FixedArray typeDesc{getTypeDescriptor<T>(), u32(numItems)};
        return &typeDesc;
    }
};

//-----------------------------------------------------------------------
// TypeDescriptorSpecializer for FixedArray
//-----------------------------------------------------------------------
template <typename T, u32 Size>
struct TypeDescriptorSpecializer<FixedArray<T, Size>> {
    static PLY_NO_INLINE TypeDescriptor_FixedArray* get() {
        static TypeDescriptor_FixedArray typeDesc{getTypeDescriptor<T>(), Size};
        return &typeDesc;
    }
};

//-----------------------------------------------------------------------
// TypeDescriptorSpecializer for Array
//-----------------------------------------------------------------------
PLY_DLL_ENTRY NativeBindings& getNativeBindings_Array();

struct TypeDescriptor_Array : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* typeKey;
    TypeDescriptor* itemType;

    PLY_INLINE TypeDescriptor_Array(TypeDescriptor* itemType)
        : TypeDescriptor{&TypeKey_Array, sizeof(details::BaseArray), getNativeBindings_Array()},
          itemType{itemType} {
    }
};

template <typename T>
struct TypeDescriptorSpecializer<Array<T>> {
    static PLY_NO_INLINE TypeDescriptor_Array* get() {
        static TypeDescriptor_Array typeDesc{getTypeDescriptor<T>()};
        return &typeDesc;
    }
};

struct TypedPtr_Array {
    typedef TypeDescriptor_Array TypeDescriptor;

    void* ptr = nullptr;
    TypeDescriptor_Array* type = nullptr;

    TypedPtr_Array() {
    }
    TypedPtr_Array(void* ptr, TypeDescriptor_Array* type) : ptr{ptr}, type{type} {
    }
    operator TypedPtr() const {
        return {ptr, type};
    }
    u32 numItems() const {
        return reinterpret_cast<details::BaseArray*>(ptr)->m_numItems;
    }
    void resize(u32 newSize);
    TypedPtr getItem(u32 index) {
        details::BaseArray* arr = reinterpret_cast<details::BaseArray*>(ptr);
        PLY_ASSERT(index < arr->m_numItems);
        return {PLY_PTR_OFFSET(arr->m_items, type->itemType->fixedSize * index), type->itemType};
    }
};

//-----------------------------------------------------------------------
// TypeDescriptorSpecializer for Owned
//-----------------------------------------------------------------------
PLY_DLL_ENTRY NativeBindings& getNativeBindings_Owned();

struct TypeDescriptor_Owned : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* typeKey;
    TypeDescriptor* targetType;
    void (*assignRawPtr)(TypedPtr ownedPtr, TypedPtr target) = nullptr;

    template <typename T>
    PLY_INLINE TypeDescriptor_Owned(T*)
        : TypeDescriptor{&TypeKey_Owned, sizeof(void*), getNativeBindings_Owned()},
          targetType{getTypeDescriptor<T>()} {
        assignRawPtr = [](TypedPtr ownedPtr, TypedPtr target) {
            // FIXME: Check that the target type is compatible
            // If the target uses multiple inheritance, may need to adjust its pointer value to
            // store as a base class object!
            *reinterpret_cast<Owned<T>*>(ownedPtr.ptr) = (T*) target.ptr;
        };
    }
};

template <typename T>
struct TypeDescriptorSpecializer<Owned<T>> {
    static PLY_NO_INLINE TypeDescriptor_Owned* get() {
        static TypeDescriptor_Owned typeDesc{(T*) nullptr};
        return &typeDesc;
    }
};

struct TypedPtr_Owned {
    typedef TypeDescriptor_Owned TypeDescriptor;

    void* ptr = nullptr;
    TypeDescriptor_Owned* type = nullptr;

    TypedPtr_Owned() {
    }
    TypedPtr_Owned(void* ptr, TypeDescriptor_Owned* type) : ptr{ptr}, type{type} {
    }
    operator TypedPtr() const {
        return {ptr, type};
    }
    void assign(TypedPtr target) {
        type->assignRawPtr(*this, target);
    }
    template <typename T>
    T* get() const {
        PLY_ASSERT(type->targetType->isEquivalentTo(TypeDescriptorSpecializer<T>::get()));
        return reinterpret_cast<Owned<T>*>(ptr)->get();
    }
};

//-----------------------------------------------------------------------
// TypeDescriptorSpecializer for Reference
//-----------------------------------------------------------------------
PLY_DLL_ENTRY NativeBindings& getNativeBindings_Reference();

struct TypeDescriptor_Reference : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* typeKey;
    TypeDescriptor* targetType;
    void (*incRef)(void* target) = nullptr;
    void (*decRef)(void* target) = nullptr;

    template <typename T>
    PLY_INLINE TypeDescriptor_Reference(T*)
        : TypeDescriptor{&TypeKey_Reference, sizeof(void*), getNativeBindings_Reference()},
          targetType{getTypeDescriptor<T>()} {
        this->incRef = [](void* target) { //
            ((T*) target)->incRef();
        };
        this->decRef = [](void* target) { //
            ((T*) target)->decRef();
        };
    }
};

template <typename T>
struct TypeDescriptorSpecializer<Reference<T>> {
    static PLY_NO_INLINE TypeDescriptor_Reference* get() {
        static TypeDescriptor_Reference typeDesc{(T*) nullptr};
        return &typeDesc;
    }
};

//-----------------------------------------------------------------------
// PLY_REFLECT() macro for structs
//-----------------------------------------------------------------------
PLY_DLL_ENTRY NativeBindings& getNativeBindings_SynthesizedStruct();

struct TypeDescriptor_Struct : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* typeKey;
    struct TemplateParam {
        String name;
        TypeDescriptor* type;
    };
    struct Member {
        String name;
        u32 offset;
        TypeDescriptor* type;
    };

    String name;
    Array<TemplateParam> templateParams;
    Array<Member> members;

    // Use expression SFINAE to invoke the object's onPostSerialize() function, if present:
    template <class T>
    static PLY_INLINE auto invokePostSerialize(T* obj) -> decltype(obj->onPostSerialize()) {
        return obj->onPostSerialize();
    }
    static PLY_INLINE void invokePostSerialize(...) {
    }
    void (*onPostSerialize)(void* ptr) = [](void*) {};

    // Constructor for synthesized TypeDescriptor_Struct:
    PLY_INLINE TypeDescriptor_Struct(u32 fixedSize, StringView name)
        : TypeDescriptor{&TypeKey_Struct, fixedSize, getNativeBindings_SynthesizedStruct()},
          name{name} {
    }
    // Constructor for TypeDescriptor_Struct of an existing C++ class:
    template <class T>
    PLY_INLINE TypeDescriptor_Struct(T*, StringView name,
                                     std::initializer_list<Member> members = {})
        : TypeDescriptor{&TypeKey_Struct, sizeof(T), NativeBindings::make<T>()}, name{name},
          members{members} {
        onPostSerialize = [](void* ptr) { invokePostSerialize((T*) ptr); };
    }

    PLY_INLINE void appendMember(StringView name, TypeDescriptor* type) {
        // FIXME: Handle alignment
        members.append({name, fixedSize, type});
        fixedSize += type->fixedSize;
    }

    PLY_NO_INLINE const Member* findMember(StringView name) const {
        for (const Member& member : members) {
            if (member.name == name)
                return &member;
        }
        return nullptr;
    }
};

struct TypedPtr_Struct : TypedPtr {
    typedef TypeDescriptor_Struct TypeDescriptor;

    TypedPtr_Struct(void* ptr, TypeDescriptor_Struct* structType) : TypedPtr{ptr, structType} {
    }
    TypeDescriptor_Struct* structType() const {
        return (TypeDescriptor_Struct*) type;
    }
    TypedPtr getMember(const TypeDescriptor_Struct::Member& member) const {
#if PLY_WITH_ASSERTS
        const TypeDescriptor_Struct::Member* firstMember = &structType()->members[0];
        PLY_ASSERT(&member >= firstMember &&
                   &member < firstMember + structType()->members.numItems());
#endif
        return {PLY_PTR_OFFSET(ptr, member.offset), member.type};
    }
};

struct Initializer {
    Initializer(void (*init)()) {
        init();
    }
};

// clang-format off
#define PLY_REFLECT(...) \
    template <typename> friend struct ply::TypeDescriptorSpecializer; \
    static __VA_ARGS__ ply::TypeDescriptor* bindTypeDescriptor(); \
    static ply::Initializer initTypeDescriptor; \
    static void initTypeDescriptorMembers();

#define PLY_STATE_REFLECT(name) \
    PLY_STATE(name) \
    PLY_REFLECT()

#define PLY_STRUCT_BEGIN(type) \
    PLY_NO_INLINE ply::TypeDescriptor* type::bindTypeDescriptor() { \
        static ply::TypeDescriptor_Struct typeDesc{(type*) nullptr, #type}; \
        return &typeDesc; \
    } \
    ply::Initializer type::initTypeDescriptor{type::initTypeDescriptorMembers}; \
    void type::initTypeDescriptorMembers() { \
        using T = type; \
        PLY_UNUSED((T*) nullptr); \
        T::bindTypeDescriptor()->cast<ply::TypeDescriptor_Struct>()->members = {

#define PLY_STRUCT_MEMBER(name) \
            {#name, PLY_MEMBER_OFFSET(T, name), ply::getTypeDescriptor<decltype(T::name)>()},

#define PLY_STRUCT_END() \
        }; \
    }

#define PLY_STRUCT_BEGIN_PRIM(type) \
    PLY_DEFINE_TYPE_DESCRIPTOR(type) { \
        static ply::TypeDescriptor_Struct typeDesc{(type*) nullptr, #type}; \
        return &typeDesc; \
    } \
    static void PLY_UNIQUE_VARIABLE(initTypeDescriptor_)(); \
    ply::Initializer PLY_UNIQUE_VARIABLE(initTypeDescriptorCaller_){PLY_UNIQUE_VARIABLE(initTypeDescriptor_)}; \
    static void PLY_UNIQUE_VARIABLE(initTypeDescriptor_)() { \
        using T = type; \
        PLY_UNUSED((T*) nullptr); \
        ply::getTypeDescriptor<T>()->cast<ply::TypeDescriptor_Struct>()->members = {

#define PLY_STRUCT_END_PRIM() \
        }; \
    }
// clang-format on

//-----------------------------------------------------------------------
// enums
//-----------------------------------------------------------------------
PLY_DLL_ENTRY NativeBindings& getNativeBindings_Enum();

struct TypeDescriptor_Enum : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* typeKey;
    struct Identifier {
        String name;
        u32 value;
    };

    String name;
    Array<Identifier> identifiers;

    PLY_INLINE TypeDescriptor_Enum(u32 fixedSize, const char* name,
                                   std::initializer_list<Identifier> identifiers)
        : TypeDescriptor{&TypeKey_Enum, fixedSize, getNativeBindings_Enum()}, name{name},
          identifiers{identifiers} {
    }

    PLY_NO_INLINE const Identifier* findIdentifier(StringView name) const {
        for (const Identifier& identifier : identifiers) {
            if (identifier.name == name)
                return &identifier;
        }
        return nullptr;
    }

    PLY_NO_INLINE const Identifier* findValue(u32 value) const {
        for (const Identifier& identifier : identifiers) {
            if (identifier.value == value)
                return &identifier;
        }
        return nullptr;
    }
};

// clang-format off
#define PLY_ENUM_BEGIN(ns, type) \
    PLY_DEFINE_TYPE_DESCRIPTOR(ns type) { \
        using T = ns type; \
        static ply::TypeDescriptor_Enum typeDesc{ \
            sizeof(T), #type, {

#define PLY_ENUM_IDENTIFIER(name) \
                {#name, ply::u32(T::name)},

#define PLY_ENUM_END() \
            } \
        }; \
        return &typeDesc; \
    }
// clang-format on

//-----------------------------------------------------------------------
// TypeDescriptorSpecializer for raw pointers
//-----------------------------------------------------------------------
PLY_DLL_ENTRY NativeBindings& getNativeBindings_RawPtr();

struct TypeDescriptor_RawPtr : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* typeKey;
    TypeDescriptor* targetType;

    TypeDescriptor_RawPtr(TypeDescriptor* targetType)
        : TypeDescriptor{&TypeKey_RawPtr, sizeof(void*), getNativeBindings_RawPtr()},
          targetType{targetType} {
    }
};

template <typename T>
struct TypeDescriptorSpecializer<T*> {
    static PLY_NO_INLINE TypeDescriptor_RawPtr* get() {
        static TypeDescriptor_RawPtr typeDesc{getTypeDescriptor<T>()};
        return &typeDesc;
    }
};

//-----------------------------------------------------------------------
// TypeDescriptorSpecializer for EnumIndexedArray
//-----------------------------------------------------------------------
PLY_DLL_ENTRY NativeBindings& getNativeBindings_EnumIndexedArray();

struct TypeDescriptor_EnumIndexedArray : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* typeKey;
    TypeDescriptor* itemType;
    TypeDescriptor_Enum* enumType;

    template <typename T, typename EnumType>
    PLY_INLINE TypeDescriptor_EnumIndexedArray(T*, EnumType*)
        : TypeDescriptor{&TypeKey_EnumIndexedArray, sizeof(EnumIndexedArray<T, EnumType>),
                         getNativeBindings_EnumIndexedArray()} {
        itemType = getTypeDescriptor<T>();
        enumType = TypeDescriptorSpecializer<EnumType>::get()->template cast<TypeDescriptor_Enum>();
        PLY_ASSERT(fixedSize == itemType->fixedSize * enumType->identifiers.numItems());
#if PLY_WITH_ASSERTS
        // The enum type must meet the following requirements:
        // Identifiers are sequentially numbered starting with 0.
        for (u32 i = 0; i < enumType->identifiers.numItems(); i++) {
            PLY_ASSERT(enumType->identifiers[i].value == i);
        }
        // The special identifier "Count" is the last one.
        // "Count" is not reflected. Thus, its integer value matches the number of reflected
        // identifiers.
        PLY_ASSERT(enumType->identifiers.numItems() == (u32) EnumType::Count);
#endif
    }
};

template <typename T, typename EnumType>
struct TypeDescriptorSpecializer<EnumIndexedArray<T, EnumType>> {
    static PLY_NO_INLINE TypeDescriptor_EnumIndexedArray* get() {
        static TypeDescriptor_EnumIndexedArray typeDesc{(T*) nullptr, (EnumType*) nullptr};
        return &typeDesc;
    }
};

//-----------------------------------------------------------------------
// Switch
//-----------------------------------------------------------------------
struct TypeDescriptor_Switch : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* typeKey;
    struct State {
        String name;
#if PLY_WITH_ASSERTS
        u16 id; // Used only by PLY_SWITCH_END() to ensure that the order of
                // PLY_SWITCH_MEMBER() macros matches the real IDs used at runtime
#endif
        TypeDescriptor_Struct* structType;
    };

    String name;
    u16 storageOffset;
    Array<State> states;

    // Constructor for TypeDescriptor_Switch of an existing C++ class:
    template <class T>
    TypeDescriptor_Switch(T*, StringView name, std::initializer_list<State> states = {})
        : TypeDescriptor{&TypeKey_Switch, sizeof(T), NativeBindings::make<T>()}, name{name},
          storageOffset{PLY_MEMBER_OFFSET(T, storage)}, states{states} {
    }
    void ensureStateIs(TypedPtr obj, u16 stateID) {
        PLY_ASSERT(obj.type == this);
        u16 oldStateID = *(u16*) obj.ptr;
        if (oldStateID != stateID) {
            void* storage = PLY_PTR_OFFSET(obj.ptr, storageOffset);
            TypedPtr{storage, states[oldStateID].structType}.destruct();
            *(u16*) obj.ptr = stateID;
            TypedPtr{storage, states[stateID].structType}.construct();
        }
    }
};

// clang-format off
#define PLY_SWITCH_REFLECT(...) \
    template <typename> friend struct ply::TypeDescriptorSpecializer; \
    static __VA_ARGS__ ply::TypeDescriptor* bindTypeDescriptor(); \
    static ply::Initializer initTypeDescriptor; \
    static void initTypeDescriptorStates();

#define PLY_SWITCH_BEGIN(type) \
    PLY_NO_INLINE ply::TypeDescriptor* type::bindTypeDescriptor() { \
        static ply::TypeDescriptor_Switch typeDesc{(type*) nullptr, #type}; \
        return &typeDesc; \
    } \
    ply::Initializer type::initTypeDescriptor{type::initTypeDescriptorStates}; \
    void type::initTypeDescriptorStates() { \
        using T = type; \
        PLY_UNUSED((T*) nullptr); \
        auto* switchType = T::bindTypeDescriptor()->cast<ply::TypeDescriptor_Switch>(); \
        switchType->states = {

#if PLY_WITH_ASSERTS
#define PLY_SWITCH_MEMBER(id) \
            {#id, (u16) T::ID::id, ply::getTypeDescriptor<T::id>()->cast<ply::TypeDescriptor_Struct>()},
#else
#define PLY_SWITCH_MEMBER(id) \
            {#id, ply::getTypeDescriptor<T::id>()},
#endif

#define PLY_SWITCH_END() \
        }; \
        PLY_ASSERT(switchType->states.numItems() == (u32) T::ID::Count); \
        for (u32 i = 0; i < switchType->states.numItems(); i++) { \
            PLY_ASSERT(switchType->states[i].id == i); \
        } \
    }
// clang-format on

//-----------------------------------------------------------------------
// OwnTypedPtr
//-----------------------------------------------------------------------
struct OwnTypedPtr : TypedPtr {
    OwnTypedPtr() {
    }
    OwnTypedPtr(void* ptr, TypeDescriptor* type) : TypedPtr{ptr, type} {
    }
    OwnTypedPtr(TypedPtr&& other) : TypedPtr{other.ptr, other.type} {
    }
    OwnTypedPtr(OwnTypedPtr&& other) : TypedPtr{other.ptr, other.type} {
        other.ptr = nullptr;
        other.type = nullptr;
    }
    void operator=(TypedPtr&& other) {
        destroy();
        ptr = other.ptr;
        type = other.type;
    }
    void operator=(OwnTypedPtr&& other) {
        destroy();
        ptr = other.ptr;
        type = other.type;
        other.ptr = nullptr;
        other.type = nullptr;
    }
    ~OwnTypedPtr() {
        destroy();
    }
    template <typename T>
    void assign(Owned<T>&& other) {
        destroy();
        ptr = other.release();
        type = getTypeDescriptor<T>();
    }
    template <typename T>
    static OwnTypedPtr bind(T* ptr) {
        // FIXME: Find a better way to handle cases where this function is passed a pointer to
        // const.
        return OwnTypedPtr{(void*) ptr, TypeDescriptorSpecializer<T>::get()};
    }
    template <typename T, typename... Args>
    static OwnTypedPtr create(Args&&... args) {
        return bind(new T{std::forward<Args>(args)...});
    }
    template <typename T>
    T* release();
};

template <>
struct TypeDescriptorSpecializer<OwnTypedPtr> {
    static PLY_DLL_ENTRY ply::TypeDescriptor* get();
};

//-----------------------------------------------------------------------
// TypedPtr member functions
//-----------------------------------------------------------------------
template <typename T>
TypedPtr TypedPtr::bind(T* ptr) {
    // FIXME: Find a better way to handle cases where this function is passed a pointer to
    // const.
    return TypedPtr{(void*) ptr, TypeDescriptorSpecializer<T>::get()};
}

template <class T>
bool TypedPtr::is() const {
    return TypeDescriptorSpecializer<T>::get()->isEquivalentTo(type);
}

template <class T>
T* TypedPtr::cast() const {
    // Not sure if this is a good general strategy, but for now, it's valid to cast a null
    // pointer to any target type, regardless of src type (even if src type is null). This
    // extra flexibility was added to simplify callers of readObject(), such as
    // CookCommandReader(), when an unexpected EOF is encountered:
    PLY_ASSERT(!ptr || TypeDescriptorSpecializer<T>::get()->isEquivalentTo(type));
    return (T*) ptr;
}

template <class T>
T* TypedPtr::safeCast() const {
    return (TypeDescriptorSpecializer<T>::get() == this->type) ? (T*) ptr : nullptr;
}

template <class S>
const S& TypedPtr::refine() const {
    PLY_ASSERT(type->typeKey == S::TypeDescriptor::typeKey);
    return (const S&) *this;
}

inline TypedPtr TypedPtr::create(TypeDescriptor* typeDesc) {
    return typeDesc->bindings.create(typeDesc);
}

inline void TypedPtr::destroy() {
    if (ptr) {
        type->bindings.destroy(*this);
        ptr = nullptr;
        type = nullptr;
    }
}

inline void TypedPtr::construct() {
    type->bindings.construct(*this);
}

inline void TypedPtr::destruct() {
    type->bindings.destruct(*this);
    ptr = nullptr;
    type = nullptr;
}

inline void TypedPtr::move(TypedPtr other) {
    type->bindings.move(*this, other);
}

inline void TypedPtr::copy(const TypedPtr other) {
    type->bindings.copy(*this, other);
}

template <typename T>
PLY_INLINE T* OwnTypedPtr::release() {
    PLY_ASSERT(type->isEquivalentTo(TypeDescriptorSpecializer<T>::get()));
    T* r = (T*) ptr;
    ptr = nullptr;
    type = nullptr;
    return r;
}

} // namespace ply
