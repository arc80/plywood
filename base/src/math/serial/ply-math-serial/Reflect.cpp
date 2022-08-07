/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-math/Core.h>
#include <ply-math-serial/Reflect.h>
#include <ply-math/Matrix.h>
#include <ply-math/QuatPos.h>
#include <ply-math/AxisVector.h>
#include <ply-math/IntVector.h>

namespace ply {

PLY_DEFINE_TYPE_DESCRIPTOR(Float2) {
    static TypeDescriptor_FixedArray typeDesc{getTypeDescriptor<float>(), 2};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(Float3) {
    static TypeDescriptor_FixedArray typeDesc{getTypeDescriptor<float>(), 3};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(Float4) {
    static TypeDescriptor_FixedArray typeDesc{getTypeDescriptor<float>(), 4};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(Quaternion) {
    static TypeDescriptor_FixedArray typeDesc{getTypeDescriptor<float>(), 4};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(Float2x2) {
    static TypeDescriptor_FixedArray typeDesc{getTypeDescriptor<Float2>(), 2};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(Float3x3) {
    static TypeDescriptor_FixedArray typeDesc{getTypeDescriptor<Float3>(), 3};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(Float3x4) {
    static TypeDescriptor_FixedArray typeDesc{getTypeDescriptor<Float3>(), 4};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(Float4x4) {
    static TypeDescriptor_FixedArray typeDesc{getTypeDescriptor<Float4>(), 4};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(Box<Int2<u16>>) {
    static TypeDescriptor_FixedArray typeDesc{getTypeDescriptor<Int2<u16>>(), 2};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(Box<Float2>) {
    static TypeDescriptor_FixedArray typeDesc{getTypeDescriptor<Float2>(), 2};
    return &typeDesc;
};

PLY_DEFINE_TYPE_DESCRIPTOR(Box<Int2<s16>>) {
    static TypeDescriptor_FixedArray typeDesc{getTypeDescriptor<Int2<s16>>(), 2};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(Int2<u16>) {
    static TypeDescriptor_FixedArray typeDesc{getTypeDescriptor<u16>(), 2};
    return &typeDesc;
};

PLY_DEFINE_TYPE_DESCRIPTOR(Int2<s16>) {
    static TypeDescriptor_FixedArray typeDesc{getTypeDescriptor<s16>(), 2};
    return &typeDesc;
};

PLY_DEFINE_TYPE_DESCRIPTOR(Int3<u8>) {
    static TypeDescriptor_FixedArray typeDesc{getTypeDescriptor<u8>(), 3};
    return &typeDesc;
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

PLY_ENUM_BEGIN(ply::, Axis3)
PLY_ENUM_IDENTIFIER(XPos)
PLY_ENUM_IDENTIFIER(XNeg)
PLY_ENUM_IDENTIFIER(YPos)
PLY_ENUM_IDENTIFIER(YNeg)
PLY_ENUM_IDENTIFIER(ZPos)
PLY_ENUM_IDENTIFIER(ZNeg)
PLY_ENUM_END()

PLY_ENUM_BEGIN(ply::, Axis2)
PLY_ENUM_IDENTIFIER(XPos)
PLY_ENUM_IDENTIFIER(YPos)
PLY_ENUM_IDENTIFIER(XNeg)
PLY_ENUM_IDENTIFIER(YNeg)
PLY_ENUM_END()

