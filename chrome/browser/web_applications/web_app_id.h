// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_ID_H_
#define CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_ID_H_

#include <string>

#include "components/webapps/common/web_app_id.h"

class GURL;

namespace web_app {

// An example AppId id is "fedbieoalmbobgfjapopkghdmhgncnaa", and is derived
// from the web app's ManifestId (see below).
// This id starts with a URL which is then:
// - hashed using SHA256,
// - hashed using SHA256 again,
// - hex encoded into the characters 0-f,
// - transformed to only use alpha characters between a-p (inclusive).
// This algorithm was designed for historical reasons and needs to stay this way
// for backwards compatibility.
using AppId = webapps::AppId;

// This is computed from the manifest's `start_url` and `id` members:
// https://www.w3.org/TR/appmanifest/#id-member. This can be hashed using
// GenerateAppIdFromManifestId in
// chrome/browser/web_applications/web_app_helpers.h to produce an AppId above.
using ManifestId = webapps::ManifestId;

}  // namespace web_app

#endif  // CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_ID_H_
