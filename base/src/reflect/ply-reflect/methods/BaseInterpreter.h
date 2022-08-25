/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/methods/ObjectStack.h>
#include <ply-reflect/AnyObject.h>
#include <ply-runtime/io/OutStream.h>

namespace ply {

struct BaseInterpreter {
    ObjectStack localVariableStorage;
    AnyObject returnValue;
    OutStream* outs = nullptr;

    ~BaseInterpreter() {
    }
    virtual void error(StringView message) {
    }
};

} // namespace ply
