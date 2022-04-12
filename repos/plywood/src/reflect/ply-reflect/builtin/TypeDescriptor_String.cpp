/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_String.h>
#include <ply-reflect/AnyObject.h>

namespace ply {

PLY_DEFINE_TYPE_DESCRIPTOR(String) {
    static TypeDescriptor typeDesc{
        &TypeKey_String, (String*) nullptr, NativeBindings::make<String>(), {}};
    return &typeDesc;
}

} // namespace ply
