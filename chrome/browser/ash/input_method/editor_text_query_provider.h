// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_INPUT_METHOD_EDITOR_TEXT_QUERY_PROVIDER_H_
#define CHROME_BROWSER_ASH_INPUT_METHOD_EDITOR_TEXT_QUERY_PROVIDER_H_

#include "chrome/browser/manta/orca_provider.h"
#include "chrome/browser/profiles/profile.h"
#include "chromeos/ash/services/orca/public/mojom/orca_service.mojom.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"

namespace ash::input_method {

class EditorTextQueryProvider : public orca::mojom::TextQueryProvider {
 public:
  EditorTextQueryProvider(
      mojo::PendingAssociatedReceiver<orca::mojom::TextQueryProvider> receiver,
      Profile* profile);
  ~EditorTextQueryProvider() override;

  // orca::mojom::TextQueryProvider overrides
  void Process(orca::mojom::TextQueryRequestPtr request,
               ProcessCallback callback) override;

  void OnProfileChanged(Profile* profile);

 private:
  mojo::AssociatedReceiver<orca::mojom::TextQueryProvider>
      text_query_provider_receiver_;
  std::unique_ptr<manta::OrcaProvider> orca_provider_;
};
}  // namespace ash::input_method

#endif  // CHROME_BROWSER_ASH_INPUT_METHOD_EDITOR_TEXT_QUERY_PROVIDER_H_
