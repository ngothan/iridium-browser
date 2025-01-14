// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.segmentation_platform;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

@JNINamespace("segmentation_platform")
public class PredictionOptions {
    private final boolean mOnDemandExecution;

    public PredictionOptions(boolean onDemandExecution) {
        mOnDemandExecution = onDemandExecution;
    }

    @CalledByNative
    void fillNativePredictionOptions(long target) {
        PredictionOptionsJni.get().fillNative(target, mOnDemandExecution);
    }

    @NativeMethods
    interface Natives {
        void fillNative(long target, boolean onDemandExecution);
    }
}
