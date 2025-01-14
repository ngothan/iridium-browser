// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PARCEL_TRACKING_METRICS_H_
#define IOS_CHROME_BROWSER_PARCEL_TRACKING_METRICS_H_

namespace parcel_tracking {

// Name of the histogram that records the action taken by the user after the
// Parcel Tracking Opt-In prompt is presented.
extern const char kOptInPromptActionHistogramName[];

// Name of the histogram that logs when the Parcel Tracking Opt-In prompt is
// presented.
extern const char kOptInPromptDisplayedHistogramName[];

// Interactions with the Parcel Tracking Opt-In prompt. This is mapped to
// the IOSParcelTrackingOptInActionOnPrompt enum in enums.xml for metrics.
enum class OptInPromptActionType {
  kAskEveryTime = 0,
  kAlwaysTrack = 1,
  kNoThanks = 2,
  kSwipeToDismiss = 3,
  kMaxValue = kSwipeToDismiss,
};

}  // namespace parcel_tracking

#endif  // IOS_CHROME_BROWSER_PARCEL_TRACKING_METRICS_H_
