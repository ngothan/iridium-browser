// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.browser.test.util;

import org.chromium.base.Promise;
import org.chromium.content.browser.webid.IdentityCredentialsDelegate;
import org.chromium.content.browser.webid.MDocProvider;

/**
 * Used to mock IdentityCredentialsDelegate in tests.
 */
public class MDocProviderUtils {
    // Allows tests to @Mock the IdentityCredentialsDelegate.
    public static class MockIdentityCredentialsDelegate implements IdentityCredentialsDelegate {
        @Override
        public Promise<String> get(String origin, String request) {
            return Promise.rejected();
        }
    }

    public static void setDelegateForTesting(MockIdentityCredentialsDelegate delegate) {
        MDocProvider.setDelegateForTesting(delegate);
    }
}
