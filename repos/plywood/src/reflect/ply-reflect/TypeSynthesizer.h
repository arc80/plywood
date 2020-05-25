/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/PersistRead.h>
#include <ply-reflect/TypeDescriptorOwner.h>
#include <map>

namespace ply {

// Might actually be a built-in FormatDescriptor, like FormatDescriptor_U16 (in the case of
// index buffers):
Reference<TypeDescriptorOwner> synthesizeType(FormatDescriptor* format);

// Currently, we just use a global hook for the app to install custom type synthesizers.
// If needed, we could use a more flexible approach in the future:
struct TypeSynthesizer;
TypeDescriptor* synthesize(TypeSynthesizer* synth, FormatDescriptor* formatDesc);
void append(TypeSynthesizer* synth, TypeDescriptor* typeDesc);
typedef TypeDescriptor* (*SynthFunc)(TypeSynthesizer* synth, FormatDescriptor* formatDesc);
extern std::map<String, SynthFunc> g_TypeSynthRegistry;

} // namespace ply
