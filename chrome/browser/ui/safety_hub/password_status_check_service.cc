// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/safety_hub/password_status_check_service.h"

#include "base/barrier_closure.h"
#include "base/json/values_util.h"
#include "base/rand_util.h"
#include "base/time/time.h"
#include "chrome/browser/password_manager/account_password_store_factory.h"
#include "chrome/browser/password_manager/affiliation_service_factory.h"
#include "chrome/browser/password_manager/password_store_factory.h"
#include "chrome/browser/ui/safety_hub/safety_hub_constants.h"
#include "chrome/browser/ui/safety_hub/safety_hub_prefs.h"
#include "chrome/common/chrome_features.h"
#include "chrome/grit/branded_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/password_manager/core/browser/ui/credential_ui_entry.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/time_format.h"

namespace {

// Returns true if a new check time should be saved. This is the case when:
// - There is no existing time available, e.g. in first run.
// - The configuration for the interval has changed. This is to ensure changes
//   in the interval are applied without large delays in case the interval is so
//   short that it exceeds backend capacity.
bool ShouldFindNewCheckTime(Profile* profile) {
  // The pref dict looks like this:
  // {
  //   ...
  //   kBackgroundPasswordCheckTimeAndInterval: {
  //     kPasswordCheckIntervalKey: "1728000000000",
  //     kNextPasswordCheckTimeKey: "13333556059805713"
  //   },
  //   ...
  // }
  const base::Value::Dict& check_schedule_dict = profile->GetPrefs()->GetDict(
      safety_hub_prefs::kBackgroundPasswordCheckTimeAndInterval);

  bool uninitialized =
      !check_schedule_dict.Find(safety_hub_prefs::kNextPasswordCheckTimeKey) ||
      !check_schedule_dict.Find(safety_hub_prefs::kPasswordCheckIntervalKey);

  base::TimeDelta update_interval =
      features::kBackgroundPasswordCheckInterval.Get();
  absl::optional<base::TimeDelta> interval_used_for_scheduling =
      base::ValueToTimeDelta(check_schedule_dict.Find(
          safety_hub_prefs::kPasswordCheckIntervalKey));

  return uninitialized || !interval_used_for_scheduling.has_value() ||
         update_interval != interval_used_for_scheduling.value();
}

// Helper functions for displaying passwords in the UI
base::Value::Dict GetCompromisedPasswordCardData(int compromised_count) {
  base::Value::Dict result;

  result.Set(safety_hub::kCardHeaderKey,
             l10n_util::GetPluralStringFUTF16(
                 IDS_PASSWORD_MANAGER_UI_COMPROMISED_PASSWORDS_COUNT,
                 compromised_count));
  result.Set(safety_hub::kCardSubheaderKey,
             l10n_util::GetStringUTF16(
                 IDS_PASSWORD_MANAGER_UI_HAS_COMPROMISED_PASSWORDS));
  result.Set(safety_hub::kCardStateKey,
             static_cast<int>(safety_hub::SafetyHubCardState::kWarning));
  return result;
}

base::Value::Dict GetReusedPasswordCardData(int reused_count, bool signed_in) {
  base::Value::Dict result;

  result.Set(safety_hub::kCardHeaderKey,
             l10n_util::GetPluralStringFUTF16(
                 IDS_PASSWORD_MANAGER_UI_REUSED_PASSWORDS_COUNT, reused_count));
  result.Set(
      safety_hub::kCardSubheaderKey,
      l10n_util::GetStringUTF16(
          signed_in
              ? IDS_PASSWORD_MANAGER_UI_HAS_REUSED_PASSWORDS
              : IDS_SETTINGS_SAFETY_HUB_PASSWORD_CHECK_SUBHEADER_SIGN_IN));
  result.Set(safety_hub::kCardStateKey,
             static_cast<int>(safety_hub::SafetyHubCardState::kWeak));
  return result;
}

base::Value::Dict GetWeakPasswordCardData(int weak_count, bool signed_in) {
  base::Value::Dict result;

  result.Set(safety_hub::kCardHeaderKey,
             l10n_util::GetPluralStringFUTF16(
                 IDS_PASSWORD_MANAGER_UI_WEAK_PASSWORDS_COUNT, weak_count));
  result.Set(
      safety_hub::kCardSubheaderKey,
      l10n_util::GetStringUTF16(
          signed_in
              ? IDS_PASSWORD_MANAGER_UI_HAS_WEAK_PASSWORDS
              : IDS_SETTINGS_SAFETY_HUB_PASSWORD_CHECK_SUBHEADER_SIGN_IN));
  result.Set(safety_hub::kCardStateKey,
             static_cast<int>(safety_hub::SafetyHubCardState::kWeak));
  return result;
}

base::Value::Dict GetSafePasswordCardData(base::Time last_check) {
  base::Value::Dict result;

  result.Set(safety_hub::kCardHeaderKey,
             l10n_util::GetPluralStringFUTF16(
                 IDS_PASSWORD_MANAGER_UI_COMPROMISED_PASSWORDS_COUNT, 0));
  // The subheader string depends on how much time has passed since the last
  // check.
  base::TimeDelta time_delta = base::Time::Now() - last_check;
  if (time_delta < base::Minutes(1)) {
    result.Set(safety_hub::kCardSubheaderKey,
               l10n_util::GetStringUTF16(
                   IDS_SETTINGS_SAFETY_HUB_PASSWORD_CHECK_SUBHEADER_RECENTLY));
  } else {
    std::u16string last_check_string =
        ui::TimeFormat::Simple(ui::TimeFormat::Format::FORMAT_DURATION,
                               ui::TimeFormat::Length::LENGTH_LONG, time_delta);
    result.Set(
        safety_hub::kCardSubheaderKey,
        l10n_util::GetStringFUTF16(
            IDS_SETTINGS_SAFETY_HUB_PASSWORD_CHECK_SUBHEADER_SOME_TIME_AGO,
            last_check_string));
  }
  result.Set(safety_hub::kCardStateKey,
             static_cast<int>(safety_hub::SafetyHubCardState::kSafe));
  return result;
}

base::Value::Dict GetNoWeakOrReusedPasswordCardData(bool signed_in) {
  base::Value::Dict result;
  result.Set(
      safety_hub::kCardHeaderKey,
      l10n_util::GetStringUTF16(
          IDS_SETTINGS_SAFETY_HUB_PASSWORD_CHECK_HEADER_NO_WEAK_OR_REUSED));
  result.Set(
      safety_hub::kCardSubheaderKey,
      l10n_util::GetStringUTF16(
          signed_in
              ? IDS_SETTINGS_SAFETY_HUB_PASSWORD_CHECK_SUBHEADER_GO_TO_PASSWORD_MANAGER
              : IDS_SETTINGS_SAFETY_HUB_PASSWORD_CHECK_SUBHEADER_SIGN_IN));
  result.Set(safety_hub::kCardStateKey,
             static_cast<int>(safety_hub::SafetyHubCardState::kInfo));
  return result;
}

base::Value::Dict GetNoPasswordCardData(bool password_saving_allowed) {
  base::Value::Dict result;

  result.Set(safety_hub::kCardHeaderKey,
             l10n_util::GetStringUTF16(
                 IDS_SETTINGS_SAFETY_HUB_PASSWORD_CHECK_HEADER_NO_PASSWORDS));
  result.Set(
      safety_hub::kCardSubheaderKey,
      l10n_util::GetStringUTF16(
          password_saving_allowed
              ? IDS_SETTINGS_SAFETY_HUB_PASSWORD_CHECK_SUBHEADER_NO_PASSWORDS
              : IDS_SETTINGS_SAFETY_HUB_PASSWORD_CHECK_SUBHEADER_NO_PASSWORDS_POLICY));
  result.Set(safety_hub::kCardStateKey,
             static_cast<int>(safety_hub::SafetyHubCardState::kInfo));
  return result;
}

}  // namespace

PasswordStatusCheckService::PasswordStatusCheckService(Profile* profile)
    : profile_(profile) {
  scoped_refptr<password_manager::PasswordStoreInterface> profile_store =
      PasswordStoreFactory::GetForProfile(profile_,
                                          ServiceAccessType::IMPLICIT_ACCESS);

  scoped_refptr<password_manager::PasswordStoreInterface> account_store =
      AccountPasswordStoreFactory::GetForProfile(
          profile_, ServiceAccessType::IMPLICIT_ACCESS);

  profile_password_store_observation_.Observe(profile_store.get());
  if (account_store) {
    account_password_store_observation_.Observe(account_store.get());
  }

  StartRepeatedUpdates();
  UpdateInsecureCredentialCountAsync();
}

PasswordStatusCheckService::~PasswordStatusCheckService() = default;

void PasswordStatusCheckService::Shutdown() {
  password_check_timer_.Stop();
  saved_passwords_presenter_observation_.Reset();
  bulk_leak_check_observation_.Reset();
  profile_password_store_observation_.Reset();
  account_password_store_observation_.Reset();

  password_check_delegate_.reset();
  saved_passwords_presenter_.reset();
  credential_id_generator_.reset();
}

void PasswordStatusCheckService::StartRepeatedUpdates() {
  if (ShouldFindNewCheckTime(profile_)) {
    base::TimeDelta update_interval =
        features::kBackgroundPasswordCheckInterval.Get();

    base::TimeDelta random_delta = base::Microseconds(
        base::RandGenerator(update_interval.InMicroseconds()));
    base::Time scheduled_check_time = base::Time::Now() + random_delta;

    SetPasswordCheckSchedulePrefsWithInterval(scheduled_check_time);
  }

  // If the scheduled time for the password check is in the future, it should
  // run at that time. If password check is overdue, pick a random time in the
  // next hour.
  base::TimeDelta password_check_run_delta =
      GetScheduledPasswordCheckTime() > base::Time::Now()
          ? GetScheduledPasswordCheckTime() - base::Time::Now()
          : base::Microseconds(base::RandGenerator(
                safety_hub::kPasswordCheckOverdueTimeWindow.InMicroseconds()));

  password_check_timer_.Start(
      FROM_HERE, password_check_run_delta,
      base::BindOnce(&PasswordStatusCheckService::RunPasswordCheckAsync,
                     base::Unretained(this)));
}

void PasswordStatusCheckService::UpdateInsecureCredentialCountAsync() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (is_update_credential_count_pending_) {
    return;
  }

  is_update_credential_count_pending_ = true;

  InitializePasswordCheckInfrastructure();

  CHECK(saved_passwords_presenter_);
  if (!saved_passwords_presenter_observation_.IsObserving()) {
    saved_passwords_presenter_observation_.Observe(
        saved_passwords_presenter_.get());
  }
}

void PasswordStatusCheckService::RunPasswordCheckAsync() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (is_password_check_running_) {
    return;
  }

  is_password_check_running_ = true;

  InitializePasswordCheckInfrastructure();

  CHECK(password_check_delegate_);
  if (!bulk_leak_check_observation_.IsObserving()) {
    bulk_leak_check_observation_.Observe(
        BulkLeakCheckServiceFactory::GetForProfile(profile_));
  }

  password_check_delegate_->StartPasswordCheck();
}

void PasswordStatusCheckService::OnSavedPasswordsChanged(
    const password_manager::PasswordStoreChangeList& changes) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  CHECK(IsInfrastructureReady());

  no_passwords_saved_ =
      saved_passwords_presenter_->GetSavedCredentials().size() == 0;

  base::RepeatingClosure on_done = base::BarrierClosure(
      /*num_closures=*/2,
      base::BindOnce(&PasswordStatusCheckService::OnWeakAndReuseChecksDone,
                     weak_ptr_factory_.GetWeakPtr()));

  // `InsecureCredentialManager` already has information on leaked credentials,
  // check for weak and reused passwords.
  password_check_delegate_->GetInsecureCredentialsManager()->StartReuseCheck(
      on_done);

  password_check_delegate_->GetInsecureCredentialsManager()->StartWeakCheck(
      on_done);
}

void PasswordStatusCheckService::OnWeakAndReuseChecksDone() {
  is_update_credential_count_pending_ = false;
  UpdateInsecureCredentialCount();
  MaybeResetInfrastructureAsync();
}

void PasswordStatusCheckService::OnStateChanged(
    password_manager::BulkLeakCheckService::State state) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  CHECK(IsInfrastructureReady());

  switch (state) {
    case password_manager::BulkLeakCheckServiceInterface::State::kRunning:
      return;
    case password_manager::BulkLeakCheckService::State::kServiceError:
    case password_manager::BulkLeakCheckServiceInterface::State::kIdle:
    case password_manager::BulkLeakCheckServiceInterface::State::kCanceled:
    case password_manager::BulkLeakCheckServiceInterface::State::kSignedOut:
    case password_manager::BulkLeakCheckServiceInterface::State::
        kTokenRequestFailure:
    case password_manager::BulkLeakCheckServiceInterface::State::
        kHashingFailure:
    case password_manager::BulkLeakCheckServiceInterface::State::kNetworkError:
    case password_manager::BulkLeakCheckServiceInterface::State::kQuotaLimit:
      is_password_check_running_ = false;

      // Set time for next password check and schedule the next run.
      base::Time scheduled_check_time = GetScheduledPasswordCheckTime();
      // If current check was scheduled to run a long time ago (larger than the
      // interval) we make sure the next run is in the future with a minimum
      // distance between subsequent checks.
      while (scheduled_check_time <
             base::Time::Now() + safety_hub::kMinTimeBetweenPasswordChecks) {
        scheduled_check_time +=
            features::kBackgroundPasswordCheckInterval.Get();
      }

      SetPasswordCheckSchedulePrefsWithInterval(scheduled_check_time);

      StartRepeatedUpdates();

      MaybeResetInfrastructureAsync();
  }
}

void PasswordStatusCheckService::OnCredentialDone(
    const password_manager::LeakCheckCredential& credential,
    password_manager::IsLeaked is_leaked) {}

void PasswordStatusCheckService::OnLoginsChanged(
    password_manager::PasswordStoreInterface* store,
    const password_manager::PasswordStoreChangeList& changes) {
  for (const auto& change : changes) {
    if (change.type() == password_manager::PasswordStoreChange::ADD ||
        change.type() == password_manager::PasswordStoreChange::REMOVE ||
        change.password_changed() || change.insecure_credentials_changed()) {
      UpdateInsecureCredentialCountAsync();
      return;
    }
  }
}

void PasswordStatusCheckService::OnLoginsRetained(
    password_manager::PasswordStoreInterface* store,
    const std::vector<password_manager::PasswordForm>& retained_passwords) {}

void PasswordStatusCheckService::InitializePasswordCheckInfrastructure() {
  if (IsInfrastructureReady()) {
    return;
  }

  credential_id_generator_ = std::make_unique<extensions::IdGenerator>();
  saved_passwords_presenter_ =
      std::make_unique<password_manager::SavedPasswordsPresenter>(
          AffiliationServiceFactory::GetForProfile(profile_),
          PasswordStoreFactory::GetForProfile(
              profile_, ServiceAccessType::IMPLICIT_ACCESS),
          AccountPasswordStoreFactory::GetForProfile(
              profile_, ServiceAccessType::IMPLICIT_ACCESS));
  saved_passwords_presenter_->Init();
  password_check_delegate_ =
      std::make_unique<extensions::PasswordCheckDelegate>(
          profile_, saved_passwords_presenter_.get(),
          credential_id_generator_.get());
}

void PasswordStatusCheckService::UpdateInsecureCredentialCount() {
  CHECK(IsInfrastructureReady());
  auto insecure_credentials =
      password_check_delegate_->GetInsecureCredentialsManager()
          ->GetInsecureCredentialEntries();

  compromised_credential_count_ = 0;
  weak_credential_count_ = 0;
  reused_credential_count_ = 0;

  for (const auto& entry : insecure_credentials) {
    if (entry.IsMuted()) {
      continue;
    }
    if (password_manager::IsCompromised(entry)) {
      compromised_credential_count_++;
    }
    if (entry.IsWeak()) {
      weak_credential_count_++;
    }
    if (entry.IsReused()) {
      reused_credential_count_++;
    }
  }
}

void PasswordStatusCheckService::MaybeResetInfrastructureAsync() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!is_update_credential_count_pending_ && !is_password_check_running_) {
    saved_passwords_presenter_observation_.Reset();
    bulk_leak_check_observation_.Reset();

    // The reset is done as a task rather than directly because when observers
    // are notified that e.g. the password check is done, it may be too early to
    // reset the infrastructure immediately. Synchronous operations may still be
    // ongoing in `SavedPasswordsPresenter`.
    base::SingleThreadTaskRunner::GetCurrentDefault()->DeleteSoon(
        FROM_HERE, std::move(password_check_delegate_));
    base::SingleThreadTaskRunner::GetCurrentDefault()->DeleteSoon(
        FROM_HERE, std::move(saved_passwords_presenter_));
    base::SingleThreadTaskRunner::GetCurrentDefault()->DeleteSoon(
        FROM_HERE, std::move(credential_id_generator_));
  }
}

bool PasswordStatusCheckService::IsInfrastructureReady() const {
  if (saved_passwords_presenter_ || password_check_delegate_ ||
      credential_id_generator_) {
    // `saved_passwords_presenter_`, `password_check_delegate_`, and
    // `credential_id_generator_` should always be initialized at the same time.
    CHECK(credential_id_generator_);
    CHECK(saved_passwords_presenter_);
    CHECK(password_check_delegate_);
    return true;
  }

  return false;
}

void PasswordStatusCheckService::SetPasswordCheckSchedulePrefsWithInterval(
    base::Time check_time) {
  base::TimeDelta check_interval =
      features::kBackgroundPasswordCheckInterval.Get();

  base::Value::Dict dict;
  dict.Set(safety_hub_prefs::kNextPasswordCheckTimeKey,
           base::TimeToValue(check_time));
  dict.Set(safety_hub_prefs::kPasswordCheckIntervalKey,
           base::TimeDeltaToValue(check_interval));

  profile_->GetPrefs()->SetDict(
      safety_hub_prefs::kBackgroundPasswordCheckTimeAndInterval,
      std::move(dict));
}

base::Time PasswordStatusCheckService::GetScheduledPasswordCheckTime() const {
  const base::Value::Dict& check_schedule_dict = profile_->GetPrefs()->GetDict(
      safety_hub_prefs::kBackgroundPasswordCheckTimeAndInterval);
  absl::optional<base::Time> check_time = base::ValueToTime(
      check_schedule_dict.Find(safety_hub_prefs::kNextPasswordCheckTimeKey));
  CHECK(check_time.has_value());
  return check_time.value();
}

base::TimeDelta PasswordStatusCheckService::GetScheduledPasswordCheckInterval()
    const {
  const base::Value::Dict& check_schedule_dict = profile_->GetPrefs()->GetDict(
      safety_hub_prefs::kBackgroundPasswordCheckTimeAndInterval);
  absl::optional<base::TimeDelta> check_interval = base::ValueToTimeDelta(
      check_schedule_dict.Find(safety_hub_prefs::kPasswordCheckIntervalKey));
  CHECK(check_interval.has_value());
  return check_interval.value();
}

base::Value::Dict PasswordStatusCheckService::GetPasswordCardData(
    bool signed_in) {
  if (no_passwords_saved_) {
    bool password_saving_allowed = profile_->GetPrefs()->GetBoolean(
        password_manager::prefs::kCredentialsEnableService);
    return GetNoPasswordCardData(password_saving_allowed);
  }

  if (compromised_credential_count_ > 0) {
    return GetCompromisedPasswordCardData(compromised_credential_count_);
  }

  if (reused_credential_count_ > 0) {
    return GetReusedPasswordCardData(reused_credential_count_, signed_in);
  }

  if (weak_credential_count_ > 0) {
    return GetWeakPasswordCardData(weak_credential_count_, signed_in);
  }

  CHECK(compromised_credential_count_ == 0);
  CHECK(reused_credential_count_ == 0);
  CHECK(weak_credential_count_ == 0);

  base::Time last_check_completed =
      base::Time::FromTimeT(profile_->GetPrefs()->GetDouble(
          password_manager::prefs::kLastTimePasswordCheckCompleted));
  if (!last_check_completed.is_null() && signed_in) {
    return GetSafePasswordCardData(last_check_completed);
  }

  return GetNoWeakOrReusedPasswordCardData(signed_in);
}
