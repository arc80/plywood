/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/string/StringView.h>
#include <ply-runtime/container/BigPool.h>
#include <ply-runtime/container/HashMap.h>
#include <ply-runtime/string/details/LabelEncoder.h>
#include <ply-runtime/thread/RaceDetector.h>

namespace ply {

struct Label {
    u32 idx = 0;

    PLY_INLINE Label() {
    }
    explicit PLY_INLINE Label(u32 idx) : idx{idx} {
    }
    PLY_INLINE bool isValid() const {
        return idx != 0;
    }
    PLY_INLINE bool operator==(const Label& other) const {
        return this->idx == other.idx;
    }
};

PLY_INLINE Hasher& operator<<(Hasher& hasher, const Label& label) {
    hasher << label.idx;
    return hasher;
}

struct LabelMap {
private:
    struct Traits {
        using Key = StringView;
        using Item = u32;
        using Context = BigPool<>;
        static bool match(u32 id, const StringView& key, const BigPool<>& bigPool) {
            const char* ptr = bigPool.get(id);
            u32 numBytes = details::LabelEncoder::decodeValue(ptr);
            return StringView{ptr, numBytes} == key;
        }
    };

    PLY_DEFINE_RACE_DETECTOR(raceDetector)
    BigPool<> bigPool;
    HashMap<Traits> strToIndex;

public:
    LabelMap();
    Label insertOrFind(StringView view);
    Label find(StringView view) const;
    StringView view(Label label) const;

    static LabelMap instance;
};

} // namespace ply
