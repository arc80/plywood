/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/Base.h>

namespace ply {

struct Error_t {
    void log_internal(StringView fmt, ArrayView<const FormatArg> args);
    template <typename... Args>
    void log(StringView fmt, Args&&... args) {
        FixedArray<FormatArg, sizeof...(Args)> arg_list;
        impl::InitItems<FormatArg>::init(arg_list.view().items, std::forward<Args>(args)...);
        this->log_internal(fmt, arg_list);
    }
};

extern Error_t Error;

} // namespace ply
