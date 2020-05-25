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

PLY_NO_INLINE TypeDescriptor* TypeResolver<Float2>::get() {
    static TypeDescriptor_FixedArray typeDesc{TypeResolver<float>::get(), 2};
    return &typeDesc;
}

PLY_NO_INLINE TypeDescriptor* TypeResolver<Float3>::get() {
    static TypeDescriptor_FixedArray typeDesc{TypeResolver<float>::get(), 3};
    return &typeDesc;
}

PLY_NO_INLINE TypeDescriptor* TypeResolver<Float4>::get() {
    static TypeDescriptor_FixedArray typeDesc{TypeResolver<float>::get(), 4};
    return &typeDesc;
}

PLY_NO_INLINE TypeDescriptor* TypeResolver<Quaternion>::get() {
    static TypeDescriptor_FixedArray typeDesc{TypeResolver<float>::get(), 4};
    return &typeDesc;
}

PLY_NO_INLINE TypeDescriptor* TypeResolver<Float2x2>::get() {
    static TypeDescriptor_FixedArray typeDesc{TypeResolver<Float2>::get(), 2};
    return &typeDesc;
}

PLY_NO_INLINE TypeDescriptor* TypeResolver<Float3x3>::get() {
    static TypeDescriptor_FixedArray typeDesc{TypeResolver<Float3>::get(), 3};
    return &typeDesc;
}

PLY_NO_INLINE TypeDescriptor* TypeResolver<Float3x4>::get() {
    static TypeDescriptor_FixedArray typeDesc{TypeResolver<Float3>::get(), 4};
    return &typeDesc;
}

PLY_NO_INLINE TypeDescriptor* TypeResolver<Float4x4>::get() {
    static TypeDescriptor_FixedArray typeDesc{TypeResolver<Float4>::get(), 4};
    return &typeDesc;
}

PLY_NO_INLINE TypeDescriptor* TypeResolver<Box<Int2<u16>>>::get() {
    static TypeDescriptor_FixedArray typeDesc{TypeResolver<Int2<u16>>::get(), 2};
    return &typeDesc;
}

PLY_NO_INLINE TypeDescriptor* TypeResolver<Rect>::get() {
    static TypeDescriptor_FixedArray typeDesc{TypeResolver<Float2>::get(), 2};
    return &typeDesc;
};

PLY_NO_INLINE TypeDescriptor* TypeResolver<Box<Int2<s16>>>::get() {
    static TypeDescriptor_FixedArray typeDesc{TypeResolver<Int2<s16>>::get(), 2};
    return &typeDesc;
}

PLY_NO_INLINE TypeDescriptor* TypeResolver<Int2<u16>>::get() {
    static TypeDescriptor_FixedArray typeDesc{TypeResolver<u16>::get(), 2};
    return &typeDesc;
};

PLY_NO_INLINE TypeDescriptor* TypeResolver<Int2<s16>>::get() {
    static TypeDescriptor_FixedArray typeDesc{TypeResolver<s16>::get(), 2};
    return &typeDesc;
};

PLY_NO_INLINE TypeDescriptor* TypeResolver<Int3<u8>>::get() {
    static TypeDescriptor_FixedArray typeDesc{TypeResolver<u8>::get(), 3};
    return &typeDesc;
};

} // namespace ply

PLY_STRUCT_BEGIN_PRIM(ply::QuatPos)
PLY_STRUCT_MEMBER(quat)
PLY_STRUCT_MEMBER(pos)
PLY_STRUCT_END_PRIM()

PLY_STRUCT_BEGIN_PRIM(ply::QuatPosScale)
PLY_STRUCT_MEMBER(quat)
PLY_STRUCT_MEMBER(pos)
PLY_STRUCT_MEMBER(scale)
PLY_STRUCT_END_PRIM()

PLY_ENUM_BEGIN(ply::Axis3)
PLY_ENUM_IDENTIFIER(XPos)
PLY_ENUM_IDENTIFIER(XNeg)
PLY_ENUM_IDENTIFIER(YPos)
PLY_ENUM_IDENTIFIER(YNeg)
PLY_ENUM_IDENTIFIER(ZPos)
PLY_ENUM_IDENTIFIER(ZNeg)
PLY_ENUM_END()

PLY_ENUM_BEGIN(ply::Axis2)
PLY_ENUM_IDENTIFIER(XPos)
PLY_ENUM_IDENTIFIER(YPos)
PLY_ENUM_IDENTIFIER(XNeg)
PLY_ENUM_IDENTIFIER(YNeg)
PLY_ENUM_END()

PLY_STRUCT_BEGIN_PRIM(ply::AxisRot)
PLY_STRUCT_MEMBER(cols)
PLY_STRUCT_END_PRIM()

PLY_STRUCT_BEGIN_PRIM(ply::AxisRotPos)
PLY_STRUCT_MEMBER(rot)
PLY_STRUCT_MEMBER(pos)
PLY_STRUCT_END_PRIM()
