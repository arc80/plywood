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

PLY_NO_INLINE TypeDescriptor* getReflection(Float2*) {
    static TypeDescriptor_FixedArray typeDesc{TypeResolver<float>::get(), 2};
    return &typeDesc;
}

PLY_NO_INLINE TypeDescriptor* getReflection(Float3*) {
    static TypeDescriptor_FixedArray typeDesc{TypeResolver<float>::get(), 3};
    return &typeDesc;
}

PLY_NO_INLINE TypeDescriptor* getReflection(Float4*) {
    static TypeDescriptor_FixedArray typeDesc{TypeResolver<float>::get(), 4};
    return &typeDesc;
}

PLY_NO_INLINE TypeDescriptor* getReflection(Quaternion*) {
    static TypeDescriptor_FixedArray typeDesc{TypeResolver<float>::get(), 4};
    return &typeDesc;
}

PLY_NO_INLINE TypeDescriptor* getReflection(Float2x2*) {
    static TypeDescriptor_FixedArray typeDesc{TypeResolver<Float2>::get(), 2};
    return &typeDesc;
}

PLY_NO_INLINE TypeDescriptor* getReflection(Float3x3*) {
    static TypeDescriptor_FixedArray typeDesc{TypeResolver<Float3>::get(), 3};
    return &typeDesc;
}

PLY_NO_INLINE TypeDescriptor* getReflection(Float3x4*) {
    static TypeDescriptor_FixedArray typeDesc{TypeResolver<Float3>::get(), 4};
    return &typeDesc;
}

PLY_NO_INLINE TypeDescriptor* getReflection(Float4x4*) {
    static TypeDescriptor_FixedArray typeDesc{TypeResolver<Float4>::get(), 4};
    return &typeDesc;
}

PLY_NO_INLINE TypeDescriptor* getReflection(Box<Int2<u16>>*) {
    static TypeDescriptor_FixedArray typeDesc{TypeResolver<Int2<u16>>::get(), 2};
    return &typeDesc;
}

PLY_NO_INLINE TypeDescriptor* getReflection(Box<Float2>*) {
    static TypeDescriptor_FixedArray typeDesc{TypeResolver<Float2>::get(), 2};
    return &typeDesc;
};

PLY_NO_INLINE TypeDescriptor* getReflection(Box<Int2<s16>>*) {
    static TypeDescriptor_FixedArray typeDesc{TypeResolver<Int2<s16>>::get(), 2};
    return &typeDesc;
}

PLY_NO_INLINE TypeDescriptor* getReflection(Int2<u16>*) {
    static TypeDescriptor_FixedArray typeDesc{TypeResolver<u16>::get(), 2};
    return &typeDesc;
};

PLY_NO_INLINE TypeDescriptor* getReflection(Int2<s16>*) {
    static TypeDescriptor_FixedArray typeDesc{TypeResolver<s16>::get(), 2};
    return &typeDesc;
};

PLY_NO_INLINE TypeDescriptor* getReflection(Int3<u8>*) {
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

PLY_STRUCT_BEGIN_PRIM(ply::AxisRot)
PLY_STRUCT_MEMBER(cols)
PLY_STRUCT_END_PRIM()

PLY_STRUCT_BEGIN_PRIM(ply::AxisRotPos)
PLY_STRUCT_MEMBER(rot)
PLY_STRUCT_MEMBER(pos)
PLY_STRUCT_END_PRIM()
