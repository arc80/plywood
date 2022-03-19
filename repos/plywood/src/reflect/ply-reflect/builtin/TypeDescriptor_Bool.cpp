/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_Bool.h>
#include <ply-reflect/AnyObject.h>

namespace ply {

PLY_DEFINE_TYPE_DESCRIPTOR(bool) {
    static TypeDescriptor typeDesc{&TypeKey_Bool, sizeof(bool),
                                   NativeBindings::make<bool>() PLY_METHOD_TABLES_ONLY(, {})};
    return &typeDesc;
};

} // namespace ply
