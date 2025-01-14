// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/input/single_scrollbar_animation_controller_thinning.h"

#include <algorithm>

#include "base/memory/ptr_util.h"
#include "base/time/time.h"
#include "cc/input/scrollbar_animation_controller.h"
#include "cc/layers/layer_impl.h"
#include "cc/layers/scrollbar_layer_impl_base.h"
#include "cc/trees/layer_tree_impl.h"

namespace cc {

namespace {

float DistanceToScrollbarPart(const gfx::PointF& device_viewport_point,
                              const ScrollbarLayerImplBase& scrollbar,
                              const ScrollbarPart part) {
  gfx::RectF rect;
  if (part == ScrollbarPart::kThumb) {
    rect = gfx::RectF(gfx::Rect(scrollbar.ComputeExpandedThumbQuadRect()));
  } else {
    rect = gfx::RectF(gfx::Rect(scrollbar.bounds()));
  }

  gfx::RectF device_viewport_rect =
      MathUtil::MapClippedRect(scrollbar.ScreenSpaceTransform(), rect);

  return device_viewport_rect.ManhattanDistanceToPoint(device_viewport_point) /
         scrollbar.layer_tree_impl()->device_scale_factor();
}

}  // namespace

std::unique_ptr<SingleScrollbarAnimationControllerThinning>
SingleScrollbarAnimationControllerThinning::Create(
    ElementId scroll_element_id,
    ScrollbarOrientation orientation,
    ScrollbarAnimationControllerClient* client,
    base::TimeDelta thinning_duration) {
  return base::WrapUnique(new SingleScrollbarAnimationControllerThinning(
      scroll_element_id, orientation, client, thinning_duration));
}

SingleScrollbarAnimationControllerThinning::
    SingleScrollbarAnimationControllerThinning(
        ElementId scroll_element_id,
        ScrollbarOrientation orientation,
        ScrollbarAnimationControllerClient* client,
        base::TimeDelta thinning_duration)
    : client_(client),
      is_animating_(false),
      scroll_element_id_(scroll_element_id),
      orientation_(orientation),
      captured_(false),
      mouse_is_over_scrollbar_thumb_(false),
      mouse_is_near_scrollbar_thumb_(false),
      mouse_is_near_scrollbar_track_(false),
      thickness_change_(AnimationChange::kNone),
      thinning_duration_(thinning_duration),
      tickmarks_showing_(false) {
  ApplyThumbThicknessScale(kIdleThicknessScale);
}

ScrollbarLayerImplBase*
SingleScrollbarAnimationControllerThinning::GetScrollbar() const {
  for (ScrollbarLayerImplBase* scrollbar :
       client_->ScrollbarsFor(scroll_element_id_)) {
    DCHECK(scrollbar->is_overlay_scrollbar());

    if (scrollbar->orientation() == orientation_)
      return scrollbar;
  }

  return nullptr;
}

bool SingleScrollbarAnimationControllerThinning::Animate(base::TimeTicks now) {
  if (!is_animating_)
    return false;

  if (last_awaken_time_.is_null())
    last_awaken_time_ = now;

  float progress = AnimationProgressAtTime(now);
  RunAnimationFrame(progress);

  return true;
}

float SingleScrollbarAnimationControllerThinning::AnimationProgressAtTime(
    base::TimeTicks now) {
  // In tests, there may be no duration; snap to the end in such a case.
  if (thinning_duration_.is_zero())
    return 1.0f;

  const base::TimeDelta delta = now - last_awaken_time_;
  return std::clamp(static_cast<float>(delta / thinning_duration_), 0.0f, 1.0f);
}

void SingleScrollbarAnimationControllerThinning::RunAnimationFrame(
    float progress) {
  if (captured_)
    return;

  ApplyThumbThicknessScale(ThumbThicknessScaleAt(progress));

  client_->SetNeedsRedrawForScrollbarAnimation();
  if (progress == 1.f) {
    StopAnimation();
    thickness_change_ = AnimationChange::kNone;
  }
}

void SingleScrollbarAnimationControllerThinning::StartAnimation() {
  is_animating_ = true;
  last_awaken_time_ = base::TimeTicks();
  client_->SetNeedsAnimateForScrollbarAnimation();
}

void SingleScrollbarAnimationControllerThinning::StopAnimation() {
  is_animating_ = false;
}

void SingleScrollbarAnimationControllerThinning::DidScrollUpdate() {
  if (captured_ || !mouse_is_near_scrollbar_track_)
    return;

  CalculateThicknessShouldChange(device_viewport_last_pointer_location_);
}

void SingleScrollbarAnimationControllerThinning::DidMouseDown() {
  if (!mouse_is_over_scrollbar_thumb_)
    return;

  StopAnimation();
  captured_ = true;
  ApplyThumbThicknessScale(1.f);
}

void SingleScrollbarAnimationControllerThinning::DidMouseUp() {
  if (!captured_)
    return;

  captured_ = false;
  StopAnimation();

  // On mouse up, Fluent scrollbars go straight to the scrollbar disappearance
  // animation (via ScrollbarAnimationController) without queueing a thinning
  // animation.
  const bool thickness_should_decrease =
      !client_->IsFluentOverlayScrollbar() && !mouse_is_near_scrollbar_thumb_;

  if (thickness_should_decrease) {
    thickness_change_ = AnimationChange::kDecrease;
    StartAnimation();
  } else {
    thickness_change_ = AnimationChange::kNone;
  }
}

void SingleScrollbarAnimationControllerThinning::DidMouseLeave() {
  mouse_is_over_scrollbar_thumb_ = false;
  mouse_is_near_scrollbar_thumb_ = false;
  mouse_is_near_scrollbar_track_ = false;

  // On mouse leave, Fluent scrollbars go straight to the scrollbar
  // disappearance animation (via ScrollbarAnimationController) without queueing
  // a thinning animation.
  if (client_->IsFluentOverlayScrollbar()) {
    thickness_change_ = AnimationChange::kNone;
    return;
  }

  if (captured_)
    return;

  thickness_change_ = AnimationChange::kDecrease;
  StartAnimation();
}

void SingleScrollbarAnimationControllerThinning::DidMouseMove(
    const gfx::PointF& device_viewport_point) {
  CalculateThicknessShouldChange(device_viewport_point);
  device_viewport_last_pointer_location_ = device_viewport_point;
}

void SingleScrollbarAnimationControllerThinning::CalculateThicknessShouldChange(
    const gfx::PointF& device_viewport_point) {
  ScrollbarLayerImplBase* scrollbar = GetScrollbar();

  if (!scrollbar)
    return;

  const float distance_to_scrollbar_track = DistanceToScrollbarPart(
      device_viewport_point, *scrollbar, ScrollbarPart::kTrackButtonsTickmarks);
  const float distance_to_scrollbar_thumb = DistanceToScrollbarPart(
      device_viewport_point, *scrollbar, ScrollbarPart::kThumb);

  const bool mouse_is_near_scrollbar_track =
      distance_to_scrollbar_track <= MouseMoveDistanceToTriggerFadeIn();

  const bool mouse_is_over_scrollbar_thumb =
      distance_to_scrollbar_thumb == 0.0f;
  const bool mouse_is_near_scrollbar_thumb =
      distance_to_scrollbar_thumb <= MouseMoveDistanceToTriggerExpand();
  bool thickness_should_change;
  if (client_->IsFluentOverlayScrollbar()) {
    const bool is_visible = scrollbar->OverlayScrollbarOpacity() > 0.f;
    const bool moved_over_track =
        mouse_is_near_scrollbar_track_ != mouse_is_near_scrollbar_track;
    const bool mouse_far_from_track =
        (!mouse_is_near_scrollbar_track &&
         thickness_change_ == AnimationChange::kNone);
    // On mouse move Fluent scrollbars will queue a thinning animation iff the
    // scrollbar is visible and either the mouse has moved over the track
    // (increase thickness) or the mouse has moved far away from the track
    // and there is no previously queued animation (decreasse thickness).
    // If tickmarks are shown, the scrollbars should be and should remain in
    // Full mode.
    thickness_should_change = !tickmarks_showing_ && is_visible &&
                              (moved_over_track || mouse_far_from_track);
  } else {
    thickness_should_change =
        (mouse_is_near_scrollbar_thumb_ != mouse_is_near_scrollbar_thumb);
  }

  if (!captured_ && thickness_should_change) {
    const bool thickness_should_increase = client_->IsFluentOverlayScrollbar()
                                               ? mouse_is_near_scrollbar_track
                                               : mouse_is_near_scrollbar_thumb;
    thickness_change_ = thickness_should_increase ? AnimationChange::kIncrease
                                                  : AnimationChange::kDecrease;
    StartAnimation();
  }

  mouse_is_near_scrollbar_track_ = mouse_is_near_scrollbar_track;
  mouse_is_near_scrollbar_thumb_ = mouse_is_near_scrollbar_thumb;
  mouse_is_over_scrollbar_thumb_ = mouse_is_over_scrollbar_thumb;
}

float SingleScrollbarAnimationControllerThinning::CurrentThumbThicknessScale()
    const {
  bool thumb_should_be_expanded;
  if (client_->IsFluentOverlayScrollbar()) {
    thumb_should_be_expanded =
        mouse_is_near_scrollbar_track_ || tickmarks_showing_;
  } else {
    thumb_should_be_expanded = mouse_is_near_scrollbar_thumb_;
  }
  return thumb_should_be_expanded ? 1.f : kIdleThicknessScale;
}

float SingleScrollbarAnimationControllerThinning::ThumbThicknessScaleAt(
    float progress) const {
  if (thickness_change_ == AnimationChange::kNone) {
    return CurrentThumbThicknessScale();
  }
  float factor = thickness_change_ == AnimationChange::kIncrease
                     ? progress
                     : (1.f - progress);
  return ((1.f - kIdleThicknessScale) * factor) + kIdleThicknessScale;
}

float SingleScrollbarAnimationControllerThinning::AdjustScale(
    float new_value,
    float current_value,
    AnimationChange animation_change,
    float min_value,
    float max_value) {
  float result;
  if (animation_change == AnimationChange::kIncrease &&
      current_value > new_value) {
    result = current_value;
  } else if (animation_change == AnimationChange::kDecrease &&
             current_value < new_value) {
    result = current_value;
  } else {
    result = new_value;
  }
  if (result > max_value)
    return max_value;
  if (result < min_value)
    return min_value;
  return result;
}

void SingleScrollbarAnimationControllerThinning::UpdateThumbThicknessScale() {
  StopAnimation();
  ApplyThumbThicknessScale(CurrentThumbThicknessScale());
}

void SingleScrollbarAnimationControllerThinning::ApplyThumbThicknessScale(
    float thumb_thickness_scale) {
  for (auto* scrollbar : client_->ScrollbarsFor(scroll_element_id_)) {
    if (scrollbar->orientation() != orientation_)
      continue;
    DCHECK(scrollbar->is_overlay_scrollbar());

    float scale = AdjustScale(thumb_thickness_scale,
                              scrollbar->thumb_thickness_scale_factor(),
                              thickness_change_, kIdleThicknessScale, 1);

    scrollbar->SetThumbThicknessScaleFactor(scale);
  }
}

void SingleScrollbarAnimationControllerThinning::UpdateTickmarksVisibility(
    bool show) {
  tickmarks_showing_ = show;
}

float SingleScrollbarAnimationControllerThinning::
    MouseMoveDistanceToTriggerExpand() {
  return client_->IsFluentOverlayScrollbar() ? 0.0f : 25.0f;
}

float SingleScrollbarAnimationControllerThinning::
    MouseMoveDistanceToTriggerFadeIn() {
  return client_->IsFluentOverlayScrollbar() ? 0.0f : 30.0f;
}

}  // namespace cc
