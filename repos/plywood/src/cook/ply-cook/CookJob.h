/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-cook/Core.h>
#include <ply-cook/Hash128.h>
#include <ply-reflect/StaticPtr.h>
#include <ply-runtime/container/BTree.h>

namespace ply {

// FIXME: Rename and move to runtime
struct FileIOWrappers {
    static Tuple<Owned<StringReader>, TextFormat>
    createStringReaderAutodetect(Owned<InStream>&& ins);
    static Tuple<String, TextFormat> loadTextAutodetect(Owned<InStream>&& ins);
};

namespace cook {

struct CookContext;
struct CookJobID;
struct CookJob;
struct CookResult;

struct CookJobType {
    String name;
    TypeDescriptor* resultType = nullptr;
    TypeDescriptor* argType = nullptr;
    void (*cook)(CookResult* result, TypedPtr jobArg) = nullptr;
};

struct CookJobID {
    PLY_REFLECT()
    StaticPtr<CookJobType> type = nullptr;
    String desc;
    // ply reflect off

    PLY_INLINE CookJobID() = default;
    PLY_INLINE CookJobID(CookJobType* type, StringView desc) : type{type}, desc{desc} {
    }
    PLY_INLINE bool operator==(const CookJobID& other) const {
        return (this->type == other.type && this->desc == other.desc);
    }
    // operator< is used by DependencyTracker::allCookJobs to keep CookJobs grouped by type.
    PLY_INLINE bool operator<(const CookJobID& other) const {
        if (this->type.get() != other.type.get()) {
            return this->type.get() < other.type.get();
        }
        return this->desc < other.desc;
    }
    PLY_INLINE String str() const {
        return String::format("{}:{}", this->type->name, this->desc);
    }
    template <typename Hasher>
    PLY_INLINE void appendTo(Hasher& hasher) const {
        hasher.appendPtr(this->type);
        this->desc.appendTo(hasher);
    }
};

struct Dependency;

struct DependencyType {
    bool (*hasChanged)(Dependency* dep, CookResult* job, TypedPtr jobArg) = nullptr;
};

struct Dependency {
    PLY_REFLECT()
    // ply reflect off

    StaticPtr<DependencyType> type = nullptr;
};

struct Dependency_File;

struct CookResult {
    struct FileDepScope {
        double modificationTime = 0;
        Dependency_File* depFile;

        PLY_INLINE bool isValid() const {
            return this->modificationTime != 0;
        }
        void onSuccessfulFileOpen();
    };

    PLY_REFLECT()
    CookJob* job = nullptr;
    Array<Owned<Dependency>> dependencies;
    Array<Reference<CookJob>> references;
    Array<String> errors;
    // ply reflect off

    virtual ~CookResult() {
    }
    virtual void unlinkFromDatabase() {
    }

    Owned<InStream> openFileAsDependency(StringView path);
    FileDepScope createFileDependency(StringView path);
    void addReference(const CookJobID& jobID);
    void addError(String&& error);
};

struct CookJob : RefCounted<CookJob> {
    PLY_REFLECT()
    CookJobID id;
    Owned<CookResult> result;
    // ply reflect off

    ~CookJob();
    PLY_INLINE void onRefCountZero() {
        delete this;
    }
    template <typename T>
    T* castResult() const {
        if (!TypeResolver<T>::get()->isEquivalentTo(this->id.type->resultType))
            return nullptr;
        return static_cast<T*>(this->result.get());
    }
};

struct DependencyTracker {
    struct AllCookJobsTraits {
        using Index = const CookJobID*;
        using Item = CookJob*;
        static constexpr u32 NodeCapacity = 8;
        static Index getIndex(const CookJob* job) {
            return &job->id;
        }
        static bool less(Index a, Index b) {
            return *a < *b;
        }
        static void onItemMoved(Item, void*) {
        }
    };

    PLY_REFLECT()
    Array<Reference<CookJob>> rootReferences;
    // ply reflect off

    // FIXME: Serialize allCookJobs
    BTree<AllCookJobsTraits> allCookJobs;

    // FIXME: Serialize userData
    OwnTypedPtr userData;

    void setRootReferences(Array<Reference<CookJob>>&& rootRefs);
    Reference<CookJob> getOrCreateCookJob(const CookJobID& id);

    static DependencyTracker* current_;
    static PLY_INLINE DependencyTracker* current() {
        PLY_ASSERT(current_);
        return current_;
    }
    ~DependencyTracker();
};

struct CookContext {
    enum Status {
        CookInProgress,
        UpToDate,
    };

    struct CheckedTraits {
        using Key = CookJob*;
        struct Item {
            CookJob* job;
            Status status;
            Item(CookJob* job) : job{job}, status{CookInProgress} {
            }
        };
        static PLY_INLINE Key comparand(const Item& item) {
            return item.job;
        }
        static PLY_INLINE u32 hash(Key key) {
            Hasher h;
            h.appendPtr(key);
            return h.result();
        }
    };

    struct DeferredTraits {
        using Key = CookJob*;
        using Item = CookJob*;
        static PLY_INLINE Key comparand(const Item& item) {
            return item;
        }
        static PLY_INLINE u32 hash(Key key) {
            Hasher h;
            h.appendPtr(key);
            return h.result();
        }
    };

    static CookContext* current_;
    static PLY_INLINE CookContext* current() {
        PLY_ASSERT(current_);
        return current_;
    }

    DependencyTracker* depTracker = nullptr;
    // Alternatively, checkedTypes and checkedJobs *could* just be implemented as a status code in
    // every CookJob/CookJobType...
    HashMap<CheckedTraits> checkedJobs;
    HashMap<DeferredTraits> deferredJobs;

    ~CookContext();
    void beginCook();
    void endCook();
    void ensureCooked(CookJob* job, TypedPtr jobArg = {});
    void cookDeferred();
    CookResult* getAlreadyCookedResult(const CookJobID& id);
    bool isCooked(CookJob* job);

    PLY_INLINE Reference<CookJob> cook(const CookJobID& id, TypedPtr jobArg = {}) {
        PLY_ASSERT(CookContext::current() == this);
        Reference<CookJob> cookJob = this->depTracker->getOrCreateCookJob(id);
        this->ensureCooked(cookJob, jobArg);
        return cookJob;
    }
};

} // namespace cook
} // namespace ply
