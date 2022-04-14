/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-crowbar/Core.h>
#include <ply-reflect/methods/ObjectStack.h>
#include <ply-crowbar/ParseTree.h>

namespace ply {
namespace crowbar {

struct VariableMapTraits {
    struct Item {
        u32 key = 0;
        AnyObject obj;
        Item(u32 key) : key{key} {
        }
    };
    using Key = u32;
    static PLY_INLINE bool match(const Item& item, Key key) {
        return item.key == key;
    }
};

struct Interpreter {
    struct StackFrame {
        Interpreter* interp = nullptr;
        HashMap<VariableMapTraits> localVariableTable;
    };

    const InternedStrings* internedStrings = nullptr;
    ObjectStack localVariableStorage;
    AnyObject returnValue;
    Array<HashMap<VariableMapTraits>*> outerNameSpaces;
};

void execFunction(Interpreter::StackFrame* frame, const StatementBlock* block);

} // namespace crowbar
} // namespace ply