/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeDescriptor.h>
#include <ply-reflect/FormatDescriptor.h>
#include <map>

namespace ply {

#if _DEBUG
#define PLY_DEBUG_WRITE_FORMAT_CONTEXT 1 // FIXME: Make this part of a global config
#endif

class WriteFormatContext {
private:
    NativeEndianWriter m_out;
    std::map<TypeDescriptor*, u32> m_typeToFormatID;
    u32 m_nextUserFormatID = FormatID_StartUserRange;
#if PLY_DEBUG_WRITE_FORMAT_CONTEXT
    Array<TypeDescriptor*> m_userFormatTypeDescs;
#endif

    u32 addFormatDesc(TypeDescriptor* typeDesc);
    void writeChildFormatDesc(TypeDescriptor* typeDesc);

public:
    WriteFormatContext(OutStream& out);
    u32 addOrGetFormatID(TypeDescriptor* typeDesc);
    u32 getFormatID(TypeDescriptor* typeDesc) const;
    void endSchema();

    // API for TypeKey implementors
    // FIXME: Pass StringView instead of const String&
    void writePrimitive(FormatKey formatKey);
    void writeFixedArray(u32 numItems, TypeDescriptor* itemType);
    void writeArray(TypeDescriptor* itemType);
    void writeOwned(TypeDescriptor* itemType);
    void writeRawPtr(TypeDescriptor* itemType);
    void beginStruct(const String& name, u32 numTemplateParams, u32 numMembers);
    void writeTemplateParam(const String& name, TypeDescriptor* typeDesc);
    void writeMember(const String& name, TypeDescriptor* typeDesc);
    void endStruct();
    void beginEnum(const String& name, u32 numEntries, u8 fixedSize);
    void writeEnumEntry(const String& name);
    void endEnum();
    void writeEnumIndexedArray(TypeDescriptor* itemType, TypeDescriptor_Enum* enumType);
    void beginSwitch(const String& name, u32 numStates);
    void writeState(const String& name, TypeDescriptor_Struct* structType);
    void endSwitch();
};

struct SavedPtrResolver {
    struct WeakPointerToResolve {
        AnyObject obj;
        u32 fileOffset = 0;
    };

    struct SavedOwnedPtr {
        AnyObject obj;
        u32 fileOffset = 0;
        s32 linkIndex = -1;
    };

    // In the future, we could generalize this to support pointers to array items and struct members
    // (and not just Owned pointer) by using a BTree instead of a hash map. Not sure it's a great
    // idea though. (If we did that, savedOwnedPtrs would need to generalize to everything that was
    // saved. In that case, not sure it's worth maintaining savedOwnedPtrs at all; could perform a
    // second pass over the object being saved instead.)
    struct PtrMapTraits {
        using Key = void*;
        using Item = u32;
        using Context = Array<SavedOwnedPtr>;
        static PLY_INLINE bool match(Item item, Key key, const Context& ctx) {
            return ctx[item].obj.data == key;
        }
    };

    Array<WeakPointerToResolve> weakPtrsToResolve;
    Array<SavedOwnedPtr> savedOwnedPtrs;
    HashMap<PtrMapTraits> addrToSaveInfo;
};

struct WriteObjectContext {
    NativeEndianWriter out;
    WriteFormatContext* writeFormatContext;
    SavedPtrResolver ptrResolver;
};

//--------------------------------------------------------------------
// Write
//

void writeObject(AnyObject obj, WriteObjectContext* context);
void resolveLinksAndWriteLinkTable(MutStringView view, OutStream& out,
                                   SavedPtrResolver* ptrResolver);

} // namespace ply
