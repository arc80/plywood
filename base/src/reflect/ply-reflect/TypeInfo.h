/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                   ┃
┃    ╱   ╱╲    Plywood Multimedia Toolkit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/          ┃
┃    └──┴┴┴┘                                 ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#pragma once
#include <ply-reflect/Core.h>

namespace ply {

//  ▄▄▄▄▄▄
//    ██   ▄▄  ▄▄ ▄▄▄▄▄   ▄▄▄▄
//    ██   ██  ██ ██  ██ ██▄▄██
//    ██   ▀█▄▄██ ██▄▄█▀ ▀█▄▄▄
//          ▄▄▄█▀ ██

struct Type {
    Label tag;
    Label name;
};

template <typename>
struct TypeFactory;

#define PLY_DECLARE_TYPE(metatype, type) \
    template <> \
    struct ::ply::TypeFactory<struct_type> { \
        static metatype type; \
    };

template <typename T>
constexpr auto get_type(T* = nullptr) {
    return &TypeFactory<T>::type;
}

//  ▄▄▄▄▄         ▄▄          ▄▄  ▄▄   ▄▄
//  ██  ██ ▄▄▄▄▄  ▄▄ ▄▄▄▄▄▄▄  ▄▄ ▄██▄▄ ▄▄ ▄▄   ▄▄  ▄▄▄▄   ▄▄▄▄
//  ██▀▀▀  ██  ▀▀ ██ ██ ██ ██ ██  ██   ██ ▀█▄ ▄█▀ ██▄▄██ ▀█▄▄▄
//  ██     ██     ██ ██ ██ ██ ██  ▀█▄▄ ██   ▀█▀   ▀█▄▄▄   ▄▄▄█▀
//

PLY_DECLARE_TYPE(Type, u8)
PLY_DECLARE_TYPE(Type, u16)
PLY_DECLARE_TYPE(Type, u32)
PLY_DECLARE_TYPE(Type, u64)
PLY_DECLARE_TYPE(Type, s8)
PLY_DECLARE_TYPE(Type, s16)
PLY_DECLARE_TYPE(Type, s32)
PLY_DECLARE_TYPE(Type, s64)
PLY_DECLARE_TYPE(Type, float)
PLY_DECLARE_TYPE(Type, double)
PLY_DECLARE_TYPE(Type, bool)
PLY_DECLARE_TYPE(Type, String)

struct Pointer : Type {
    Type* target_type;
    constexpr Pointer(Type* target_type)
        : Type{LABEL_pointer, LABEL_pointer}, target_type{target_type} {
    }
};

template <typename T>
struct TypeFactory<T*> {
    static constexpr Pointer type{get_type<T>()};
};

//   ▄▄▄▄   ▄▄                        ▄▄
//  ██  ▀▀ ▄██▄▄ ▄▄▄▄▄  ▄▄  ▄▄  ▄▄▄▄ ▄██▄▄
//   ▀▀▀█▄  ██   ██  ▀▀ ██  ██ ██     ██
//  ▀█▄▄█▀  ▀█▄▄ ██     ▀█▄▄██ ▀█▄▄▄  ▀█▄▄
//

struct Member {
    Label name;
    Type* type;
    u32 offset;
    u32 alignment;
};

struct Choice {
    Label name;
    Type* type;
};

struct Struct : Type {
    bool is_owner = true;
    Array<Member> members;
    Array<Choice> choices;

    // This constructor lets you synthesize new Structs at runtime.
    Struct(Label name) : Type{LABEL_struct, name} {
    }

    // This constructor should only be used to initialize a Struct from constexpr arrays
    // of Member and Choice. The Struct does not own or make copies of the provided
    // arrays and you can't modify the Struct after it's created.
    template <u32 M, u32 C>
    Struct(Label name, const Member (&m)[M], const Choice (&c)[C])
        : Type{LABEL_struct, name} {
        this->is_owner = false;
        this->members.adopt(m, static_array_size(m));
        this->choices.adopt(c, static_array_size(c));
    }

    ~Struct() {
        if (!this->is_owner) {
            // Don't free borrowed arrays.
            this->members.orphan();
            this->choices.orphan();
        }
    }
};

#define PLY_MEMBER(struct_type, member_name) \
    { \
        ::ply::LABEL_##member_name, \
            ::ply::get_type<decltype(struct_type::member_name)>(), \
            offsetof(struct_type, member_name), alignof(struct_type, member_name) \
    }

#define PLY_STRUCT(struct_type, label_name, members) \
    Struct ::ply::TypeFactory<struct_type>::type{LABEL_struct_##label_name, members};

//  ▄▄▄▄▄
//  ██    ▄▄▄▄▄  ▄▄  ▄▄ ▄▄▄▄▄▄▄
//  ██▀▀  ██  ██ ██  ██ ██ ██ ██
//  ██▄▄▄ ██  ██ ▀█▄▄██ ██ ██ ██
//

struct Enumerator {
    Label name;
    u32 value;
};

struct Enum : Type {
    bool is_owner = true;
    Array<Enumerator> enumerators;

    // This constructor should only be used to initialize an Enum from a constexpr array
    // of Enumerator. The Enum does not own or make a copy of the provided array and you
    // can't modify the Enum after it's created.
    template <u32 E>
    Enum(Label name, const Enumerator (&e)[E]) : Type{LABEL_enum, name} {
        this->is_owner = false;
        this->enumerators.adopt(e, static_array_size(e));
    }

    ~Enum() {
        if (!this->is_owner) {
            // Don't free borrowed array.
            this->enumerators.orphan();
        }
    }
};

#define PLY_ENUMERATOR(enum_type, enumerator_name) \
    { ::ply::LABEL_##enumerator_name, (u32) enum_type::enumerator_name }

#define PLY_ENUM(enum_type, label_name, enumerators) \
    Enum ::ply::TypeFactory<enum_type>::type{LABEL_enum_##label_name, members};

//  ▄▄▄▄▄                      ▄▄   ▄▄
//  ██    ▄▄  ▄▄ ▄▄▄▄▄   ▄▄▄▄ ▄██▄▄ ▄▄  ▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄
//  ██▀▀  ██  ██ ██  ██ ██     ██   ██ ██  ██ ██  ██ ▀█▄▄▄
//  ██    ▀█▄▄██ ██  ██ ▀█▄▄▄  ▀█▄▄ ██ ▀█▄▄█▀ ██  ██  ▄▄▄█▀
//

struct FnParams {
    ScratchPad* scratch;
    Any self;
    ArrayView<const Any> args;
};

using NativeFunction = FnResult(const FnParams& params);
template <typename T>
using NativeMethod = FnResult(T* self, const FnParams& params);

struct BoundNativeMethod {
    void* self = nullptr;
    NativeMethod<void>* func = nullptr;
};

template <typename T>
BoundNativeMethod bind(T* obj, NativeMethod<T>* func) {
    return {obj, func};
}

PLY_DECLARE_TYPE(Type, NativeFunction)
PLY_DECLARE_TYPE(Type, BoundNativeMethod)

} // namespace ply
