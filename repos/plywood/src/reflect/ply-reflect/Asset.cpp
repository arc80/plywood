/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/Asset.h>
#include <ply-reflect/PersistWrite.h>

namespace ply {

//--------------------------------------------------------------------
// Write
//

void writeAsset(OutStream* out, TypedPtr obj) {
    WriteFormatContext writeFormatContext{out};
    MemOutStream memOut;
    WriteObjectContext writeObjectContext{&memOut, &writeFormatContext};
    writeObject(obj, &writeObjectContext);
    writeFormatContext.endSchema();

    // Resolve links and write link table
    Buffer bin = memOut.moveToBuffer();
    resolveLinksAndWriteLinkTable(bin, out, &writeObjectContext.ptrResolver);

    // Write object data
    out->write(bin);
}

//--------------------------------------------------------------------
// Read
//

TypedPtr readAsset(InStream* ins, PersistentTypeResolver* resolver) {
    Schema schema;
    readSchema(schema, ins);
    ReadObjectContext context{&schema, ins, resolver};
    readLinkTable(&context.in, &context.ptrResolver);
    context.ptrResolver.objDataOffset = safeDemote<u32>(ins->getSeekPos());
    TypedPtr obj = readObject(&context);
    resolveLinks(&context.ptrResolver);
    return obj;
}

//--------------------------------------------------------------------
// ExpectedTypeResolver
//

TypeDescriptor* ExpectedTypeResolver::getType(FormatDescriptor* formatDesc) {
    return m_expected;
}

} // namespace ply
