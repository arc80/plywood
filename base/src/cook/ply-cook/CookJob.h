/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-cook/Core.h>
#include <ply-runtime/container/Hash128.h>
#include <ply-reflect/StaticPtr.h>
#include <ply-runtime/container/BTree.h>

namespace ply {

// FIXME: Rename and move to runtime
struct FileIOWrappers {
    static Tuple<Owned<InStream>, TextFormat>
    create_string_reader_autodetect(Owned<InStream>&& ins);
    static Tuple<String, TextFormat> load_text_autodetect(Owned<InStream>&& ins);
};

namespace cook {

struct CookContext;
struct CookJobID;
struct CookJob;
struct CookResult;

struct CookJobType {
    String name;
    TypeDescriptor* result_type = nullptr;
    TypeDescriptor* arg_type = nullptr;
    void (*cook)(CookResult* result, AnyObject job_arg) = nullptr;
};

struct CookJobID {
    PLY_REFLECT()
    StaticPtr<CookJobType> type = nullptr;
    String desc;
    // ply reflect off

    CookJobID() = default;
    CookJobID(CookJobType* type, StringView desc) : type{type}, desc{desc} {
    }
    bool operator==(const CookJobID& other) const {
        return (this->type == other.type && this->desc == other.desc);
    }
    // operator< is used by DependencyTracker::all_cook_jobs to keep CookJobs grouped by
    // type.
    bool operator<(const CookJobID& other) const {
        if (this->type.get() != other.type.get()) {
            return this->type.get() < other.type.get();
        }
        return this->desc < other.desc;
    }
    String str() const {
        return String::format("{}:{}", this->type->name, this->desc);
    }
};

struct Dependency;

struct DependencyType {
    bool (*has_changed)(Dependency* dep, CookResult* job, AnyObject job_arg) = nullptr;
};

struct Dependency {
    PLY_REFLECT()
    // ply reflect off

    StaticPtr<DependencyType> type = nullptr;
};

struct Dependency_File;

struct CookResult {
    struct FileDepScope {
        double modification_time = 0;
        Dependency_File* dep_file;

        bool is_valid() const {
            return this->modification_time != 0;
        }
        void on_successful_file_open();
    };

    PLY_REFLECT()
    CookJob* job = nullptr;
    Array<Owned<Dependency>> dependencies;
    Array<Reference<CookJob>> references;
    Array<String> errors;
    // ply reflect off

    virtual ~CookResult() {
    }
    virtual void unlink_from_database() {
    }

    Owned<InStream> open_file_as_dependency(StringView path);
    FileDepScope create_file_dependency(StringView path);
    void add_reference(const CookJobID& job_id);
    void add_error(String&& error);
};

struct CookJob : RefCounted<CookJob> {
    PLY_REFLECT()
    CookJobID id;
    Owned<CookResult> result;
    // ply reflect off

    ~CookJob();
    void on_ref_count_zero() {
        delete this;
    }
    template <typename T>
    T* cast_result() const {
        if (!get_type_descriptor<T>()->is_equivalent_to(this->id.type->result_type))
            return nullptr;
        return static_cast<T*>(this->result.get());
    }
};

struct DependencyTracker {
    struct AllCookJobsTraits {
        using Index = const CookJobID*;
        using Item = CookJob*;
        static constexpr u32 NodeCapacity = 8;
        static Index get_index(const CookJob* job) {
            return &job->id;
        }
        static bool less(Index a, Index b) {
            return *a < *b;
        }
        static void on_item_moved(Item, void*) {
        }
    };

    PLY_REFLECT()
    Array<Reference<CookJob>> root_references;
    // ply reflect off

    // FIXME: Serialize all_cook_jobs
    BTree<AllCookJobsTraits> all_cook_jobs;

    // FIXME: Serialize user_data
    AnyOwnedObject user_data;

    void set_root_references(Array<Reference<CookJob>>&& root_refs);
    Reference<CookJob> get_or_create_cook_job(const CookJobID& id);

    static DependencyTracker* current_;
    static DependencyTracker* current() {
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
        static bool match(const Item& item, Key key) {
            return item.job == key;
        }
    };

    struct DeferredTraits {
        using Key = CookJob*;
        using Item = CookJob*;
        static bool match(Item item, Key key) {
            return item == key;
        }
    };

    static CookContext* current_;
    static CookContext* current() {
        PLY_ASSERT(current_);
        return current_;
    }

    DependencyTracker* dep_tracker = nullptr;
    // Alternatively, checked_types and checked_jobs *could* just be implemented as a
    // status code in every CookJob/CookJobType...
    HashMap<CheckedTraits> checked_jobs;
    HashMap<DeferredTraits> deferred_jobs;

    ~CookContext();
    void begin_cook();
    void end_cook();
    void ensure_cooked(CookJob* job, AnyObject job_arg = {});
    void cook_deferred();
    CookResult* get_already_cooked_result(const CookJobID& id);
    bool is_cooked(CookJob* job);

    Reference<CookJob> cook(const CookJobID& id, AnyObject job_arg = {}) {
        PLY_ASSERT(CookContext::current() == this);
        Reference<CookJob> cook_job = this->dep_tracker->get_or_create_cook_job(id);
        this->ensure_cooked(cook_job, job_arg);
        return cook_job;
    }
};

} // namespace cook
} // namespace ply
