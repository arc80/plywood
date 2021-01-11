/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/SynthTypeDeduplicator.h>
#include <ply-reflect/TypeKey.h>

namespace ply {

struct SynthTypeDeduplicator {
    struct HashMapTraits {
        using Key = TypeDescriptorOwner*;
        using Item = Reference<TypeDescriptorOwner>;
        static PLY_INLINE u32 hash(const Key& key) {
            return Hasher::hash(key->getRootType());
        }
        static PLY_INLINE bool match(const Item& a, Key b) {
            return a->getRootType()->isEquivalentTo(b->getRootType());
        }
    };

    HashMap<HashMapTraits> map;
};

SynthTypeDeduplicator g_typeDedup;

TypeDescriptorOwner* getUniqueType(SynthTypeDeduplicator* dedup, TypeDescriptorOwner* fromOwner) {
    auto mapCursor = dedup->map.insertOrFind(fromOwner);
    return *mapCursor;
}

} // namespace ply
