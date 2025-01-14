// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/test/integration/typed_urls_helper.h"

#include <stddef.h>

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/synchronization/waitable_event.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/time/time.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/test/integration/sync_datatype_helper.h"
#include "chrome/browser/sync/test/integration/sync_test.h"
#include "components/history/core/browser/history_backend.h"
#include "components/history/core/browser/history_backend_observer.h"
#include "components/history/core/browser/history_database.h"
#include "components/history/core/browser/history_db_task.h"
#include "components/history/core/browser/history_service.h"

using sync_datatype_helper::test;

namespace {

class FlushHistoryDBQueueTask : public history::HistoryDBTask {
 public:
  explicit FlushHistoryDBQueueTask(base::WaitableEvent* event)
      : wait_event_(event) {}
  bool RunOnDBThread(history::HistoryBackend* backend,
                     history::HistoryDatabase* db) override {
    wait_event_->Signal();
    return true;
  }

  void DoneRunOnMainThread() override {}

 private:
  ~FlushHistoryDBQueueTask() override = default;

  const raw_ptr<base::WaitableEvent> wait_event_;
};

class GetUrlTask : public history::HistoryDBTask {
 public:
  GetUrlTask(const GURL& url,
             history::URLRow* row,
             bool* found,
             base::WaitableEvent* event)
      : url_(url), row_(row), wait_event_(event), found_(found) {}

  bool RunOnDBThread(history::HistoryBackend* backend,
                     history::HistoryDatabase* db) override {
    // Fetch the typed URLs.
    *found_ = backend->GetURL(url_, row_);
    wait_event_->Signal();
    return true;
  }

  void DoneRunOnMainThread() override {}

 private:
  ~GetUrlTask() override = default;

  const GURL url_;
  const raw_ptr<history::URLRow> row_;
  const raw_ptr<base::WaitableEvent> wait_event_;
  const raw_ptr<bool> found_;
};

class GetUrlByIdTask : public history::HistoryDBTask {
 public:
  GetUrlByIdTask(history::URLID url_id,
                 history::URLRow* row,
                 bool* found,
                 base::WaitableEvent* event)
      : url_id_(url_id), row_(row), wait_event_(event), found_(found) {}

  bool RunOnDBThread(history::HistoryBackend* backend,
                     history::HistoryDatabase* db) override {
    // Fetch the typed URLs.
    *found_ = backend->GetURLByID(url_id_, row_);
    wait_event_->Signal();
    return true;
  }

  void DoneRunOnMainThread() override {}

 private:
  ~GetUrlByIdTask() override = default;

  const history::URLID url_id_;
  const raw_ptr<history::URLRow> row_;
  const raw_ptr<base::WaitableEvent> wait_event_;
  const raw_ptr<bool> found_;
};

class GetVisitsTask : public history::HistoryDBTask {
 public:
  GetVisitsTask(history::URLID id,
                history::VisitVector* visits,
                base::WaitableEvent* event)
      : id_(id), visits_(visits), wait_event_(event) {}

  bool RunOnDBThread(history::HistoryBackend* backend,
                     history::HistoryDatabase* db) override {
    // Fetch the visits.
    backend->GetVisitsForURL(id_, visits_);
    wait_event_->Signal();
    return true;
  }

  void DoneRunOnMainThread() override {}

 private:
  ~GetVisitsTask() override = default;

  const history::URLID id_;
  const raw_ptr<history::VisitVector> visits_;
  const raw_ptr<base::WaitableEvent> wait_event_;
};

class GetAnnotatedVisitsTask : public history::HistoryDBTask {
 public:
  GetAnnotatedVisitsTask(history::URLID id,
                         std::vector<history::AnnotatedVisit>* visits,
                         base::WaitableEvent* event)
      : id_(id), annotated_visits_(visits), wait_event_(event) {}

  bool RunOnDBThread(history::HistoryBackend* backend,
                     history::HistoryDatabase* db) override {
    // Fetch the visits.
    history::VisitVector basic_visits;
    backend->GetVisitsForURL(id_, &basic_visits);
    *annotated_visits_ = backend->ToAnnotatedVisits(
        basic_visits, /*compute_redirect_chain_start_properties=*/false);
    wait_event_->Signal();
    return true;
  }

  void DoneRunOnMainThread() override {}

 private:
  ~GetAnnotatedVisitsTask() override = default;

  const history::URLID id_;
  const raw_ptr<std::vector<history::AnnotatedVisit>> annotated_visits_;
  const raw_ptr<base::WaitableEvent> wait_event_;
};

class GetRedirectChainTask : public history::HistoryDBTask {
 public:
  GetRedirectChainTask(const history::VisitRow& final_visit,
                       history::VisitVector* visits,
                       base::WaitableEvent* event)
      : final_visit_(final_visit), visits_(visits), wait_event_(event) {}

  bool RunOnDBThread(history::HistoryBackend* backend,
                     history::HistoryDatabase* db) override {
    // Fetch the visits.
    *visits_ = backend->GetRedirectChain(final_visit_);
    wait_event_->Signal();
    return true;
  }

  void DoneRunOnMainThread() override {}

 private:
  ~GetRedirectChainTask() override = default;

  const history::VisitRow final_visit_;
  const raw_ptr<history::VisitVector> visits_;
  const raw_ptr<base::WaitableEvent> wait_event_;
};

// Waits for the history DB thread to finish executing its current set of
// tasks.
void WaitForHistoryDBThread(int index) {
  base::CancelableTaskTracker tracker;
  history::HistoryService* service =
      HistoryServiceFactory::GetForProfileWithoutCreating(
          test()->GetProfile(index));
  base::WaitableEvent wait_event(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  service->ScheduleDBTask(FROM_HERE,
                          std::unique_ptr<history::HistoryDBTask>(
                              new FlushHistoryDBQueueTask(&wait_event)),
                          &tracker);
  wait_event.Wait();
}

// Creates a URLRow in the specified HistoryService with the passed transition
// type.
void AddToHistory(history::HistoryService* service,
                  const GURL& url,
                  ui::PageTransition transition,
                  history::VisitSource source,
                  const base::Time& timestamp) {
  service->AddPage(url, timestamp, /*context_id=*/0,
                   /*nav_entry_id=*/1234,
                   /*referrer=*/GURL(), history::RedirectList(), transition,
                   source, /*did_replace_entry=*/false);
}

bool GetUrlFromHistoryService(history::HistoryService* service,
                              const GURL& url,
                              history::URLRow* row) {
  base::CancelableTaskTracker tracker;
  base::WaitableEvent wait_event(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  bool found = false;
  service->ScheduleDBTask(FROM_HERE,
                          std::unique_ptr<history::HistoryDBTask>(
                              new GetUrlTask(url, row, &found, &wait_event)),
                          &tracker);
  wait_event.Wait();
  return found;
}

bool GetUrlFromHistoryService(history::HistoryService* service,
                              history::URLID url_id,
                              history::URLRow* row) {
  base::CancelableTaskTracker tracker;
  base::WaitableEvent wait_event(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  bool found = false;
  service->ScheduleDBTask(
      FROM_HERE,
      std::unique_ptr<history::HistoryDBTask>(
          new GetUrlByIdTask(url_id, row, &found, &wait_event)),
      &tracker);
  wait_event.Wait();
  return found;
}

history::VisitVector GetVisitsFromHistoryService(
    history::HistoryService* service,
    history::URLID id) {
  base::CancelableTaskTracker tracker;
  base::WaitableEvent wait_event(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  history::VisitVector visits;
  service->ScheduleDBTask(FROM_HERE,
                          std::unique_ptr<history::HistoryDBTask>(
                              new GetVisitsTask(id, &visits, &wait_event)),
                          &tracker);
  wait_event.Wait();
  return visits;
}

std::vector<history::AnnotatedVisit> GetAnnotatedVisitsFromHistoryService(
    history::HistoryService* service,
    history::URLID id) {
  base::CancelableTaskTracker tracker;
  base::WaitableEvent wait_event(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  std::vector<history::AnnotatedVisit> visits;
  service->ScheduleDBTask(
      FROM_HERE,
      std::unique_ptr<history::HistoryDBTask>(
          new GetAnnotatedVisitsTask(id, &visits, &wait_event)),
      &tracker);
  wait_event.Wait();
  return visits;
}

history::VisitVector GetRedirectChainFromHistoryService(
    history::HistoryService* service,
    history::VisitRow final_visit) {
  base::CancelableTaskTracker tracker;
  base::WaitableEvent wait_event(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  history::VisitVector visits;
  service->ScheduleDBTask(
      FROM_HERE,
      std::unique_ptr<history::HistoryDBTask>(
          new GetRedirectChainTask(final_visit, &visits, &wait_event)),
      &tracker);
  wait_event.Wait();
  return visits;
}

history::HistoryService* GetHistoryServiceFromClient(int index) {
  return HistoryServiceFactory::GetForProfileWithoutCreating(
      test()->GetProfile(index));
}

static base::Time* timestamp = nullptr;

base::Time GetUniqueTimestamp() {
  // The history subsystem doesn't like identical timestamps for page visits,
  // and it will massage the visit timestamps if we try to use identical
  // values, which can lead to spurious errors. So make sure all timestamps
  // are unique.
  if (!::timestamp) {
    ::timestamp = new base::Time(base::Time::Now());
  }
  base::Time original = *::timestamp;
  *::timestamp += base::Milliseconds(1);
  return original;
}

}  // namespace

namespace typed_urls_helper {

bool GetUrlFromClient(int index, const GURL& url, history::URLRow* row) {
  history::HistoryService* service = GetHistoryServiceFromClient(index);
  return GetUrlFromHistoryService(service, url, row);
}

bool GetUrlFromClient(int index, history::URLID url_id, history::URLRow* row) {
  history::HistoryService* service = GetHistoryServiceFromClient(index);
  return GetUrlFromHistoryService(service, url_id, row);
}

history::VisitVector GetVisitsFromClient(int index, history::URLID id) {
  history::HistoryService* service = GetHistoryServiceFromClient(index);
  return GetVisitsFromHistoryService(service, id);
}

history::VisitVector GetVisitsForURLFromClient(int index, const GURL& url) {
  history::HistoryService* service = GetHistoryServiceFromClient(index);
  history::URLRow url_row;
  if (!GetUrlFromHistoryService(service, url, &url_row)) {
    return history::VisitVector();
  }
  return GetVisitsFromHistoryService(service, url_row.id());
}

std::vector<history::AnnotatedVisit> GetAnnotatedVisitsFromClient(
    int index,
    history::URLID id) {
  history::HistoryService* service = GetHistoryServiceFromClient(index);
  return GetAnnotatedVisitsFromHistoryService(service, id);
}

std::vector<history::AnnotatedVisit> GetAnnotatedVisitsForURLFromClient(
    int index,
    const GURL& url) {
  history::HistoryService* service = GetHistoryServiceFromClient(index);
  history::URLRow url_row;
  if (!GetUrlFromHistoryService(service, url, &url_row)) {
    return std::vector<history::AnnotatedVisit>();
  }
  return GetAnnotatedVisitsFromHistoryService(service, url_row.id());
}

history::VisitVector GetRedirectChainFromClient(int index,
                                                history::VisitRow final_visit) {
  history::HistoryService* service = GetHistoryServiceFromClient(index);
  return GetRedirectChainFromHistoryService(service, final_visit);
}

void AddUrlToHistory(int index, const GURL& url) {
  AddUrlToHistoryWithTransition(index, url, ui::PAGE_TRANSITION_TYPED,
                                history::SOURCE_BROWSED);
}
void AddUrlToHistoryWithTransition(int index,
                                   const GURL& url,
                                   ui::PageTransition transition,
                                   history::VisitSource source) {
  base::Time timestamp = GetUniqueTimestamp();
  AddUrlToHistoryWithTimestamp(index, url, transition, source, timestamp);
}
void AddUrlToHistoryWithTimestamp(int index,
                                  const GURL& url,
                                  ui::PageTransition transition,
                                  history::VisitSource source,
                                  const base::Time& timestamp) {
  AddToHistory(GetHistoryServiceFromClient(index), url, transition, source,
               timestamp);
  if (test()->UseVerifier()) {
    AddToHistory(HistoryServiceFactory::GetForProfile(
                     test()->verifier(), ServiceAccessType::IMPLICIT_ACCESS),
                 url, transition, source, timestamp);
  }

  // Wait until the AddPage() request has completed so we know the change has
  // filtered down to the sync observers (don't need to wait for the
  // verifier profile since it doesn't sync).
  WaitForHistoryDBThread(index);
}

}  // namespace typed_urls_helper
