/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/FormatDescriptor.h>

namespace ply {

FormatDescriptor FormatDescriptor_Bool{u32(FormatKey::Bool)};
FormatDescriptor FormatDescriptor_S8{u32(FormatKey::S8)};
FormatDescriptor FormatDescriptor_S16{u32(FormatKey::S16)};
FormatDescriptor FormatDescriptor_S32{u32(FormatKey::S32)};
FormatDescriptor FormatDescriptor_S64{u32(FormatKey::S64)};
FormatDescriptor FormatDescriptor_U8{u32(FormatKey::U8)};
FormatDescriptor FormatDescriptor_U16{u32(FormatKey::U16)};
FormatDescriptor FormatDescriptor_U32{u32(FormatKey::U32)};
FormatDescriptor FormatDescriptor_U64{u32(FormatKey::U64)};
FormatDescriptor FormatDescriptor_Float{u32(FormatKey::Float)};
FormatDescriptor FormatDescriptor_Double{u32(FormatKey::Double)};
FormatDescriptor FormatDescriptor_String{u32(FormatKey::String)};
FormatDescriptor FormatDescriptor_TypedArray{u32(FormatKey::TypedArray)};
FormatDescriptor FormatDescriptor_Typed{u32(FormatKey::Typed)};

} // namespace ply
