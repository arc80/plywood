/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeDescriptor.h>
#include <ply-reflect/FormatDescriptor.h>

namespace ply {

struct FormatDescriptor;

//--------------------------------------------------------------------
// Read
//

struct Schema {
    Array<Owned<FormatDescriptor>> userFormatDescs;

    FormatDescriptor* getFormatDesc(u32 formatID) const;
};

class PersistentTypeResolver {
public:
    virtual TypeDescriptor* getType(FormatDescriptor* formatDesc) = 0;
};

void readSchema(Schema& schema, InStream* in);

#define PLY_VALIDATE_RESOLVED_PTR_TYPES 1

struct LoadPtrResolver {
    struct LinkTableEntry {
        union {
            u32 fileOffset;
            void* ptr;
        };
#if PLY_VALIDATE_RESOLVED_PTR_TYPES
        TypeDescriptor* typeDesc = nullptr;
#endif

        PLY_INLINE LinkTableEntry() : ptr{0} {
        }
    };

    union RawPtr {
        u32 linkIndex;
        void* ptr;
    };

    struct RawPtrToResolve {
        RawPtr* weakPtr = nullptr;
#if PLY_VALIDATE_RESOLVED_PTR_TYPES
        TypeDescriptor* typeDesc = nullptr;
#endif
    };

    Array<LinkTableEntry> linkTable;
    u32 linkTableIndex = 0;
    Array<RawPtrToResolve> weakPtrsToResolve;
    u32 objDataOffset = 0;
};

struct ReadObjectContext {
    const Schema* schema;
    NativeEndianReader in;
    PersistentTypeResolver* typeResolver;
    LoadPtrResolver ptrResolver;

    ReadObjectContext(const Schema* schema, InStream* in, PersistentTypeResolver* typeResolver)
        : schema(schema), in(in), typeResolver(typeResolver) {
    }
};

void readLinkTable(NativeEndianReader* in, LoadPtrResolver* ptrResolver);
AnyObject readObject(ReadObjectContext* context);
void resolveLinks(LoadPtrResolver* ptrResolver);
void skip(ReadObjectContext* context, FormatDescriptor* formatDesc);

} // namespace ply
