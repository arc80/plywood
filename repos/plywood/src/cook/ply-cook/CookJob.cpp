/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-cook/Core.h>
#include <ply-cook/CookJob.h>
#include <ply-runtime/algorithm/Find.h>
#include <ply-reflect/Asset.h>

namespace ply {

// FIXME: Rename and move to runtime
Tuple<Owned<InStream>, TextFormat>
FileIOWrappers::createStringReaderAutodetect(Owned<InStream>&& ins) {
    if (!ins)
        return {nullptr, TextFormat{}};
    TextFormat textFormat = TextFormat::autodetect(ins);
    return {textFormat.createImporter(std::move(ins)), textFormat};
}

Tuple<String, TextFormat> FileIOWrappers::loadTextAutodetect(Owned<InStream>&& ins) {
    Tuple<Owned<InStream>, TextFormat> tuple = createStringReaderAutodetect(std::move(ins));
    String contents;
    if (tuple.first) {
        contents = tuple.first->readRemainingContents();
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
    double modificationTime = 0;
    // ply reflect off

    Dependency_File() {
        this->type = &DependencyType_File;
    }
};

DependencyType DependencyType_File = {
    // hasChanged
    [](Dependency* dep_, CookResult* result, AnyObject) -> bool { //
        // FIXME: Use a safe cast once reflection supports derived classes
        Dependency_File* depFile = static_cast<Dependency_File*>(dep_);
        FileStatus stat = FileSystem::native()->getFileStatus(depFile->path);
        PLY_ASSERT(stat.result == FSResult::OK);
        if (stat.modificationTime != depFile->modificationTime) {
            SLOG(Cook, "Recooking \"{}\" because \"{}\" changed", result->job->id.str(),
                 depFile->path);
            return true;
        }
        return false;
    },
};

//------------------------------------
// CookResult
//------------------------------------
void CookResult::FileDepScope::onSuccessfulFileOpen() {
    this->depFile->modificationTime = this->modificationTime;
}

CookResult::FileDepScope CookResult::createFileDependency(StringView path) {
    CookResult::FileDepScope fds;
    fds.depFile = new Dependency_File;
    fds.depFile->path = path;
    this->dependencies.append(fds.depFile);

    FileStatus status = FileSystem::native()->getFileStatus(path);
    if (status.result == FSResult::OK) {
        fds.modificationTime = status.modificationTime;
    } else {
        this->errors.append(String::format("error opening {}\n", path));
    }
    return fds;
}

Owned<InStream> CookResult::openFileAsDependency(StringView path) {
    FileDepScope fds = this->createFileDependency(path);
    if (!fds.isValid()) {
        return nullptr;
    }

    Owned<InStream> ins = FileSystem::native()->openStreamForRead(path);
    if (!ins) {
        this->errors.append(String::format("error opening {}\n", path));
        return nullptr;
    }

    // FIXME: What if there is an I/O error reading the file before it's completely read?
    // In that case, we should not mark it successful yet...
    fds.onSuccessfulFileOpen();
    return ins;
}

void CookResult::addReference(const CookJobID& jobID) {
    CookContext* ctx = CookContext::current();
    PLY_ASSERT(ctx->depTracker == DependencyTracker::current());
    Reference<CookJob> refJob = ctx->depTracker->getOrCreateCookJob(jobID);
    PLY_ASSERT(find(this->references, refJob) < 0);
    this->references.append(refJob);
    if (!ctx->checkedJobs.find(refJob).wasFound()) {
        ctx->deferredJobs.insertOrFind(refJob);
    }
}

void CookResult::addError(String&& error) {
    // FIXME: For now, we always dump the error to the console. Make it configurable instead.
    StdErr::text() << error;
    this->errors.append(std::move(error));
}

//------------------------------
// CookJob
//------------------------------
CookJob::~CookJob() {
    DependencyTracker* depTracker = DependencyTracker::current();
    auto iter = depTracker->allCookJobs.findFirstGreaterOrEqualTo(&this->id);
    PLY_ASSERT(iter.isValid() && iter.getItem()->id == this->id);
    depTracker->allCookJobs.remove(iter);
}

//------------------------------
// DependencyTracker
//------------------------------
void DependencyTracker::setRootReferences(Array<Reference<CookJob>>&& rootRefs) {
    Array<Reference<CookJob>> oldRootRefs = std::move(this->rootReferences);
    this->rootReferences = std::move(rootRefs);
}

Reference<CookJob> DependencyTracker::getOrCreateCookJob(const CookJobID& id) {
    auto iter = this->allCookJobs.findFirstGreaterOrEqualTo(&id);
    if (iter.isValid() && iter.getItem()->id == id) {
        return iter.getItem();
    }
    // FIXME: Implement safe cast that recognizes base classes
    //    Reference<CookJob> cookJob = AnyObject::create(id.type->resultType).cast<CookJob>();
    Reference<CookJob> cookJob = new CookJob;
    cookJob->id = id;
    this->allCookJobs.insert(cookJob.get());
    return cookJob;
}

DependencyTracker::~DependencyTracker() {
    PLY_ASSERT(DependencyTracker::current_ != this);
    PLY_SET_IN_SCOPE(current_, this);
    this->rootReferences.clear();
    this->allCookJobs.clear();
    this->userData.destroy();
}

//------------------------------
// CookContext
//------------------------------
CookContext::~CookContext() {
    // deferred jobs should be cooked before
    PLY_ASSERT(CookContext::current_ != this);
    PLY_ASSERT(this->deferredJobs.numItems() == 0);
}

void CookContext::beginCook() {
    PLY_ASSERT(!CookContext::current_);
    PLY_ASSERT(!DependencyTracker::current_);
    PLY_ASSERT(this->depTracker);
    CookContext::current_ = this;
    DependencyTracker::current_ = this->depTracker;
}

void CookContext::endCook() {
    PLY_ASSERT(CookContext::current_ = this);
    PLY_ASSERT(DependencyTracker::current_ = this->depTracker);
    CookContext::current_ = nullptr;
    DependencyTracker::current_ = nullptr;
}

void CookContext::ensureCooked(CookJob* job, AnyObject jobArg) {
    PLY_ASSERT(CookContext::current());

    {
        // Don't keep cursor beyond this scope because this->alreadyChecked can change inside the
        // body of this function.
        auto cursor = this->checkedJobs.insertOrFind(job);
        if (cursor.wasFound()) {
            // If this assert gets hit, there's a dependency loop.
            PLY_ASSERT(cursor->status == CookContext::UpToDate);
            return; // Already cooked
        }
        cursor->status = CookContext::CookInProgress;
    }

    // Check if (re)cook is needed
    bool mustCook = true;
    if (job->result) {
        mustCook = false;
        for (Dependency* dep : job->result->dependencies) {
            if (dep->type->hasChanged(dep, job->result, jobArg)) {
                mustCook = true;
                break;
            }
        }
    }

    // Invoke the cook if needed
    if (mustCook) {
        if (job->result) {
            job->result->unlinkFromDatabase();
        }
        Owned<CookResult> oldResult = std::move(job->result);
        job->result = (CookResult*) AnyObject::create(job->id.type->resultType).ptr;
        job->result->job = job;
        job->id.type->cook(job->result, jobArg);
    } else {
        // Make sure references are checked as deferred cook jobs
        PLY_ASSERT(job->result);
        for (cook::CookJob* deferredJob : job->result->references) {
            this->deferredJobs.insertOrFind(deferredJob);
        }
    }

    // Finally, mark this job as up-to-date
    auto cursor = this->checkedJobs.insertOrFind(job);
    PLY_ASSERT(cursor.wasFound() && cursor->status == CookContext::CookInProgress);
    cursor->status = CookContext::UpToDate;
}

void CookContext::cookDeferred() {
    PLY_ASSERT(CookContext::current());

    while (this->deferredJobs.numItems() > 0) {
        Array<Reference<CookJob>> jobsToCook;
        for (CookJob* cookJob : this->deferredJobs) {
            jobsToCook.append(cookJob);
        }
        this->deferredJobs = {};

        for (CookJob* job : jobsToCook) {
            this->ensureCooked(job);
        }
    }
}

CookResult* CookContext::getAlreadyCookedResult(const CookJobID& id) {
    auto iter = this->depTracker->allCookJobs.findFirstGreaterOrEqualTo(&id);
    PLY_ASSERT(iter.isValid() && iter.getItem()->id == id);
    PLY_ASSERT(iter.getItem()->result);
    return iter.getItem()->result;
}

bool CookContext::isCooked(CookJob* job) {
    auto cursor = this->checkedJobs.insertOrFind(job);
    return cursor.wasFound() && cursor->status == CookContext::UpToDate;
}

} // namespace cook
} // namespace ply

#include "codegen/CookJob.inl" //%%
