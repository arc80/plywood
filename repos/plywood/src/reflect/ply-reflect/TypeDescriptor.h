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
// TypeResolver
//--------------------------------------------------------
template <typename T>
struct TypeResolver {
    PLY_MAKE_WELL_FORMEDNESS_CHECK_1(HasReflectionMember, T0::getReflection())
    PLY_MAKE_WELL_FORMEDNESS_CHECK_1(HasReflectionFriend, getReflection((T0*) nullptr))

    template <typename U = T, std::enable_if_t<HasReflectionMember<U>, int> = 0>
    static PLY_INLINE auto* get() {
        return T::getReflection();
    }
    template <typename U = T, std::enable_if_t<HasReflectionFriend<U>, int> = 0>
    static PLY_INLINE auto* get() {
        return getReflection((U*) nullptr);
    }
};

#define PLY_REFLECT_ENUM(linkage, name) linkage ::ply::TypeDescriptor_Enum* getReflection(name*);

#define PLY_DECLARE_TYPE_DESCRIPTOR(linkage, suffix, name) \
    linkage ::ply::TypeDescriptor##suffix* getReflection(name*);

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
PLY_DLL_ENTRY extern TypeKey TypeKey_WeakPtr;
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
                PLY_ASSERT(typeDesc == TypeResolver<T>::get());
                return {subst::createByMember<T>(), typeDesc};
            },
            // destroy
            [](TypedPtr obj) {
                PLY_ASSERT(obj.type == TypeResolver<T>::get());
                subst::destroyByMember((T*) obj.ptr);
            },
            // construct
            [](TypedPtr obj) {
                PLY_ASSERT(obj.type == TypeResolver<T>::get());
                subst::unsafeConstruct((T*) obj.ptr);
            },
            // destruct
            [](TypedPtr obj) {
                PLY_ASSERT(obj.type == TypeResolver<T>::get());
                subst::destructByMember((T*) obj.ptr);
            },
            // move
            [](TypedPtr dst, TypedPtr src) {
                PLY_ASSERT(dst.type == TypeResolver<T>::get());
                PLY_ASSERT(src.type == TypeResolver<T>::get());
                subst::unsafeMove((T*) dst.ptr, (T*) src.ptr);
            },
            // copy
            [](TypedPtr dst, const TypedPtr src) {
                PLY_ASSERT(dst.type == TypeResolver<T>::get());
                PLY_ASSERT(src.type == TypeResolver<T>::get());
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
template <>
struct TypeResolver<bool> {
    static PLY_DLL_ENTRY ply::TypeDescriptor* get();
};
template <>
struct TypeResolver<s8> {
    static PLY_DLL_ENTRY ply::TypeDescriptor* get();
};
template <>
struct TypeResolver<s16> {
    static PLY_DLL_ENTRY ply::TypeDescriptor* get();
};
template <>
struct TypeResolver<s32> {
    static PLY_DLL_ENTRY ply::TypeDescriptor* get();
};
template <>
struct TypeResolver<s64> {
    static PLY_DLL_ENTRY ply::TypeDescriptor* get();
};
template <>
struct TypeResolver<u8> {
    static PLY_DLL_ENTRY ply::TypeDescriptor* get();
};
template <>
struct TypeResolver<u16> {
    static PLY_DLL_ENTRY ply::TypeDescriptor* get();
};
template <>
struct TypeResolver<u32> {
    static PLY_DLL_ENTRY ply::TypeDescriptor* get();
};
template <>
struct TypeResolver<u64> {
    static PLY_DLL_ENTRY ply::TypeDescriptor* get();
};
template <>
struct TypeResolver<float> {
    static PLY_DLL_ENTRY ply::TypeDescriptor* get();
};
template <>
struct TypeResolver<double> {
    static PLY_DLL_ENTRY ply::TypeDescriptor* get();
};
template <>
struct TypeResolver<String> {
    static PLY_DLL_ENTRY ply::TypeDescriptor* get();
};

//-----------------------------------------------------------------------
// Strip const qualifiers
//-----------------------------------------------------------------------
template <typename T>
struct TypeResolver<const T> {
    static TypeDescriptor* get() {
        return TypeResolver<T>::get();
    }
};

//-----------------------------------------------------------------------
// C-style arrays
//-----------------------------------------------------------------------
PLY_DLL_ENTRY NativeBindings& getNativeBindings_FixedArray();

struct TypeDescriptor_FixedArray : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* typeKey; // FIXME: Make these all constexpr
    TypeDescriptor* itemType;
    u32 numItems;
    u32 stride;

    TypeDescriptor_FixedArray(TypeDescriptor* itemType, u32 numItems)
        : TypeDescriptor{&TypeKey_FixedArray, itemType->fixedSize * numItems,
                         getNativeBindings_FixedArray()},
          itemType{itemType}, numItems{numItems}, stride{itemType->fixedSize} {
    }
    TypeDescriptor_FixedArray(TypeDescriptor* itemType, u32 numItems, u32 stride)
        : TypeDescriptor{&TypeKey_FixedArray, stride * numItems, getNativeBindings_FixedArray()},
          itemType{itemType}, numItems{numItems}, stride{stride} {
        PLY_ASSERT(stride >= itemType->fixedSize);
    }
};

template <typename T, int numItems>
struct TypeResolver<T[numItems]> {
    static TypeDescriptor* get() {
        static TypeDescriptor_FixedArray typeDesc{TypeResolver<T>::get(), u32(numItems)};
        return &typeDesc;
    }
};

//-----------------------------------------------------------------------
// FixedArray<*>
//-----------------------------------------------------------------------
template <typename T, u32 Size>
struct TypeResolver<FixedArray<T, Size>> {
    static TypeDescriptor* get() {
        static TypeDescriptor_FixedArray typeDesc{TypeResolver<T>::get(), Size};
        return &typeDesc;
    }
};

//-----------------------------------------------------------------------
// Array<*>
//-----------------------------------------------------------------------
PLY_DLL_ENTRY NativeBindings& getNativeBindings_Array();

struct TypeDescriptor_Array : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* typeKey;
    TypeDescriptor* itemType;

    TypeDescriptor_Array(TypeDescriptor* itemType)
        : TypeDescriptor{&TypeKey_Array, sizeof(details::BaseArray), getNativeBindings_Array()},
          itemType{itemType} {
    }
};

template <typename T>
struct TypeResolver<Array<T>> {
    static TypeDescriptor* get() {
        static TypeDescriptor_Array typeDesc{TypeResolver<T>::get()};
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
// Owned<*>
//-----------------------------------------------------------------------
PLY_DLL_ENTRY NativeBindings& getNativeBindings_Owned();

struct TypeDescriptor_Owned : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* typeKey;
    TypeDescriptor* targetType;
    void (*assignRawPtr)(TypedPtr ownedPtr, TypedPtr target) = nullptr;

    template <typename T>
    TypeDescriptor_Owned(T*)
        : TypeDescriptor{&TypeKey_Owned, sizeof(void*), getNativeBindings_Owned()},
          targetType{TypeResolver<T>::get()} {
        assignRawPtr = [](TypedPtr ownedPtr, TypedPtr target) {
            // FIXME: Check that the target type is compatible
            // If the target uses multiple inheritance, may need to adjust its pointer value to
            // store as a base class object!
            *reinterpret_cast<Owned<T>*>(ownedPtr.ptr) = (T*) target.ptr;
        };
    }
};

template <typename T>
struct TypeResolver<Owned<T>> {
    static TypeDescriptor* get() {
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
        PLY_ASSERT(type->targetType->isEquivalentTo(TypeResolver<T>::get()));
        return reinterpret_cast<Owned<T>*>(ptr)->get();
    }
};

//-----------------------------------------------------------------------
// Reference<*>
//-----------------------------------------------------------------------
PLY_DLL_ENTRY NativeBindings& getNativeBindings_Reference();

struct TypeDescriptor_Reference : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* typeKey;
    TypeDescriptor* targetType;
    void (*incRef)(void* target) = nullptr;
    void (*decRef)(void* target) = nullptr;

    template <typename T>
    TypeDescriptor_Reference(T*)
        : TypeDescriptor{&TypeKey_Reference, sizeof(void*), getNativeBindings_Reference()},
          targetType{TypeResolver<T>::get()} {
        this->incRef = [](void* target) { //
            ((T*) target)->incRef();
        };
        this->decRef = [](void* target) { //
            ((T*) target)->decRef();
        };
    }
};

template <typename T>
struct TypeResolver<Reference<T>> {
    static TypeDescriptor* get() {
        static TypeDescriptor_Reference typeDesc{(T*) nullptr};
        return &typeDesc;
    }
};

//-----------------------------------------------------------------------
// struct
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
    static auto invokePostSerialize(T* obj) -> decltype(obj->onPostSerialize()) {
        return obj->onPostSerialize();
    }
    inline static void invokePostSerialize(...) {
    }
    void (*onPostSerialize)(void* ptr) = [](void*) {};

    // Constructor for synthesized TypeDescriptor_Struct:
    TypeDescriptor_Struct(u32 fixedSize, StringView name)
        : TypeDescriptor{&TypeKey_Struct, fixedSize, getNativeBindings_SynthesizedStruct()},
          name{name} {
    }
    // Constructor for TypeDescriptor_Struct of an existing C++ class:
    template <class T>
    TypeDescriptor_Struct(T*, StringView name, std::initializer_list<Member> members = {})
        : TypeDescriptor{&TypeKey_Struct, sizeof(T), NativeBindings::make<T>()}, name{name},
          members{members} {
        onPostSerialize = [](void* ptr) { invokePostSerialize((T*) ptr); };
    }

    void appendMember(StringView name, TypeDescriptor* type) {
        // FIXME: Handle alignment
        members.append({name, fixedSize, type});
        fixedSize += type->fixedSize;
    }

    const Member* findMember(StringView name) const {
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

#define PLY_REFLECT(...) \
    template <typename> \
    friend struct ply::TypeResolver; \
    static __VA_ARGS__ ply::TypeDescriptor_Struct* getReflection(); \
    static ply::Initializer initReflectionObj; \
    static void initReflectionMembers();

#define PLY_STATE_REFLECT(name) \
    PLY_STATE(name) \
    PLY_REFLECT()

#define PLY_STRUCT_BEGIN(type) \
    PLY_NO_INLINE ply::TypeDescriptor_Struct* type::getReflection() { \
        static ply::TypeDescriptor_Struct typeDesc{(type*) nullptr, #type}; \
        return &typeDesc; \
    } \
    ply::Initializer type::initReflectionObj{type::initReflectionMembers}; \
    void type::initReflectionMembers() { \
        using T = type; \
        PLY_UNUSED((T*) nullptr); \
        getReflection()->members = {

#define PLY_STRUCT_MEMBER(name) \
    {#name, PLY_MEMBER_OFFSET(T, name), ply::TypeResolver<decltype(T::name)>::get()},

#define PLY_STRUCT_END() \
    } \
    ; \
    }

#define PLY_STRUCT_BEGIN_PRIM(type) \
    PLY_NO_INLINE ply::TypeDescriptor_Struct* getReflection(type*) { \
        using T = type; \
        static ply::TypeDescriptor_Struct typeDesc { \
            (type*) nullptr, #type, {

#define PLY_STRUCT_END_PRIM() \
    } \
    } \
    ; \
    return &typeDesc; \
    }

//-----------------------------------------------------------------------
// enum
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

    TypeDescriptor_Enum(u32 fixedSize, const char* name,
                        std::initializer_list<Identifier> identifiers)
        : TypeDescriptor{&TypeKey_Enum, fixedSize, getNativeBindings_Enum()}, name{name},
          identifiers{identifiers} {
    }

    const Identifier* findIdentifier(StringView name) const {
        for (const Identifier& identifier : identifiers) {
            if (identifier.name == name)
                return &identifier;
        }
        return nullptr;
    }

    PLY_INLINE const Identifier* findValue(u32 value) const {
        for (const Identifier& identifier : identifiers) {
            if (identifier.value == value)
                return &identifier;
        }
        return nullptr;
    }
};

#define PLY_ENUM_BEGIN(ns, type) \
    PLY_NO_INLINE ply::TypeDescriptor_Enum* ns getReflection(type*) { \
        using T = type; \
        static ply::TypeDescriptor_Enum typeDesc { \
            sizeof(T), #type, {

#define PLY_ENUM_IDENTIFIER(name) {#name, ply::u32(T::name)},

#define PLY_ENUM_END() \
    } \
    } \
    ; \
    return &typeDesc; \
    }

//-----------------------------------------------------------------------
// Raw pointers
//-----------------------------------------------------------------------
PLY_DLL_ENTRY NativeBindings& getNativeBindings_WeakPtr();

struct TypeDescriptor_WeakPtr : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* typeKey;
    TypeDescriptor* targetType;

    TypeDescriptor_WeakPtr(TypeDescriptor* targetType)
        : TypeDescriptor{&TypeKey_WeakPtr, sizeof(void*), getNativeBindings_WeakPtr()},
          targetType{targetType} {
    }
};

template <typename T>
struct TypeResolver<T*> {
    static TypeDescriptor* get() {
        static TypeDescriptor_WeakPtr typeDesc{TypeResolver<T>::get()};
        return &typeDesc;
    }
};

//-----------------------------------------------------------------------
// Enum-indexed arrays
//-----------------------------------------------------------------------
PLY_DLL_ENTRY NativeBindings& getNativeBindings_EnumIndexedArray();

struct TypeDescriptor_EnumIndexedArray : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* typeKey;
    TypeDescriptor* itemType;
    TypeDescriptor_Enum* enumType;

    template <typename T, typename EnumType>
    TypeDescriptor_EnumIndexedArray(T*, EnumType*)
        : TypeDescriptor{&TypeKey_EnumIndexedArray, sizeof(EnumIndexedArray<T, EnumType>),
                         getNativeBindings_EnumIndexedArray()} {
        itemType = TypeResolver<T>::get();
        enumType = TypeResolver<EnumType>::get()->template cast<TypeDescriptor_Enum>();
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
struct TypeResolver<EnumIndexedArray<T, EnumType>> {
    static TypeDescriptor* get() {
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

#define PLY_SWITCH_REFLECT(...) \
    template <typename> \
    friend struct ply::TypeResolver; \
    static __VA_ARGS__ ply::TypeDescriptor_Switch* getReflection(); \
    static ply::Initializer initReflectionObj; \
    static void initReflectionStates();

#define PLY_SWITCH_BEGIN(type) \
    PLY_NO_INLINE ply::TypeDescriptor_Switch* type::getReflection() { \
        static ply::TypeDescriptor_Switch typeDesc{(type*) nullptr, #type}; \
        return &typeDesc; \
    } \
    ply::Initializer type::initReflectionObj{type::initReflectionStates}; \
    void type::initReflectionStates() { \
        using T = type; \
        PLY_UNUSED((T*) nullptr); \
        ply::TypeDescriptor_Switch* switchType = ply::TypeResolver<T>::get(); \
        switchType->states = {

#if PLY_WITH_ASSERTS
#define PLY_SWITCH_MEMBER(id) \
    {#id, (u16) T::ID::id, ply::TypeResolver<T::id>::get()->cast<TypeDescriptor_Struct>()},
#else
#define PLY_SWITCH_MEMBER(id) {#id, ply::TypeResolver<T::id>::get()->cast<TypeDescriptor_Struct>()},
#endif

#define PLY_SWITCH_END() \
    } \
    ; \
    PLY_ASSERT(switchType->states.numItems() == (u32) T::ID::Count); \
    for (u32 i = 0; i < switchType->states.numItems(); i++) { \
        PLY_ASSERT(switchType->states[i].id == i); \
    } \
    }

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
        type = TypeResolver<T>::get();
    }
    template <typename T>
    static OwnTypedPtr bind(T* ptr) {
        // FIXME: Find a better way to handle cases where this function is passed a pointer to
        // const.
        return OwnTypedPtr{(void*) ptr, TypeResolver<T>::get()};
    }
    template <typename T, typename... Args>
    static OwnTypedPtr create(Args&&... args) {
        return bind(new T{std::forward<Args>(args)...});
    }
    template <typename T>
    T* release();
};

template <>
struct TypeResolver<OwnTypedPtr> {
    static PLY_DLL_ENTRY ply::TypeDescriptor* get();
};

//-----------------------------------------------------------------------
// TypedPtr member functions
//-----------------------------------------------------------------------
template <typename T>
TypedPtr TypedPtr::bind(T* ptr) {
    // FIXME: Find a better way to handle cases where this function is passed a pointer to
    // const.
    return TypedPtr{(void*) ptr, TypeResolver<T>::get()};
}

template <class T>
bool TypedPtr::is() const {
    return TypeResolver<T>::get()->isEquivalentTo(type);
}

template <class T>
T* TypedPtr::cast() const {
    // Not sure if this is a good general strategy, but for now, it's valid to cast a null
    // pointer to any target type, regardless of src type (even if src type is null). This
    // extra flexibility was added to simplify callers of readObject(), such as
    // CookCommandReader(), when an unexpected EOF is encountered:
    PLY_ASSERT(!ptr || TypeResolver<T>::get()->isEquivalentTo(type));
    return (T*) ptr;
}

template <class T>
T* TypedPtr::safeCast() const {
    return (TypeResolver<T>::get() == this->type) ? (T*) ptr : nullptr;
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
    PLY_ASSERT(type->isEquivalentTo(TypeResolver<T>::get()));
    T* r = (T*) ptr;
    ptr = nullptr;
    type = nullptr;
    return r;
}

} // namespace ply
