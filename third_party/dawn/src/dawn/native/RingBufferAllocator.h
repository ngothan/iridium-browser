// Copyright 2018 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SRC_DAWN_NATIVE_RINGBUFFERALLOCATOR_H_
#define SRC_DAWN_NATIVE_RINGBUFFERALLOCATOR_H_

#include <limits>
#include <memory>

#include "dawn/common/SerialQueue.h"
#include "dawn/native/IntegerTypes.h"

// RingBufferAllocator is the front-end implementation used to manage a ring buffer in GPU memory.
namespace dawn::native {

class RingBufferAllocator {
  public:
    RingBufferAllocator();
    explicit RingBufferAllocator(uint64_t maxSize);
    RingBufferAllocator(const RingBufferAllocator&);
    ~RingBufferAllocator();

    RingBufferAllocator& operator=(const RingBufferAllocator&);

    uint64_t Allocate(uint64_t allocationSize,
                      ExecutionSerial serial,
                      uint64_t offsetAlignment = 1);
    void Deallocate(ExecutionSerial lastCompletedSerial);

    uint64_t GetSize() const;
    bool Empty() const;
    uint64_t GetUsedSize() const;

    static constexpr uint64_t kInvalidOffset = std::numeric_limits<uint64_t>::max();

  private:
    struct Request {
        uint64_t endOffset;
        uint64_t size;
    };

    SerialQueue<ExecutionSerial, Request> mInflightRequests;  // Queue of the recorded sub-alloc
                                                              // requests (e.g. frame of resources).

    uint64_t mUsedEndOffset = 0;    // Tail of used sub-alloc requests (in bytes).
    uint64_t mUsedStartOffset = 0;  // Head of used sub-alloc requests (in bytes).
    uint64_t mMaxBlockSize = 0;     // Max size of the ring buffer (in bytes).
    uint64_t mUsedSize = 0;         // Size of the sub-alloc requests (in bytes) of the ring buffer.
};
}  // namespace dawn::native

#endif  // SRC_DAWN_NATIVE_RINGBUFFERALLOCATOR_H_
