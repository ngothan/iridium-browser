// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/common/service_worker/service_worker_router_rule_mojom_traits.h"
#include "third_party/blink/public/common/service_worker/service_worker_router_rule.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_router_rule.mojom.h"

namespace mojo {

bool StructTraits<
    blink::mojom::ServiceWorkerRouterRunningStatusConditionDataView,
    blink::ServiceWorkerRouterRunningStatusCondition>::
    Read(blink::mojom::ServiceWorkerRouterRunningStatusConditionDataView data,
         blink::ServiceWorkerRouterRunningStatusCondition* out) {
  if (!data.ReadStatus(&out->status)) {
    return false;
  }
  return true;
}

bool StructTraits<blink::mojom::ServiceWorkerRouterOrConditionDataView,
                  blink::ServiceWorkerRouterOrCondition>::
    Read(blink::mojom::ServiceWorkerRouterOrConditionDataView data,
         blink::ServiceWorkerRouterOrCondition* out) {
  if (!data.ReadConditions(&out->conditions)) {
    return false;
  }
  return true;
}

bool StructTraits<blink::mojom::ServiceWorkerRouterRequestConditionDataView,
                  blink::ServiceWorkerRouterRequestCondition>::
    Read(blink::mojom::ServiceWorkerRouterRequestConditionDataView data,
         blink::ServiceWorkerRouterRequestCondition* out) {
  if (!data.ReadMethod(&out->method)) {
    return false;
  }
  if (data.has_mode()) {
    out->mode = data.mode();
  }
  if (data.has_destination()) {
    out->destination = data.destination();
  }
  return true;
}

blink::mojom::ServiceWorkerRouterConditionDataView::Tag
UnionTraits<blink::mojom::ServiceWorkerRouterConditionDataView,
            blink::ServiceWorkerRouterCondition>::
    GetTag(const blink::ServiceWorkerRouterCondition& data) {
  switch (data.type) {
    case blink::ServiceWorkerRouterCondition::Type::kUrlPattern:
      return blink::mojom::ServiceWorkerRouterCondition::Tag::kUrlPattern;
    case blink::ServiceWorkerRouterCondition::Type::kRequest:
      return blink::mojom::ServiceWorkerRouterCondition::Tag::kRequest;
    case blink::ServiceWorkerRouterCondition::Type::kRunningStatus:
      return blink::mojom::ServiceWorkerRouterCondition::Tag::kRunningStatus;
    case blink::ServiceWorkerRouterCondition::Type::kOr:
      return blink::mojom::ServiceWorkerRouterCondition::Tag::kOrCondition;
  }
}

bool UnionTraits<blink::mojom::ServiceWorkerRouterConditionDataView,
                 blink::ServiceWorkerRouterCondition>::
    Read(blink::mojom::ServiceWorkerRouterConditionDataView data,
         blink::ServiceWorkerRouterCondition* out) {
  switch (data.tag()) {
    case blink::mojom::ServiceWorkerRouterCondition::Tag::kUrlPattern:
      out->type = blink::ServiceWorkerRouterCondition::Type::kUrlPattern;
      if (!data.ReadUrlPattern(&out->url_pattern)) {
        return false;
      }
      return true;
    case blink::mojom::ServiceWorkerRouterCondition::Tag::kRequest:
      out->type = blink::ServiceWorkerRouterCondition::Type::kRequest;
      if (!data.ReadRequest(&out->request)) {
        return false;
      }
      return true;
    case blink::mojom::ServiceWorkerRouterCondition::Tag::kRunningStatus:
      out->type = blink::ServiceWorkerRouterCondition::Type::kRunningStatus;
      if (!data.ReadRunningStatus(&out->running_status)) {
        return false;
      }
      return true;
    case blink::mojom::ServiceWorkerRouterCondition::Tag::kOrCondition:
      out->type = blink::ServiceWorkerRouterCondition::Type::kOr;
      if (!data.ReadOrCondition(&out->or_condition)) {
        return false;
      }
      return true;
  }

  return false;
}

bool StructTraits<blink::mojom::ServiceWorkerRouterCacheSourceDataView,
                  blink::ServiceWorkerRouterCacheSource>::
    Read(blink::mojom::ServiceWorkerRouterCacheSourceDataView data,
         blink::ServiceWorkerRouterCacheSource* out) {
  if (!data.ReadCacheName(&out->cache_name)) {
    return false;
  }
  return true;
}

blink::mojom::ServiceWorkerRouterSourceDataView::Tag
UnionTraits<blink::mojom::ServiceWorkerRouterSourceDataView,
            blink::ServiceWorkerRouterSource>::
    GetTag(const blink::ServiceWorkerRouterSource& data) {
  switch (data.type) {
    case blink::ServiceWorkerRouterSource::Type::kNetwork:
      return blink::mojom::ServiceWorkerRouterSource::Tag::kNetworkSource;
    case blink::ServiceWorkerRouterSource::Type::kRace:
      return blink::mojom::ServiceWorkerRouterSource::Tag::kRaceSource;
    case blink::ServiceWorkerRouterSource::Type::kFetchEvent:
      return blink::mojom::ServiceWorkerRouterSource::Tag::kFetchEventSource;
    case blink::ServiceWorkerRouterSource::Type::kCache:
      return blink::mojom::ServiceWorkerRouterSource::Tag::kCacheSource;
  }
}

bool UnionTraits<blink::mojom::ServiceWorkerRouterSourceDataView,
                 blink::ServiceWorkerRouterSource>::
    Read(blink::mojom::ServiceWorkerRouterSourceDataView data,
         blink::ServiceWorkerRouterSource* out) {
  switch (data.tag()) {
    case blink::mojom::ServiceWorkerRouterSource::Tag::kNetworkSource:
      out->type = blink::ServiceWorkerRouterSource::Type::kNetwork;
      out->network_source.emplace();
      return true;
    case blink::mojom::ServiceWorkerRouterSource::Tag::kRaceSource:
      out->type = blink::ServiceWorkerRouterSource::Type::kRace;
      out->race_source.emplace();
      return true;
    case blink::mojom::ServiceWorkerRouterSource::Tag::kFetchEventSource:
      out->type = blink::ServiceWorkerRouterSource::Type::kFetchEvent;
      out->fetch_event_source.emplace();
      return true;
    case blink::mojom::ServiceWorkerRouterSource::Tag::kCacheSource:
      out->type = blink::ServiceWorkerRouterSource::Type::kCache;
      if (!data.ReadCacheSource(&out->cache_source)) {
        return false;
      }
      return true;
  }
  return false;
}

bool StructTraits<blink::mojom::ServiceWorkerRouterRuleDataView,
                  blink::ServiceWorkerRouterRule>::
    Read(blink::mojom::ServiceWorkerRouterRuleDataView data,
         blink::ServiceWorkerRouterRule* out) {
  if (!data.ReadConditions(&out->conditions)) {
    return false;
  }
  if (!data.ReadSources(&out->sources)) {
    return false;
  }
  return true;
}

bool StructTraits<blink::mojom::ServiceWorkerRouterRulesDataView,
                  blink::ServiceWorkerRouterRules>::
    Read(blink::mojom::ServiceWorkerRouterRulesDataView data,
         blink::ServiceWorkerRouterRules* out) {
  if (!data.ReadRules(&out->rules)) {
    return false;
  }
  return true;
}

}  // namespace mojo
