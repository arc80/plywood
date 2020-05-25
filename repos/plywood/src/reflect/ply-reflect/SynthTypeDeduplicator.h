/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeDescriptorOwner.h>

namespace ply {

struct SynthTypeDeduplicator;

extern SynthTypeDeduplicator g_typeDedup;

// fromOwner.m_types[0] is the root type
TypeDescriptorOwner* getUniqueType(SynthTypeDeduplicator* dedup, TypeDescriptorOwner* fromOwner);

} // namespace ply
