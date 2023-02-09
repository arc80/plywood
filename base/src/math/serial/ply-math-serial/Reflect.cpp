/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-math.h>
#include <ply-math-serial/Reflect.h>

namespace ply {

PLY_DEFINE_TYPE_DESCRIPTOR(vec2) {
    static TypeDescriptor_FixedArray type_desc{get_type_descriptor<float>(), 2};
    return &type_desc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(vec3) {
    static TypeDescriptor_FixedArray type_desc{get_type_descriptor<float>(), 3};
    return &type_desc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(vec4) {
    static TypeDescriptor_FixedArray type_desc{get_type_descriptor<float>(), 4};
    return &type_desc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(Quaternion) {
    static TypeDescriptor_FixedArray type_desc{get_type_descriptor<float>(), 4};
    return &type_desc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(mat2x2) {
    static TypeDescriptor_FixedArray type_desc{get_type_descriptor<vec2>(), 2};
    return &type_desc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(mat3x3) {
    static TypeDescriptor_FixedArray type_desc{get_type_descriptor<vec3>(), 3};
    return &type_desc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(mat3x4) {
    static TypeDescriptor_FixedArray type_desc{get_type_descriptor<vec3>(), 4};
    return &type_desc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(mat4x4) {
    static TypeDescriptor_FixedArray type_desc{get_type_descriptor<vec4>(), 4};
    return &type_desc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(Box<TVec2<u16>>) {
    static TypeDescriptor_FixedArray type_desc{get_type_descriptor<TVec2<u16>>(), 2};
    return &type_desc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(Box<vec2>) {
    static TypeDescriptor_FixedArray type_desc{get_type_descriptor<vec2>(), 2};
    return &type_desc;
};

PLY_DEFINE_TYPE_DESCRIPTOR(Box<TVec2<s16>>) {
    static TypeDescriptor_FixedArray type_desc{get_type_descriptor<TVec2<s16>>(), 2};
    return &type_desc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(TVec2<u16>) {
    static TypeDescriptor_FixedArray type_desc{get_type_descriptor<u16>(), 2};
    return &type_desc;
};

PLY_DEFINE_TYPE_DESCRIPTOR(TVec2<s16>) {
    static TypeDescriptor_FixedArray type_desc{get_type_descriptor<s16>(), 2};
    return &type_desc;
};

PLY_DEFINE_TYPE_DESCRIPTOR(TVec3<u8>) {
    static TypeDescriptor_FixedArray type_desc{get_type_descriptor<u8>(), 3};
    return &type_desc;
};

PLY_STRUCT_BEGIN_PRIM(QuatPos)
PLY_STRUCT_MEMBER(quat)
PLY_STRUCT_MEMBER(pos)
PLY_STRUCT_END_PRIM()

PLY_STRUCT_BEGIN_PRIM(QuatPosScale)
PLY_STRUCT_MEMBER(quat)
PLY_STRUCT_MEMBER(pos)
PLY_STRUCT_MEMBER(scale)
PLY_STRUCT_END_PRIM()

PLY_STRUCT_BEGIN_PRIM(AxisRot)
PLY_STRUCT_MEMBER(cols)
PLY_STRUCT_END_PRIM()

PLY_STRUCT_BEGIN_PRIM(AxisRotPos)
PLY_STRUCT_MEMBER(rot)
PLY_STRUCT_MEMBER(pos)
PLY_STRUCT_END_PRIM()

} // namespace ply

PLY_ENUM_BEGIN(ply::, Axis)
PLY_ENUM_IDENTIFIER(XPos)
PLY_ENUM_IDENTIFIER(XNeg)
PLY_ENUM_IDENTIFIER(YPos)
PLY_ENUM_IDENTIFIER(YNeg)
PLY_ENUM_IDENTIFIER(ZPos)
PLY_ENUM_IDENTIFIER(ZNeg)
PLY_ENUM_END()

