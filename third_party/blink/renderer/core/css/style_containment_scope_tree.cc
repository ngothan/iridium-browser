// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/style_containment_scope_tree.h"

#include "third_party/blink/renderer/core/css/counters_scope_tree.h"
#include "third_party/blink/renderer/core/css/style_containment_scope.h"
#include "third_party/blink/renderer/core/dom/layout_tree_builder_traversal.h"
#include "third_party/blink/renderer/core/dom/node_computed_style.h"
#include "third_party/blink/renderer/core/dom/pseudo_element.h"
#include "third_party/blink/renderer/core/html/html_olist_element.h"
#include "third_party/blink/renderer/core/html/list_item_ordinal.h"
#include "third_party/blink/renderer/core/layout/counter_node.h"
#include "third_party/blink/renderer/core/layout/layout_counter.h"
#include "third_party/blink/renderer/core/layout/layout_quote.h"

namespace blink {

void StyleContainmentScopeTree::Trace(Visitor* visitor) const {
  visitor->Trace(root_scope_);
  visitor->Trace(outermost_quotes_dirty_scope_);
  visitor->Trace(outermost_counters_dirty_scope_);
  visitor->Trace(scopes_);
  visitor->Trace(object_counters_map_);
}

StyleContainmentScope*
StyleContainmentScopeTree::FindOrCreateEnclosingScopeForElement(
    const Element& element) {
  // Traverse the ancestors and see if there is any with contain style.
  // The search is started from the parent of the element as the style
  // containment is scoped to the element’s sub-tree, meaning that the
  // element itself is not the part of its scope subtree.
  for (const Element* it = LayoutTreeBuilderTraversal::ParentElement(element);
       it; it = LayoutTreeBuilderTraversal::ParentElement(*it)) {
    if (!it->GetComputedStyle() || !it->ComputedStyleRef().ContainsStyle()) {
      continue;
    }
    // Create a new scope if the element is not a root to any.
    StyleContainmentScope* scope = CreateScopeForElement(*it);
    return scope;
  }
  // Return root scope if nothing found.
  return root_scope_;
}

void StyleContainmentScopeTree::DestroyScopeForElement(const Element& element) {
  if (auto it = scopes_.find(&element); it != scopes_.end()) {
    // If the element that will be removed is a scope owner,
    // we need to delete this scope and reattach its quotes and children
    // to its parent, and mark its parent dirty.
    StyleContainmentScope* scope = it->value;
    UpdateOutermostQuotesDirtyScope(scope->Parent());
    UpdateOutermostCountersDirtyScope(scope->Parent());
    scope->ReattachToParent();
    scopes_.erase(it);
  }
}

StyleContainmentScope* StyleContainmentScopeTree::CreateScopeForElement(
    const Element& element) {
  auto entry = scopes_.find(&element);
  if (entry != scopes_.end()) {
    return entry->value;
  }
  StyleContainmentScope* scope =
      MakeGarbageCollected<StyleContainmentScope>(&element, this);
  StyleContainmentScope* parent = FindOrCreateEnclosingScopeForElement(element);
  parent->AppendChild(scope);
  scopes_.insert(&element, scope);
  // Try to find if we create a scope anywhere between the parent and existing
  // children. If so, reattach the child and the quotes.
  auto children = parent->Children();
  for (StyleContainmentScope* child : children) {
    if (child != scope &&
        scope->IsAncestorOf(child->GetElement(), parent->GetElement())) {
      parent->RemoveChild(child);
      scope->AppendChild(child);
    }
  }
  parent->ReparentCountersToStyleScope(*scope);
  auto quotes = parent->Quotes();
  for (LayoutQuote* quote : quotes) {
    if (scope->IsAncestorOf(quote->GetOwningPseudo(), parent->GetElement())) {
      parent->DetachQuote(*quote);
      scope->AttachQuote(*quote);
    }
  }
  UpdateOutermostCountersDirtyScope(parent);
  UpdateOutermostQuotesDirtyScope(parent);
  return scope;
}

namespace {

StyleContainmentScope* FindCommonAncestor(StyleContainmentScope* scope1,
                                          StyleContainmentScope* scope2) {
  if (!scope1) {
    return scope2;
  }
  if (!scope2) {
    return scope1;
  }
  HeapVector<StyleContainmentScope*> ancestors1, ancestors2;
  for (StyleContainmentScope* it = scope1; it; it = it->Parent()) {
    if (it == scope2) {
      return scope2;
    }
    ancestors1.emplace_back(it);
  }
  for (StyleContainmentScope* it = scope2; it; it = it->Parent()) {
    if (it == scope1) {
      return scope1;
    }
    ancestors2.emplace_back(it);
  }
  int anc1 = ancestors1.size() - 1;
  int anc2 = ancestors2.size() - 1;
  while (anc1 >= 0 && anc2 >= 0 && ancestors1[anc1] == ancestors2[anc2]) {
    --anc1;
    --anc2;
  }
  int pos = anc1 == int(ancestors1.size()) - 1 ? anc1 : anc1 + 1;
  return ancestors1[pos];
}

}  // namespace

void StyleContainmentScopeTree::UpdateOutermostQuotesDirtyScope(
    StyleContainmentScope* scope) {
  outermost_quotes_dirty_scope_ =
      FindCommonAncestor(scope, outermost_quotes_dirty_scope_);
}

void StyleContainmentScopeTree::UpdateOutermostCountersDirtyScope(
    StyleContainmentScope* scope) {
  outermost_counters_dirty_scope_ =
      FindCommonAncestor(scope, outermost_counters_dirty_scope_);
}

void StyleContainmentScopeTree::UpdateQuotes() {
  if (!outermost_quotes_dirty_scope_) {
    return;
  }
  outermost_quotes_dirty_scope_->UpdateQuotes();
  outermost_quotes_dirty_scope_ = nullptr;
}

void StyleContainmentScopeTree::UpdateCounters() {
  if (!outermost_counters_dirty_scope_) {
    return;
  }
  outermost_counters_dirty_scope_->UpdateCounters();
  outermost_counters_dirty_scope_ = nullptr;
}

void StyleContainmentScopeTree::AddCounterToObjectMap(
    LayoutObject& object,
    const AtomicString& identifier,
    CounterNode& counter) {
  auto it = object_counters_map_.find(&object);
  if (it != object_counters_map_.end()) {
    DCHECK(it->value->find(identifier) == it->value->end());
    it->value->insert(identifier, &counter);
  } else {
    auto* object_map =
        MakeGarbageCollected<HeapHashMap<AtomicString, Member<CounterNode>>>();
    object_map->insert(identifier, &counter);
    object_counters_map_.insert(&object, object_map);
  }
}

CounterNode* StyleContainmentScopeTree::PopCounterFromObjectMap(
    LayoutObject& object,
    const AtomicString& identifier) {
  auto it = object_counters_map_.find(&object);
  if (it == object_counters_map_.end()) {
    return nullptr;
  }
  auto map_it = it->value->find(identifier);
  if (map_it == it->value->end()) {
    return nullptr;
  }
  CounterNode* counter = map_it->value;
  it->value->erase(map_it);
  if (!it->value->size()) {
    object_counters_map_.erase(it);
  }
  return counter;
}

void StyleContainmentScopeTree::RemoveCountersForLayoutObject(
    LayoutObject& object,
    const ComputedStyle& style) {
  for (const auto& [identifier, directives] : *style.GetCounterDirectives()) {
    RemoveCounterForLayoutObject(object, identifier);
  }
}

void StyleContainmentScopeTree::RemoveCounterForLayoutObject(
    LayoutObject& object,
    const AtomicString& identifier) {
  CounterNode* counter = PopCounterFromObjectMap(object, identifier);
  if (counter) {
    StyleContainmentScope* scope = counter->Scope()->StyleScope();
    CountersScopeTree* tree = scope->GetCountersScopeTree();
    Element& root_element = counter->Scope()->RootElement();
    tree->RemoveCounterFromScope(*counter, *counter->Scope());
    if (identifier == "list-item") {
      if (auto* o_list_element = DynamicTo<HTMLOListElement>(root_element)) {
        ListItemOrdinal::InvalidateAllItemsForOrderedList(o_list_element);
      }
    }
    UpdateOutermostCountersDirtyScope(scope->Parent() ? scope->Parent()
                                                      : scope);
  }
}

void StyleContainmentScopeTree::RemoveListItemCounterForLayoutObject(
    LayoutObject& object) {
  CounterNode* counter =
      PopCounterFromObjectMap(object, AtomicString("list-item"));
  if (counter) {
    StyleContainmentScope* scope = counter->Scope()->StyleScope();
    CountersScopeTree* tree =
        counter->Scope()->StyleScope()->GetCountersScopeTree();
    Element& root_element = counter->Scope()->RootElement();
    tree->RemoveCounterFromScope(*counter, *counter->Scope());
    if (auto* o_list_element = DynamicTo<HTMLOListElement>(root_element)) {
      ListItemOrdinal::InvalidateAllItemsForOrderedList(o_list_element);
    }
    UpdateOutermostCountersDirtyScope(scope->Parent() ? scope->Parent()
                                                      : scope);
  }
}

#if DCHECK_IS_ON()
String StyleContainmentScopeTree::ToString(StyleContainmentScope* style_scope,
                                           wtf_size_t depth) const {
  StringBuilder builder;
  if (!style_scope) {
    style_scope = root_scope_;
    builder.AppendFormat("OVERALL SCOPES: %d\n", scopes_.size());
  }
  for (wtf_size_t i = 0; i < depth; ++i) {
    builder.Append(" ");
  }
  if (style_scope->GetElement()) {
    builder.AppendFormat(
        "SCOPE: %s; ", style_scope->GetElement()->DebugName().Ascii().c_str());
    builder.AppendFormat(
        "PARENT: %s",
        style_scope->Parent()->GetElement()
            ? style_scope->Parent()->GetElement()->DebugName().Ascii().c_str()
            : "root");
  } else {
    builder.Append("SCOPE: root");
  }
  builder.Append("\n");
  builder.Append(style_scope->ScopesTreeToString(depth));
  for (wtf_size_t i = 0; i < depth; ++i) {
    builder.Append(" ");
  }
  for (LayoutQuote* quote : style_scope->Quotes()) {
    builder.AppendFormat("QUOTE %p depth %d; ", quote, quote->GetDepth());
  }
  builder.Append("\n");
  for (wtf_size_t i = 0; i < depth; ++i) {
    builder.Append(" ");
  }
  for (StyleContainmentScope* child : style_scope->Children()) {
    builder.AppendFormat("CHILD %s; ",
                         child->GetElement()->DebugName().Ascii().c_str());
  }
  builder.Append("\n");
  for (StyleContainmentScope* child : style_scope->Children()) {
    builder.Append(ToString(child, depth + 1));
    builder.Append("\n");
  }
  return builder.ReleaseString();
}
#endif  // DCHECK_IS_ON()

}  // namespace blink
