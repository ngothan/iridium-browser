/*
 * Copyright (C) 2019 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_MEMORY_MANAGED_PAINT_RECORDER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_MEMORY_MANAGED_PAINT_RECORDER_H_

#include "base/memory/raw_ptr.h"
#include "third_party/blink/renderer/platform/graphics/memory_managed_paint_canvas.h"
#include "third_party/blink/renderer/platform/platform_export.h"

namespace blink {

class PLATFORM_EXPORT MemoryManagedPaintRecorder {
 public:
  class Client : public MemoryManagedPaintCanvas::Client {
   public:
    virtual void InitializeForRecording(cc::PaintCanvas* canvas) const = 0;
  };

  // `client` can't be nullptr and must outlive this object.
  explicit MemoryManagedPaintRecorder(Client* client);
  ~MemoryManagedPaintRecorder();

  cc::PaintCanvas* beginRecording(const gfx::Size& size);
  cc::PaintRecord finishRecordingAsPicture();

  bool HasRecordedDrawOps() const {
    DCHECK(canvas_);
    return canvas_->HasRecordedDrawOps();
  }
  size_t TotalOpCount() const {
    DCHECK(canvas_);
    return canvas_->TotalOpCount();
  }
  size_t OpBytesUsed() const {
    DCHECK(canvas_);
    return canvas_->OpBytesUsed();
  }

  // Only valid while recording.
  cc::PaintCanvas* getRecordingCanvas() const {
    DCHECK(!is_recording_ || canvas_);
    return is_recording_ ? canvas_.get() : nullptr;
  }

 private:
  // Unowned, must not be nullptr.
  raw_ptr<MemoryManagedPaintRecorder::Client, ExperimentalRenderer> client_;
  bool is_recording_ = false;
  gfx::Size size_;
  std::unique_ptr<MemoryManagedPaintCanvas> canvas_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_MEMORY_MANAGED_PAINT_RECORDER_H_
