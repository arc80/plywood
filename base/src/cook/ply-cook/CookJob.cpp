/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-cook/Core.h>
#include <ply-cook/CookJob.h>
#include <ply-reflect/Asset.h>

namespace ply {

// FIXME: Rename and move to runtime
Tuple<Owned<InStream>, TextFormat>
FileIOWrappers::create_string_reader_autodetect(Owned<InStream>&& ins) {
    if (!ins)
        return {nullptr, TextFormat{}};
    TextFormat text_format = TextFormat::autodetect(ins);
    return {text_format.create_importer(std::move(ins)), text_format};
}

Tuple<String, TextFormat> FileIOWrappers::load_text_autodetect(Owned<InStream>&& ins) {
    Tuple<Owned<InStream>, TextFormat> tuple =
        create_string_reader_autodetect(std::move(ins));
    String contents;
    if (tuple.first) {
        contents = tuple.first->read_remaining_contents();
    }
    return {contents, tuple.second};
}

namespace cook {

SLOG_CHANNEL(Cook, "Cook");

CookContext* CookContext::current_ = nullptr;
DependencyTracker* DependencyTracker::current_ = nullptr;

//------------------------------------
// Dependency_File
//------------------------------------
extern DependencyType DependencyType_File;

struct Dependency_File : Dependency {
    PLY_REFLECT()
    String path;
    double modification_time = 0;
    // ply reflect off

    Dependency_File() {
        this->type = &DependencyType_File;
    }
};

DependencyType DependencyType_File = {
    // has_changed
    [](Dependency* dep_, CookResult* result, AnyObject) -> bool { //
        // FIXME: Use a safe cast once reflection supports derived classes
        Dependency_File* dep_file = static_cast<Dependency_File*>(dep_);
        FileStatus stat = FileSystem.get_file_status(dep_file->path);
        PLY_ASSERT(stat.result == FSResult::OK);
        if (stat.modification_time != dep_file->modification_time) {
            SLOG(Cook, "Recooking \"{}\" because \"{}\" changed", result->job->id.str(),
                 dep_file->path);
            return true;
        }
        return false;
    },
};

//------------------------------------
// CookResult
//------------------------------------
void CookResult::FileDepScope::on_successful_file_open() {
    this->dep_file->modification_time = this->modification_time;
}

CookResult::FileDepScope CookResult::create_file_dependency(StringView path) {
    CookResult::FileDepScope fds;
    fds.dep_file = new Dependency_File;
    fds.dep_file->path = path;
    this->dependencies.append(fds.dep_file);

    FileStatus status = FileSystem.get_file_status(path);
    if (status.result == FSResult::OK) {
        fds.modification_time = status.modification_time;
    } else {
        this->errors.append(String::format("error opening {}\n", path));
    }
    return fds;
}

Owned<InStream> CookResult::open_file_as_dependency(StringView path) {
    FileDepScope fds = this->create_file_dependency(path);
    if (!fds.is_valid()) {
        return nullptr;
    }

    Owned<InStream> ins = FileSystem.open_stream_for_read(path);
    if (!ins) {
        this->errors.append(String::format("error opening {}\n", path));
        return nullptr;
    }

    // FIXME: What if there is an I/O error reading the file before it's completely
    // read? In that case, we should not mark it successful yet...
    fds.on_successful_file_open();
    return ins;
}

void CookResult::add_reference(const CookJobID& job_id) {
    CookContext* ctx = CookContext::current();
    PLY_ASSERT(ctx->dep_tracker == DependencyTracker::current());
    Reference<CookJob> ref_job = ctx->dep_tracker->get_or_create_cook_job(job_id);
    PLY_ASSERT(find(this->references, ref_job) < 0);
    this->references.append(ref_job);
    if (!ctx->checked_jobs.find(ref_job).was_found()) {
        ctx->deferred_jobs.insert_or_find(ref_job);
    }
}

void CookResult::add_error(String&& error) {
    // FIXME: For now, we always dump the error to the console. Make it configurable
    // instead.
    StdErr::text() << error;
    this->errors.append(std::move(error));
}

//------------------------------
// CookJob
//------------------------------
CookJob::~CookJob() {
    DependencyTracker* dep_tracker = DependencyTracker::current();
    auto iter = dep_tracker->all_cook_jobs.find_first_greater_or_equal_to(&this->id);
    PLY_ASSERT(iter.is_valid() && iter.get_item()->id == this->id);
    dep_tracker->all_cook_jobs.remove(iter);
}

//------------------------------
// DependencyTracker
//------------------------------
void DependencyTracker::set_root_references(Array<Reference<CookJob>>&& root_refs) {
    Array<Reference<CookJob>> old_root_refs = std::move(this->root_references);
    this->root_references = std::move(root_refs);
}

Reference<CookJob> DependencyTracker::get_or_create_cook_job(const CookJobID& id) {
    auto iter = this->all_cook_jobs.find_first_greater_or_equal_to(&id);
    if (iter.is_valid() && iter.get_item()->id == id) {
        return iter.get_item();
    }
    // FIXME: Implement safe cast that recognizes base classes
    //    Reference<CookJob> cook_job =
    //    AnyObject::create(id.type->result_type).cast<CookJob>();
    Reference<CookJob> cook_job = new CookJob;
    cook_job->id = id;
    this->all_cook_jobs.insert(cook_job.get());
    return cook_job;
}

DependencyTracker::~DependencyTracker() {
    PLY_ASSERT(DependencyTracker::current_ != this);
    PLY_SET_IN_SCOPE(current_, this);
    this->root_references.clear();
    this->all_cook_jobs.clear();
    this->user_data.destroy();
}

//------------------------------
// CookContext
//------------------------------
CookContext::~CookContext() {
    // deferred jobs should be cooked before
    PLY_ASSERT(CookContext::current_ != this);
    PLY_ASSERT(this->deferred_jobs.num_items() == 0);
}

void CookContext::begin_cook() {
    PLY_ASSERT(!CookContext::current_);
    PLY_ASSERT(!DependencyTracker::current_);
    PLY_ASSERT(this->dep_tracker);
    CookContext::current_ = this;
    DependencyTracker::current_ = this->dep_tracker;
}

void CookContext::end_cook() {
    PLY_ASSERT(CookContext::current_ = this);
    PLY_ASSERT(DependencyTracker::current_ = this->dep_tracker);
    CookContext::current_ = nullptr;
    DependencyTracker::current_ = nullptr;
}

void CookContext::ensure_cooked(CookJob* job, AnyObject job_arg) {
    PLY_ASSERT(CookContext::current());

    {
        // Don't keep cursor beyond this scope because this->already_checked can change
        // inside the body of this function.
        auto cursor = this->checked_jobs.insert_or_find(job);
        if (cursor.was_found()) {
            // If this assert gets hit, there's a dependency loop.
            PLY_ASSERT(cursor->status == CookContext::UpToDate);
            return; // Already cooked
        }
        cursor->status = CookContext::CookInProgress;
    }

    // Check if (re)cook is needed
    bool must_cook = true;
    if (job->result) {
        must_cook = false;
        for (Dependency* dep : job->result->dependencies) {
            if (dep->type->has_changed(dep, job->result, job_arg)) {
                must_cook = true;
                break;
            }
        }
    }

    // Invoke the cook if needed
    if (must_cook) {
        if (job->result) {
            job->result->unlink_from_database();
        }
        Owned<CookResult> old_result = std::move(job->result);
        job->result = (CookResult*) AnyObject::create(job->id.type->result_type).ptr;
        job->result->job = job;
        job->id.type->cook(job->result, job_arg);
    } else {
        // Make sure references are checked as deferred cook jobs
        PLY_ASSERT(job->result);
        for (cook::CookJob* deferred_job : job->result->references) {
            this->deferred_jobs.insert_or_find(deferred_job);
        }
    }

    // Finally, mark this job as up-to-date
    auto cursor = this->checked_jobs.insert_or_find(job);
    PLY_ASSERT(cursor.was_found() && cursor->status == CookContext::CookInProgress);
    cursor->status = CookContext::UpToDate;
}

void CookContext::cook_deferred() {
    PLY_ASSERT(CookContext::current());

    while (this->deferred_jobs.num_items() > 0) {
        Array<Reference<CookJob>> jobs_to_cook;
        for (CookJob* cook_job : this->deferred_jobs) {
            jobs_to_cook.append(cook_job);
        }
        this->deferred_jobs = {};

        for (CookJob* job : jobs_to_cook) {
            this->ensure_cooked(job);
        }
    }
}

CookResult* CookContext::get_already_cooked_result(const CookJobID& id) {
    auto iter = this->dep_tracker->all_cook_jobs.find_first_greater_or_equal_to(&id);
    PLY_ASSERT(iter.is_valid() && iter.get_item()->id == id);
    PLY_ASSERT(iter.get_item()->result);
    return iter.get_item()->result;
}

bool CookContext::is_cooked(CookJob* job) {
    auto cursor = this->checked_jobs.insert_or_find(job);
    return cursor.was_found() && cursor->status == CookContext::UpToDate;
}

} // namespace cook
} // namespace ply

#include "codegen/CookJob.inl" //%%
