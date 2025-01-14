// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/attribution_reporting/stored_source.h"

#include <stdint.h>

#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/time/time.h"
#include "components/attribution_reporting/aggregation_keys.h"
#include "components/attribution_reporting/constants.h"
#include "components/attribution_reporting/destination_set.h"
#include "components/attribution_reporting/event_report_windows.h"
#include "components/attribution_reporting/filters.h"
#include "content/browser/attribution_reporting/common_source_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace content {

// static
bool StoredSource::IsExpiryOrReportWindowTimeValid(
    base::Time expiry_or_report_window_time,
    base::Time source_time) {
  // The source must expire strictly after it occurred.
  return expiry_or_report_window_time > source_time &&
         expiry_or_report_window_time - source_time <=
             attribution_reporting::kMaxSourceExpiry;
}

StoredSource::StoredSource(
    CommonSourceInfo common_info,
    uint64_t source_event_id,
    attribution_reporting::DestinationSet destination_sites,
    base::Time source_time,
    base::Time expiry_time,
    attribution_reporting::EventReportWindows event_report_windows,
    base::Time aggregatable_report_window_time,
    int max_event_level_reports,
    int64_t priority,
    attribution_reporting::FilterData filter_data,
    absl::optional<uint64_t> debug_key,
    attribution_reporting::AggregationKeys aggregation_keys,
    AttributionLogic attribution_logic,
    ActiveState active_state,
    Id source_id,
    int64_t aggregatable_budget_consumed,
    double randomized_response_rate)
    : common_info_(std::move(common_info)),
      source_event_id_(source_event_id),
      destination_sites_(std::move(destination_sites)),
      source_time_(source_time),
      expiry_time_(expiry_time),
      event_report_windows_(std::move(event_report_windows)),
      aggregatable_report_window_time_(aggregatable_report_window_time),
      max_event_level_reports_(max_event_level_reports),
      priority_(priority),
      filter_data_(std::move(filter_data)),
      debug_key_(debug_key),
      aggregation_keys_(std::move(aggregation_keys)),
      attribution_logic_(attribution_logic),
      active_state_(active_state),
      source_id_(source_id),
      aggregatable_budget_consumed_(aggregatable_budget_consumed),
      randomized_response_rate_(randomized_response_rate) {
  DCHECK_GE(aggregatable_budget_consumed_, 0);
  DCHECK_GE(max_event_level_reports_, 0);

  DCHECK_GE(randomized_response_rate_, 0);
  DCHECK_LE(randomized_response_rate_, 1);

  DCHECK(IsExpiryOrReportWindowTimeValid(expiry_time_, source_time_));
  DCHECK(IsExpiryOrReportWindowTimeValid(aggregatable_report_window_time_,
                                         source_time_));
}

StoredSource::~StoredSource() = default;

StoredSource::StoredSource(const StoredSource&) = default;

StoredSource::StoredSource(StoredSource&&) = default;

StoredSource& StoredSource::operator=(const StoredSource&) = default;

StoredSource& StoredSource::operator=(StoredSource&&) = default;

}  // namespace content
