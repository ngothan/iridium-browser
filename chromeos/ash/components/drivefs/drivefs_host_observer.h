// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_ASH_COMPONENTS_DRIVEFS_DRIVEFS_HOST_OBSERVER_H_
#define CHROMEOS_ASH_COMPONENTS_DRIVEFS_DRIVEFS_HOST_OBSERVER_H_

#include <vector>

#include "base/component_export.h"
#include "base/observer_list_types.h"

namespace drivefs {
namespace mojom {
class DriveError;
class FileChange;
class ProgressEvent;
class SyncingStatus;
}  // namespace mojom

struct SyncState;

class COMPONENT_EXPORT(CHROMEOS_ASH_COMPONENTS_DRIVEFS) DriveFsHostObserver
    : public base::CheckedObserver {
 public:
  ~DriveFsHostObserver() override;
  virtual void OnUnmounted() {}
  virtual void OnSyncingStatusUpdate(const mojom::SyncingStatus& status) {}
  virtual void OnIndividualSyncingStatusesDelta(
      const std::vector<const SyncState>& sync_states) {}
  virtual void OnMirrorSyncingStatusUpdate(const mojom::SyncingStatus& status) {
  }
  virtual void OnFilesChanged(const std::vector<mojom::FileChange>& changes) {}
  virtual void OnError(const mojom::DriveError& error) {}
  virtual void OnItemProgress(const mojom::ProgressEvent& event) {}
};

}  // namespace drivefs

#endif  // CHROMEOS_ASH_COMPONENTS_DRIVEFS_DRIVEFS_HOST_OBSERVER_H_
