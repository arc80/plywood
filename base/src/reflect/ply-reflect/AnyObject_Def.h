﻿/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-reflect/Core.h>

namespace ply {

struct TypeDescriptor;

//-----------------------------------------------------------------------
//  AnyObject
//-----------------------------------------------------------------------
struct AnyObject {
    void* data = nullptr;
    TypeDescriptor* type = nullptr;

    AnyObject() = default;
    AnyObject(void* data, TypeDescriptor* type) : data{data}, type{type} {
    }

    template <typename T>
    static AnyObject bind(T* data);
    template <class T>
    bool is() const;
    template <class T>
    T* cast() const;
    template <class T>
    T* safe_cast() const;
    template <class S>
    const S& refine() const;

    static AnyObject create(TypeDescriptor* type_desc);
    void destroy();
    void construct();
    void destruct();
    void move(AnyObject other);
    void copy(const AnyObject other);

    PLY_INLINE operator bool() const {
        return this->data != nullptr;
    }
    PLY_INLINE bool operator==(const AnyObject& other) const {
        return this->data == other.data && this->type == other.type;
    }
};

} // namespace ply
