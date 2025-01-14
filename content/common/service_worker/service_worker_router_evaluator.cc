// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/service_worker/service_worker_router_evaluator.h"

#include <tuple>

#include "base/json/json_writer.h"
#include "base/metrics/histogram_functions.h"
#include "base/notreached.h"
#include "base/strings/stringprintf.h"
#include "services/network/public/cpp/request_destination.h"
#include "services/network/public/cpp/request_mode.h"
#include "third_party/blink/public/common/service_worker/service_worker_router_rule.h"
#include "third_party/liburlpattern/options.h"
#include "third_party/liburlpattern/pattern.h"
#include "third_party/liburlpattern/utils.h"
#include "third_party/re2/src/re2/re2.h"

namespace {

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class ServiceWorkerRouterEvaluatorErrorEnums {
  kNoError = 0,
  kInvalidType = 1,
  kParseError = 2,
  kCompileError = 3,
  kEmptyCondition = 4,
  kEmptySource = 5,
  kInvalidSource = 6,
  kInvalidCondition = 7,

  kMaxValue = kInvalidCondition,
};

void RecordSetupError(ServiceWorkerRouterEvaluatorErrorEnums e) {
  base::UmaHistogramEnumeration("ServiceWorker.RouterEvaluator.Error", e);
}

void RecordMatchedSourceType(
    const std::vector<blink::ServiceWorkerRouterSource>& sources) {
  base::UmaHistogramEnumeration(
      "ServiceWorker.RouterEvaluator.MatchedFirstSourceType", sources[0].type);
}

// TODO(crbug.com/1371756): consolidate code with blink::url_pattern.
//
// The type and method come form
// third_party/blink/renderer/core/url_pattern/url_pattern_component.{h,cc}.
// GetOptions is not exported yet, and there is little benefit to depend
// on the blink::URLPattern now.
enum class URLPatternFieldType {
  kProtocol,
  kUsername,
  kPassword,
  kHostname,
  kPort,
  kPathname,
  kSearch,
  kHash,
};

// Utility method to get the correct liburlpattern parse options for a given
// type.
const std::tuple<liburlpattern::Options, std::string>
GetOptionsAndSegmentWildcardRegex(const blink::SafeUrlPattern& url_pattern,
                                  URLPatternFieldType type) {
  using liburlpattern::Options;

  Options options = {.delimiter_list = "",
                     .prefix_list = "",
                     .sensitive = !url_pattern.options.ignore_case,
                     .strict = true};

  if (type == URLPatternFieldType::kHostname) {
    options.delimiter_list = ".";
  } else if (type == URLPatternFieldType::kPathname) {
    // TODO(crbug.com/1371756): follows the original GetOptions behavior.
    // It sets the following delimiters for some limited protocols.
    options.delimiter_list = "/";
    options.prefix_list = "/";
  }
  std::string segment_wildcard_regex = base::StringPrintf(
      "[^%s]+?",
      liburlpattern::EscapeRegexpString(options.delimiter_list).c_str());

  return std::tie(options, segment_wildcard_regex);
}

liburlpattern::Pattern ConvertToPattern(
    const blink::SafeUrlPattern& url_pattern,
    URLPatternFieldType type) {
  std::vector<liburlpattern::Part> parts;
  switch (type) {
    case URLPatternFieldType::kProtocol:
      parts = url_pattern.protocol;
      break;
    case URLPatternFieldType::kUsername:
      parts = url_pattern.username;
      break;
    case URLPatternFieldType::kPassword:
      parts = url_pattern.password;
      break;
    case URLPatternFieldType::kHostname:
      parts = url_pattern.hostname;
      break;
    case URLPatternFieldType::kPort:
      parts = url_pattern.port;
      break;
    case URLPatternFieldType::kPathname:
      parts = url_pattern.pathname;
      break;
    case URLPatternFieldType::kSearch:
      parts = url_pattern.search;
      break;
    case URLPatternFieldType::kHash:
      parts = url_pattern.hash;
      break;
  }
  auto [options, swr] = GetOptionsAndSegmentWildcardRegex(url_pattern, type);
  return liburlpattern::Pattern(parts, options, swr);
}

std::string ConvertToRegex(const blink::SafeUrlPattern& url_pattern,
                           URLPatternFieldType type) {
  auto pattern = ConvertToPattern(url_pattern, type);
  std::string regex_string = pattern.GenerateRegexString();
  VLOG(3) << "regex string: " << regex_string;
  return regex_string;
}

std::string ConvertToPatternString(const blink::SafeUrlPattern& url_pattern,
                                   URLPatternFieldType type) {
  auto pattern = ConvertToPattern(url_pattern, type);
  return pattern.GeneratePatternString();
}

base::Value RequestToValue(
    const blink::ServiceWorkerRouterRequestCondition& request) {
  base::Value::Dict ret;
  if (request.method) {
    ret.Set("method", *request.method);
  }
  if (request.mode) {
    ret.Set("mode", network::RequestModeToString(*request.mode));
  }
  if (request.destination) {
    ret.Set("destination",
            network::RequestDestinationToString(*request.destination));
  }
  return base::Value(std::move(ret));
}

std::string RunningStatusToString(
    const blink::ServiceWorkerRouterRunningStatusCondition& running_status) {
  switch (running_status.status) {
    case blink::ServiceWorkerRouterRunningStatusCondition::RunningStatusEnum::
        kRunning:
      return "running";
    case blink::ServiceWorkerRouterRunningStatusCondition::RunningStatusEnum::
        kNotRunning:
      return "not-running";
  }
}

bool IsValidCondition(const blink::ServiceWorkerRouterCondition& condition) {
  switch (condition.type) {
    case blink::ServiceWorkerRouterCondition::Type::kUrlPattern:
      return condition.url_pattern.has_value();
    case blink::ServiceWorkerRouterCondition::Type::kRequest:
      return condition.request.has_value() &&
             (condition.request->method.has_value() ||
              condition.request->mode.has_value() ||
              condition.request->destination.has_value());
    case blink::ServiceWorkerRouterCondition::Type::kRunningStatus:
      return condition.running_status.has_value();
    case blink::ServiceWorkerRouterCondition::Type::kOr:
      NOTIMPLEMENTED();
      NOTREACHED_NORETURN();
  }
}

bool IsValidSources(
    const std::vector<blink::ServiceWorkerRouterSource> sources) {
  if (sources.empty()) {
    // At least a source must exist.
    RecordSetupError(ServiceWorkerRouterEvaluatorErrorEnums::kEmptySource);
    return false;
  }
  // TODO(crbug.com/1371756): support other sources in the future.
  // Currently, only network source is supported.
  for (const auto& s : sources) {
    switch (s.type) {
      case blink::ServiceWorkerRouterSource::Type::kNetwork:
        if (!s.network_source) {
          RecordSetupError(
              ServiceWorkerRouterEvaluatorErrorEnums::kInvalidSource);
          return false;
        }
        break;
      case blink::ServiceWorkerRouterSource::Type::kRace:
        if (!s.race_source) {
          RecordSetupError(
              ServiceWorkerRouterEvaluatorErrorEnums::kInvalidSource);
          return false;
        }
        break;
      case blink::ServiceWorkerRouterSource::Type::kFetchEvent:
        if (!s.fetch_event_source) {
          RecordSetupError(
              ServiceWorkerRouterEvaluatorErrorEnums::kInvalidSource);
          return false;
        }
        break;
      case blink::ServiceWorkerRouterSource::Type::kCache:
        if (!s.cache_source) {
          RecordSetupError(
              ServiceWorkerRouterEvaluatorErrorEnums::kInvalidSource);
          return false;
        }
        break;
    }
  }
  return true;
}

bool MatchRequestCondition(
    const blink::ServiceWorkerRouterRequestCondition& pattern,
    const network::ResourceRequest& request) {
  if (pattern.method && *pattern.method != request.method) {
    return false;
  }
  if (pattern.mode && *pattern.mode != request.mode) {
    return false;
  }
  if (pattern.destination && *pattern.destination != request.destination) {
    return false;
  }
  return true;
}

bool MatchRunningCondition(
    const blink::ServiceWorkerRouterRunningStatusCondition& condition,
    const blink::EmbeddedWorkerStatus& running_status) {
  // returns true if both condition matches.
  bool is_condition_running = condition.status ==
                              blink::ServiceWorkerRouterRunningStatusCondition::
                                  RunningStatusEnum::kRunning;
  bool is_status_running =
      running_status == blink::EmbeddedWorkerStatus::kRunning;
  return is_condition_running == is_status_running;
}

}  // namespace

namespace content {

class ServiceWorkerRouterEvaluator::BaseCondition {
 public:
  BaseCondition() = default;
  ~BaseCondition() = default;
  // Returns true on success. Otherwise, false.
  bool Set(const std::vector<blink::ServiceWorkerRouterCondition>& conditions);
  bool Match(const network::ResourceRequest& request,
             absl::optional<blink::EmbeddedWorkerStatus> running_status) const;
  bool need_running_status() const { return need_running_status_; }

 private:
  bool MatchUrlPatternConditions(const network::ResourceRequest& request) const;
  bool MatchNonUrlPatternConditions(
      const network::ResourceRequest& request,
      absl::optional<blink::EmbeddedWorkerStatus> running_status) const;

  absl::optional<RE2> protocol_pattern_;
  absl::optional<RE2> username_pattern_;
  absl::optional<RE2> password_pattern_;
  absl::optional<RE2> hostname_pattern_;
  absl::optional<RE2> port_pattern_;
  absl::optional<RE2> pathname_pattern_;
  absl::optional<RE2> search_pattern_;
  absl::optional<RE2> hash_pattern_;
  bool has_url_pattern_ = false;
  // Non-SafeUrlPattern conditions are processed one by one.
  std::vector<blink::ServiceWorkerRouterCondition> non_url_pattern_conditions_;
  bool need_running_status_ = false;
};

bool ServiceWorkerRouterEvaluator::BaseCondition::Set(
    const std::vector<blink::ServiceWorkerRouterCondition>& conditions) {
  if (conditions.empty()) {
    // At least one condition must be set.
    RecordSetupError(ServiceWorkerRouterEvaluatorErrorEnums::kEmptyCondition);
    return false;
  }
  for (const auto& condition : conditions) {
    if (!IsValidCondition(condition)) {
      RecordSetupError(
          ServiceWorkerRouterEvaluatorErrorEnums::kInvalidCondition);
      return false;
    }
    if (condition.type !=
        blink::ServiceWorkerRouterCondition::Type::kUrlPattern) {
      non_url_pattern_conditions_.push_back(condition);
      if (condition.type ==
          blink::ServiceWorkerRouterCondition::Type::kRunningStatus) {
        need_running_status_ = true;
      }
      continue;
    }

    const blink::SafeUrlPattern& url_pattern = *condition.url_pattern;
#define SET_PATTERN(type_name, type)                                         \
  do {                                                                       \
    auto regex = ConvertToRegex(url_pattern, type);                          \
    type_name##_pattern_.emplace(regex, RE2::Options());                     \
    if (!type_name##_pattern_->ok()) {                                       \
      RecordSetupError(ServiceWorkerRouterEvaluatorErrorEnums::kParseError); \
      return false;                                                          \
    }                                                                        \
  } while (0)
    SET_PATTERN(protocol, URLPatternFieldType::kProtocol);
    SET_PATTERN(username, URLPatternFieldType::kUsername);
    SET_PATTERN(password, URLPatternFieldType::kPassword);
    SET_PATTERN(hostname, URLPatternFieldType::kHostname);
    SET_PATTERN(port, URLPatternFieldType::kPort);
    SET_PATTERN(pathname, URLPatternFieldType::kPathname);
    SET_PATTERN(search, URLPatternFieldType::kSearch);
    SET_PATTERN(hash, URLPatternFieldType::kHash);
#undef SET_PATTERN
    // Counts the conditions to ensure all conditions are matched.
    has_url_pattern_ = true;
    // TODO(crbug.com/1371756): consider fast path on empty parts and "*".
    // Currently, regular expressions are executed even for empty parts cases,
    // which try to match inputs with "^$".  It is also executed for "*".
    // If performance to evaluate regular expressions matter, fast path can
    // be needed.
  }
  return true;
}

bool ServiceWorkerRouterEvaluator::BaseCondition::Match(
    const network::ResourceRequest& request,
    absl::optional<blink::EmbeddedWorkerStatus> running_status) const {
  return MatchUrlPatternConditions(request) &&
         MatchNonUrlPatternConditions(request, running_status);
}

bool ServiceWorkerRouterEvaluator::BaseCondition::MatchUrlPatternConditions(
    const network::ResourceRequest& request) const {
  if (!has_url_pattern_) {  // nothing need to be matched.
    return true;
  }
#define PATTERN_MATCH(type_name, field)                                  \
  CHECK(type_name##_pattern_.has_value());                               \
  if (!RE2::FullMatch(request.url.field(), *type_name##_pattern_)) {     \
    VLOG(3) << "not matched. url=" << request.url << " field=" << #field \
            << " value=" << request.url.field();                         \
    return false;                                                        \
  }
  PATTERN_MATCH(protocol, scheme);
  PATTERN_MATCH(username, username);
  PATTERN_MATCH(password, password);
  PATTERN_MATCH(hostname, host);
  PATTERN_MATCH(port, port);
  PATTERN_MATCH(pathname, path);
  PATTERN_MATCH(search, query);
  PATTERN_MATCH(hash, ref);
#undef PATTERN_MATCH
  return true;
}

bool ServiceWorkerRouterEvaluator::BaseCondition::MatchNonUrlPatternConditions(
    const network::ResourceRequest& request,
    absl::optional<blink::EmbeddedWorkerStatus> running_status) const {
  for (const auto& c : non_url_pattern_conditions_) {
    switch (c.type) {
      case blink::ServiceWorkerRouterCondition::Type::kUrlPattern:
        NOTREACHED_NORETURN()
            << "UrlPattern should be separated in the compile time.";
      case blink::ServiceWorkerRouterCondition::Type::kRequest:
        if (!MatchRequestCondition(*c.request, request)) {
          return false;
        }
        break;
      case blink::ServiceWorkerRouterCondition::Type::kRunningStatus:
        if (running_status &&
            !MatchRunningCondition(*c.running_status, *running_status)) {
          return false;
        }
        break;
      case blink::ServiceWorkerRouterCondition::Type::kOr:
        NOTIMPLEMENTED();
        break;
    }
  }
  return true;
}

class ServiceWorkerRouterEvaluator::RouterRule {
 public:
  bool SetRule(const blink::ServiceWorkerRouterRule& rule) {
    return conditions_.Set(rule.conditions) && SetSources(rule.sources);
  }
  bool Match(const network::ResourceRequest& request,
             absl::optional<blink::EmbeddedWorkerStatus> running_status) const {
    return conditions_.Match(request, running_status);
  }
  const std::vector<blink::ServiceWorkerRouterSource>& sources() const {
    return sources_;
  }
  bool need_running_status() const { return conditions_.need_running_status(); }

 private:
  // Returns true on success. Otherwise, false.
  bool SetSources(
      const std::vector<blink::ServiceWorkerRouterSource>& sources) {
    if (!IsValidSources(sources)) {
      return false;
    }
    sources_ = sources;
    return true;
  }

  BaseCondition conditions_;
  std::vector<blink::ServiceWorkerRouterSource> sources_;
};

ServiceWorkerRouterEvaluator::ServiceWorkerRouterEvaluator(
    blink::ServiceWorkerRouterRules rules)
    : rules_(std::move(rules)) {
  Compile();
}
ServiceWorkerRouterEvaluator::~ServiceWorkerRouterEvaluator() = default;

void ServiceWorkerRouterEvaluator::Compile() {
  for (const auto& r : rules_.rules) {
    std::unique_ptr<RouterRule> rule = std::make_unique<RouterRule>();
    if (!rule->SetRule(r)) {
      return;
    }
    need_running_status_ |= rule->need_running_status();
    compiled_rules_.emplace_back(std::move(rule));
  }
  RecordSetupError(ServiceWorkerRouterEvaluatorErrorEnums::kNoError);
  is_valid_ = true;
}

std::vector<blink::ServiceWorkerRouterSource>
ServiceWorkerRouterEvaluator::EvaluateInternal(
    const network::ResourceRequest& request,
    absl::optional<blink::EmbeddedWorkerStatus> running_status) const {
  CHECK(is_valid_);
  for (const auto& rule : compiled_rules_) {
    if (rule->Match(request, running_status)) {
      VLOG(3) << "matched request url=" << request.url;
      RecordMatchedSourceType(rule->sources());
      return rule->sources();
    }
  }
  VLOG(3) << "not matched request url=" << request.url;
  return std::vector<blink::ServiceWorkerRouterSource>();
}

std::vector<blink::ServiceWorkerRouterSource>
ServiceWorkerRouterEvaluator::Evaluate(
    const network::ResourceRequest& request,
    blink::EmbeddedWorkerStatus running_status) const {
  return EvaluateInternal(request, running_status);
}

std::vector<blink::ServiceWorkerRouterSource>
ServiceWorkerRouterEvaluator::EvaluateWithoutRunningStatus(
    const network::ResourceRequest& request) const {
  CHECK(!need_running_status_);
  return EvaluateInternal(request, absl::nullopt);
}

base::Value ServiceWorkerRouterEvaluator::ToValue() const {
  base::Value::List out;
  for (const auto& r : rules_.rules) {
    base::Value::Dict rule;
    base::Value::List condition;
    base::Value::List source;
    for (const auto& c : r.conditions) {
      switch (c.type) {
        case blink::ServiceWorkerRouterCondition::Type::kUrlPattern: {
          base::Value::Dict out_c;
          base::Value out_value;
          const blink::SafeUrlPattern& url_pattern = *c.url_pattern;
          base::Value::Dict url_pattern_value;
#define TO_VALUE(type, type_name)                           \
  do {                                                      \
    auto value = ConvertToPatternString(url_pattern, type); \
    url_pattern_value.Set(type_name, value);                \
  } while (0)
          TO_VALUE(URLPatternFieldType::kProtocol, "protocol");
          TO_VALUE(URLPatternFieldType::kUsername, "username");
          TO_VALUE(URLPatternFieldType::kPassword, "password");
          TO_VALUE(URLPatternFieldType::kHostname, "hostname");
          TO_VALUE(URLPatternFieldType::kPort, "port");
          TO_VALUE(URLPatternFieldType::kPathname, "pathname");
          TO_VALUE(URLPatternFieldType::kSearch, "search");
          TO_VALUE(URLPatternFieldType::kHash, "hash");
#undef TO_VALUE
          out_c.Set("urlPattern", std::move(url_pattern_value));
          condition.Append(std::move(out_c));
          break;
        }
        case blink::ServiceWorkerRouterCondition::Type::kRequest: {
          base::Value::Dict out_c;
          out_c.Set("request", RequestToValue(*c.request));
          condition.Append(std::move(out_c));
          break;
        }
        case blink::ServiceWorkerRouterCondition::Type::kRunningStatus: {
          base::Value::Dict out_c;
          out_c.Set("running_status", RunningStatusToString(*c.running_status));
          condition.Append(std::move(out_c));
          break;
        }
        case blink::ServiceWorkerRouterCondition::Type::kOr: {
          NOTIMPLEMENTED();
          break;
        }
      }
    }
    for (const auto& s : r.sources) {
      switch (s.type) {
        case blink::ServiceWorkerRouterSource::Type::kNetwork:
          source.Append("network");
          break;
        case blink::ServiceWorkerRouterSource::Type::kRace:
          // TODO(crbug.com/1371756): we may need to update the name per target.
          source.Append("race-network-and-fetch-handler");
          break;
        case blink::ServiceWorkerRouterSource::Type::kFetchEvent:
          source.Append("fetch-event");
          break;
        case blink::ServiceWorkerRouterSource::Type::kCache:
          if (s.cache_source->cache_name) {
            base::Value::Dict out_s;
            out_s.Set("cache_name", *s.cache_source->cache_name);
            source.Append(std::move(out_s));
          } else {
            source.Append("cache");
          }
          break;
      }
    }
    rule.Set("condition", std::move(condition));
    rule.Set("source", std::move(source));
    out.Append(std::move(rule));
  }
  return base::Value(std::move(out));
}

std::string ServiceWorkerRouterEvaluator::ToString() const {
  std::string json;
  base::JSONWriter::Write(ToValue(), &json);
  return json;
}

}  // namespace content
