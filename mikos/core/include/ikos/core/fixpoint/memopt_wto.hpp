#pragma once

#include <memory>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <algorithm>

#include <boost/container/slist.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/pending/disjoint_sets.hpp>

#include <ikos/core/number/bound.hpp>
#include <ikos/core/semantic/dumpable.hpp>
#include <ikos/core/semantic/graph.hpp>

#include <ikos/analyzer/checker/checker.hpp>

namespace ikos {
namespace core {
namespace memory {

template < typename GraphRef, typename GraphTrait >
class Wto;

template < typename GraphRef, typename GraphTrait >
class WtoVertex;

template < typename GraphRef, typename GraphTrait >
class WtoCycle;

template < typename GraphRef, typename GraphTrait >
class WtoComponentVisitor;

/// \brief Helper for sequential containers of unique_ptr
template < typename T >
struct SeqExposeConstRef {
  const T& operator()(const std::unique_ptr< T >& p) const { return *p; }
};

namespace wto_impl {

template <typename GraphRef, typename GraphTrait>
class WtoBuilder;

} // end namespace wto_impl

/// \brief Base class for components of a weak topological order
///
/// This is either a vertex or a cycle.
template < typename GraphRef, typename GraphTrait = GraphTraits< GraphRef > >
class WtoComponent {
public:
  /// \brief Default constructor
  WtoComponent() = default;

  /// \brief Copy constructor
  WtoComponent(const WtoComponent&) noexcept = default;

  /// \brief Move constructor
  WtoComponent(WtoComponent&&) noexcept = default;

  /// \brief Copy assignment operator
  WtoComponent& operator=(const WtoComponent&) noexcept = default;

  /// \brief Move assignment operator
  WtoComponent& operator=(WtoComponent&&) noexcept = default;

  /// \brief Accept the given visitor
  virtual void accept(WtoComponentVisitor< GraphRef, GraphTrait >&) const = 0;

  /// \brief Destructor
  virtual ~WtoComponent() = default;

}; // end class WtoComponent

/// \brief Represents a vertex
template < typename GraphRef, typename GraphTrait = GraphTraits< GraphRef > >
class WtoVertex final : public WtoComponent< GraphRef, GraphTrait > {
public:
  using NodeRef = typename GraphTrait::NodeRef;

private:
  NodeRef _node;

public:
  /// \brief Constructor
  explicit WtoVertex(NodeRef node) : _node(node) {}

  /// \brief Return the graph node
  NodeRef node() const { return this->_node; }

  /// \brief Accept the given visitor
  void accept(WtoComponentVisitor< GraphRef, GraphTrait >& v) const override {
    v.visit(*this);
  }

  /// \brief Dump the vertex, for debugging purpose
  void dump(std::ostream& o) const {
    DumpableTraits< NodeRef >::dump(o, this->_node);
  }

}; // end class WtoVertex

/// \brief Represents a cycle
template < typename GraphRef, typename GraphTrait = GraphTraits< GraphRef > >
class WtoCycle final : public WtoComponent< GraphRef, GraphTrait > {
public:
  using NodeRef = typename GraphTrait::NodeRef;
  using WtoComponentT = WtoComponent< GraphRef, GraphTrait >;

private:
  using WtoComponentPtr = std::unique_ptr< WtoComponentT >;
  // using WtoComponentList = boost::container::slist< WtoComponentPtr >;
  using WtoComponentList = std::vector< WtoComponentPtr >;

public:
  /// \brief Iterator over the components
  using Iterator =
      boost::transform_iterator< SeqExposeConstRef< WtoComponentT >,
                                 typename WtoComponentList::const_iterator >;

private:
  /// \brief Head of the cycle
  NodeRef _head;

  /// \brief List of components
  WtoComponentList _components;

public:
  /// \brief Constructor
  WtoCycle(NodeRef head, WtoComponentList components)
      : _head(head), _components(std::move(components)) {}

  /// \brief Return the head of the cycle
  NodeRef head() const { return this->_head; }

  /// \brief Begin iterator over the components
  Iterator begin() const {
    return boost::make_transform_iterator(this->_components.cbegin(),
                                          SeqExposeConstRef< WtoComponentT >());
  }

  /// \brief End iterator over the components
  Iterator end() const {
    return boost::make_transform_iterator(this->_components.cend(),
                                          SeqExposeConstRef< WtoComponentT >());
  }

  /// \brief Accept the given visitor
  void accept(WtoComponentVisitor< GraphRef, GraphTrait >& v) const override {
    v.visit(*this);
  }

  /// \brief Dump the cycle, for debugging purpose
  void dump(std::ostream& o) const {
    o << "(";
    DumpableTraits< NodeRef >::dump(o, this->_head);
    for (const auto& c : this->_components) {
      o << " ";
      c->dump(o);
    }
    o << ")";
  }

}; // end class WtoCycle

/// \brief Weak topological order visitor
template < typename GraphRef, typename GraphTrait = GraphTraits< GraphRef > >
class WtoComponentVisitor {
public:
  using WtoVertexT = WtoVertex< GraphRef, GraphTrait >;
  using WtoCycleT = WtoCycle< GraphRef, GraphTrait >;

public:
  /// \brief Default constructor
  WtoComponentVisitor() = default;

  /// \brief Copy constructor
  WtoComponentVisitor(const WtoComponentVisitor&) noexcept = default;

  /// \brief Move constructor
  WtoComponentVisitor(WtoComponentVisitor&&) noexcept = default;

  /// \brief Copy assignment operator
  WtoComponentVisitor& operator=(const WtoComponentVisitor&) noexcept = default;

  /// \brief Move assignment operator
  WtoComponentVisitor& operator=(WtoComponentVisitor&&) noexcept = default;

  /// \brief Visit the given vertex
  virtual void visit(const WtoVertexT&) = 0;

  /// \brief Visit the given cycle
  virtual void visit(const WtoCycleT&) = 0;

  /// \brief Destructor
  virtual ~WtoComponentVisitor() = default;

}; // end class WtoComponentVisitor

/// \brief Weak Topological Ordering
template < typename GraphRef, typename GraphTrait = GraphTraits< GraphRef > >
class Wto {
public:
  static_assert(IsGraph< GraphRef, GraphTrait >::value,
                "GraphRef does not implement GraphTraits");

public:
  using NodeRef = typename GraphTrait::NodeRef;
  using WtoComponentT = WtoComponent< GraphRef, GraphTrait >;
  using WtoVertexT = WtoVertex< GraphRef, GraphTrait >;
  using WtoCycleT = WtoCycle< GraphRef, GraphTrait >;

private:
  using WtoComponentPtr = std::unique_ptr< WtoComponentT >;
  // using WtoComponentList = boost::container::slist< WtoComponentPtr >;
  using WtoComponentList = std::vector< WtoComponentPtr >;
  using Dfn = std::size_t;
  using DfnTable = std::unordered_map< NodeRef, Dfn >;
  using NodeRefTable = std::unordered_map< NodeRef, NodeRef >;
  using NodeRefSet = std::unordered_set< NodeRef >;
  using NodeRefSetTable = std::unordered_map< NodeRef, NodeRefSet >;

public:
  /// \brief Iterator over the components
  using Iterator =
      boost::transform_iterator< SeqExposeConstRef< WtoComponentT >,
                                 typename WtoComponentList::const_iterator >;

private:
  // Top-level components.
  WtoComponentList _components;
  // Post-dfn of node.
  DfnTable _post_dfn_table;

  // Node's last user.
  NodeRefTable _last_user_table;
  // Component's predecessors that needs to be freed. Only the last uses.
  NodeRefSetTable _comp_preds_table;

  // All children nodes in LCA that has checks.
  // Excluding the header itself. This is because of `erase_pre`. It should
  // actually delete something, and the optimization in the fixpoint iterator
  // of not caching the pre value of the header violates this.
  NodeRefSetTable _children_with_checks_table;
  // All children nodes in LCA that has successors outside of the component.
  // Including the header.
  NodeRefSetTable _children_with_post_table;
  // All children nodes in LCA that has calls.
  // Including the header.
  NodeRefSetTable _children_with_calls_table;

  // Nodes with checks.
  NodeRefSet _has_check_set;
  // Nodes in a loop.
  NodeRefSet _is_in_loop_set;
  // Heads of outermost components.
  NodeRefSet _is_outermost_component_set;
  const NodeRefSet _empty_set;

  /// \brief Return the post depth-first number of the given node
  Dfn post_dfn(NodeRef n) const {
    auto it = this->_post_dfn_table.find(n);
    if (it != this->_post_dfn_table.end()) {
      return it->second;
    } else {
      return Dfn(0);
    }
  }

public:
  /// \brief Compute the weak topological order of the given graph
  explicit Wto(GraphRef cfg,
               const std::vector< std::unique_ptr< analyzer::Checker > >& checkers) {
    NodeRef root = GraphTrait::entry(cfg);

    if (GraphTrait::successor_begin(root) == GraphTrait::successor_end(root)) {
      this->_components.emplace_back(std::make_unique< WtoVertexT >(root));

      for (ar::Statement* stmt : *root) {
        for (const auto& checker : checkers) {
          if (checker->has_check(stmt)) {
            this->_has_check_set.emplace(root);
            return;
          }
        }
      }
      return;
    }

    wto_impl::WtoBuilder< GraphRef, GraphTrait > builder(cfg, *this, checkers);
  }

  /// \brief No copy constructor
  Wto(const Wto& other) = delete;

  /// \brief Move constructor
  Wto(Wto&& other) = default;

  /// \brief No copy assignment operator
  Wto& operator=(const Wto& other) = delete;

  /// \brief Move assignment operator
  Wto& operator=(Wto&& other) = default;

  /// \brief Destructor
  ~Wto() = default;

  /// \brief Begin iterator over the components
  Iterator begin() const {
    return boost::make_transform_iterator(this->_components.cbegin(),
                                          SeqExposeConstRef< WtoComponentT >());
  }

  /// \brief End iterator over the components
  Iterator end() const {
    return boost::make_transform_iterator(this->_components.cend(),
                                          SeqExposeConstRef< WtoComponentT >());
  }

  const NodeRefSet& get_children_with_checks(NodeRef head) const {
    auto it = this->_children_with_checks_table.find(head);
    if (it != this->_children_with_checks_table.end()) {
      return it->second;
    } else {
      return _empty_set;
    }
  }

  const NodeRefSet& get_children_with_post(NodeRef head) const {
    auto it = this->_children_with_post_table.find(head);
    if (it != this->_children_with_post_table.end()) {
      return it->second;
    } else {
      return _empty_set;
    }
  }

  const NodeRefSet& get_children_with_calls(NodeRef head) const {
    auto it = this->_children_with_calls_table.find(head);
    if (it != this->_children_with_calls_table.end()) {
      return it->second;
    } else {
      return _empty_set;
    }
  }

  const NodeRefSet& get_component_predecessors(NodeRef head) const {
    auto it = this->_comp_preds_table.find(head);
    if (it != this->_comp_preds_table.end()) {
      return it->second;
    } else {
      return _empty_set;
    }
  }

  /// \brief Check whether a predecessor is outside of the component.
  bool is_from_outside(NodeRef head, NodeRef pred) const {
    return post_dfn(head) < post_dfn(pred);
  }

  /// \brief Check whether the head is head of an outermost component.
  bool is_outermost_component(NodeRef head) const {
    return this->_is_outermost_component_set.find(head) != 
      this->_is_outermost_component_set.end();
  }

  /// \brief Return true if the node is in a loop.
  bool is_in_loop(NodeRef node) const {
    return this->_is_in_loop_set.find(node) != this->_is_in_loop_set.end();
  }

  /// \brief Return true if the node has an assertion check in it.
  bool has_check(NodeRef node) const {
    return this->_has_check_set.find(node) != this->_has_check_set.end();
  }

  /// \brief Return the last user of node's POST value.
  NodeRef last_user(NodeRef node) const {
    auto it = this->_last_user_table.find(node);
    if (it != this->_last_user_table.end()) {
      return it->second;
    } else {
      return nullptr;
    }
  }

  /// \brief Accept the given visitor
  void accept(WtoComponentVisitor< GraphRef, GraphTrait >& v) {
    for (const auto& c : this->_components) {
      c->accept(v);
    }
  }

  /// \brief Dump the order, for debugging purpose
  void dump(std::ostream& o) const {
    for (auto it = this->begin(), et = this->end(); it != et;) {
      it->dump(o);
      ++it;
      if (it != et) {
        o << " ";
      }
    }
  }

  void clear() {
    _components.clear();
    _post_dfn_table.clear();
    _last_user_table.clear();
    _comp_preds_table.clear();

    _children_with_checks_table.clear();
    _children_with_post_table.clear();
    _children_with_calls_table.clear();
    _has_check_set.clear();
    _is_in_loop_set.clear();
    _is_outermost_component_set.clear();
  }

  template <typename T1, typename T2>
  friend class ikos::core::memory::wto_impl::WtoBuilder;
}; // end class Wto

namespace wto_impl {

template <typename GraphRef, typename GraphTrait = GraphTraits<GraphRef>>
class WtoBuilder final {
 private:
  using WtoComponentT = WtoComponent< GraphRef, GraphTrait >;
  using WtoVertexT = WtoVertex< GraphRef, GraphTrait >;
  using WtoCycleT = WtoCycle< GraphRef, GraphTrait >;
  using WtoComponentPtr = std::unique_ptr< WtoComponentT >;
  // using WtoComponentList = boost::container::slist< WtoComponentPtr >;
  using WtoComponentList = std::vector< WtoComponentPtr >;
  using NodeRef = typename GraphTrait::NodeRef;
  using WtoT = Wto<GraphRef, GraphTrait>;
  using Dfn = std::size_t;
  using DfnTable = std::unordered_map< NodeRef, Dfn >;
  using DfnInverseTable = std::unordered_map< Dfn, NodeRef >;
  using SuccTable = std::unordered_map< NodeRef, std::pair< NodeRef, bool > >; // <succ, is_head>

 public:
  WtoBuilder(const GraphRef& cfg,
             WtoT& wto,
             const std::vector<std::unique_ptr<analyzer::Checker>>& checkers)
      : _wto(wto), _post_dfn(wto._post_dfn_table),
        _next_dfn(1), _next_post_dfn(1), _checkers(checkers) {
    construct_auxilary(cfg);
    construct_wto();
    construct_meta_info();
  }

 private:
  /// \brief Construct auxilary data-structures.
  /// Performs DFS iteratively to classify the edges
  /// and to find lowest common ancestors of cross/forward edges.
  /// Nodes are identified by their DFNs in the main construction algorithm.
  void construct_auxilary(const GraphRef& cfg) {
    using RankMap = std::unordered_map<NodeRef, std::size_t>;
    using ParentMap = std::unordered_map<NodeRef, NodeRef>;
    using RankPropertyMap = boost::associative_property_map<RankMap>;
    using ParentPropertyMap = boost::associative_property_map<ParentMap>;

    RankMap rank_map;
    ParentMap parent_map;
    RankPropertyMap r_pmap(rank_map);
    ParentPropertyMap p_pmap(parent_map);
    boost::disjoint_sets<RankPropertyMap, ParentPropertyMap> dsets(r_pmap, p_pmap);
    std::unordered_map<NodeRef, NodeRef> ancestor;

    std::stack<std::tuple<NodeRef, bool, NodeRef>> stack;
    std::unordered_set<NodeRef> finished;

    stack.push(std::make_tuple(GraphTrait::entry(cfg), false, nullptr));
    while (!stack.empty()) {
      // Iterative DFS.
      auto& stack_top = stack.top();
      auto node_ref = std::get<0>(stack_top);
      auto search_complete = std::get<1>(stack_top);
      auto pred_ref = std::get<2>(stack_top);
      stack.pop();

      if (search_complete) {
        // DFS is done with this node.
        // Post DFN.
        set_post_dfn(node_ref, _next_post_dfn++);
        finished.emplace(node_ref);

        if (pred_ref != nullptr) {
          dsets.union_set(node_ref, pred_ref);
          ancestor[dsets.find_set(pred_ref)] = pred_ref;
        }
      } else {
        if (get_dfn(node_ref) != Dfn(0) /* means node is already discovered. */) {
          if (pred_ref != nullptr) {
            // A forward edge.
            // Forward edges can be ignored, as they are redundant.
          }
          continue;
        }
        // New node is discovered.
        auto node = _next_dfn++;
        set_dfn(node_ref, node);
        dsets.make_set(node_ref);
        ancestor[node_ref] = node_ref;

        // This will be popped after all its successors are finished.
        stack.push(std::make_tuple(node_ref, true, pred_ref));

        for (auto it = GraphTrait::successor_end(node_ref),
                  et = GraphTrait::successor_begin(node_ref);
             it != et;) {
          --it;
          NodeRef succ_ref = *it;
          auto succ = get_dfn(succ_ref);
          if (succ == 0 /* 0 means node is undiscovered. */) {
            // Newly discovered node. Search continues.
            stack.push(std::make_tuple(succ_ref, false, node_ref));
          } else if (finished.find(succ_ref) != finished.end()) {
            // A cross edge.
            auto lca = ancestor[dsets.find_set(succ_ref)];
            _cross_fwds[lca].emplace_back(node_ref, succ_ref);
          } else {
            // A back edge.
            _back_preds[succ_ref].push_back(node_ref);
          }
        }
        if (pred_ref != nullptr) {
          // A tree edge.
          _non_back_preds[node_ref].push_back(pred_ref);
        }
      }
    }
  }

  void construct_wto() {
    using RankMap = std::unordered_map<NodeRef, std::size_t>;
    using ParentMap = std::unordered_map<NodeRef, NodeRef>;
    using RankPropertyMap = boost::associative_property_map<RankMap>;
    using ParentPropertyMap = boost::associative_property_map<ParentMap>;

    RankMap rank(get_next_dfn());
    ParentMap parent(get_next_dfn());
    // Partition of vertices. Each subset is known to be strongly connected.
    boost::disjoint_sets<RankPropertyMap, ParentPropertyMap> dsets(rank, parent);
    // Maps representative of a set to the vertex with minimum DFN.
    std::unordered_map<NodeRef, NodeRef> rep(get_next_dfn());

    // Initialization.
    for (auto vn = Dfn(1); vn < get_next_dfn(); vn++) {
      auto v = get_ref(vn);
      dsets.make_set(v);
      rep[v] = v;
    }

    // In reverse DFN, build sub-WTOs for SCCs bottom-up.
    for (auto hn = get_next_dfn()-1; hn > 0; hn--) {
      /*
       * Restore cross/fwd edges which has h as the LCA.
       */
      auto h = get_ref(hn);
      auto it = _cross_fwds.find(h);
      if (it != _cross_fwds.end()) {
        for (auto& edge : it->second) {
          // edge: u -> v
          auto& u = edge.first;
          auto& v = edge.second;
          auto rep_v = rep[dsets.find_set(v)];
          _non_back_preds[rep_v].push_back(u);
        }
      }

      /*
       * Find nested SCCs.
       * If a nested SCC is non-trivial, only its head is tracked as a representative.
       */
      bool is_SCC = false;
      std::unordered_set<NodeRef> backpreds_h;
      for (auto v : _back_preds[h]) {
        if (v != h) {
          backpreds_h.insert(rep[dsets.find_set(v)]);
        } else {
          // Self-loop.
          is_SCC = true;
        }

        _succ_table[v] = std::make_pair(h, false);
      }
      // Invariant: h \notin backpreds_h. Even if h has a self loop.
      if (!backpreds_h.empty()) {
        is_SCC = true;
      }
      std::unordered_set<NodeRef> nested_SCCs_h(backpreds_h);
      std::vector<NodeRef> worklist_h(backpreds_h.begin(), backpreds_h.end());
      while (!worklist_h.empty()) {
        auto v = worklist_h.back();
        worklist_h.pop_back();
        for (auto p : _non_back_preds[v]) {
          auto rep_p = rep[dsets.find_set(p)];
          auto it = nested_SCCs_h.find(rep_p);
          if (it == nested_SCCs_h.end() && rep_p != h) {
            nested_SCCs_h.insert(rep_p);
            worklist_h.push_back(rep_p);
          }
        }
      }
      // Invariant: h \notin nested_SCCs_h.

      /*
       * Process dependencies between nested SCCs.
       * If h is trivial, just skip.
       */
      if (!is_SCC) {
        // Invariant: h is a trivial SCC.
        set_comp(h, std::make_unique< WtoVertexT >(h));
        set_call_check(h);

        // Invariant: wto_space = ...::h
        continue;
      }
      // Invariant: h is a non-trivial SCC.
      set_call_check(h);
      set_head(h);
      set_in_loop(h);

      /*
       * Sort the nested SCCs.
       */
      WtoComponentList partition;
      std::vector<Dfn> posts;
      std::unordered_map<Dfn, NodeRef> ref(nested_SCCs_h.size());
      for (auto v : nested_SCCs_h) {
        auto post = get_post_dfn(v);
        ref[post] = v;
        posts.push_back(post);
      }
      std::sort(posts.begin(), posts.end(), std::greater<>());
      for (auto post : posts) {
        auto v = ref[post];
        partition.push_back(std::move(_comp_table[v]));

        for (auto u : _non_back_preds[v]) {
          if (backpreds_h.find(u) != backpreds_h.end()) {
            continue;
          }
          _succ_table[u] = std::make_pair(v, is_head(v));

          // Detects edges that go outside of a component.
          auto& u_outermost = rep[dsets.find_set(u)];
          if (u != h && is_head(u_outermost)) {
            auto head = is_head(u) ? u : _parent[u];
            while (head != u_outermost) {
              _wto._children_with_post_table[head].insert(u);
              head = _parent[head];
            }
            _wto._children_with_post_table[head].insert(u);
          }
        }
      }
      set_comp(h, std::make_unique< WtoCycleT >(h, std::move(partition)));
      // Invariant: WTO for SCC with h as its header is constructed.

      /*
       * Update information regarding the nesting structure.
       * Merge the partitions with new information.
       */
      for (auto v : nested_SCCs_h) {
        dsets.union_set(v, h);
        rep[dsets.find_set(v)] = h;
        _parent[v] = h;

        if (!is_head(v)) {
          set_in_loop(v);
        }
      }
    }

    /*
     * Sort top-level SCCs.
     */
    std::vector<Dfn> posts;
    std::unordered_map<Dfn, NodeRef> ref;
    for (auto vn = Dfn(1); vn < get_next_dfn(); vn++) {
      auto v = get_ref(vn);
      if (rep[dsets.find_set(v)] == v) {
        if (is_head(v)) {
          set_outermost_component(v);
        }
        _parent[v] = v;

        auto post = get_post_dfn(v);
        ref[post] = v;
        posts.push_back(post);
      }
    }
    std::sort(posts.begin(), posts.end(), std::greater<>());
    for (auto post : posts) {
      auto v = ref[post];
      _wto._components.push_back(std::move(_comp_table[v]));

      for (auto u : _non_back_preds[v]) {
        _succ_table[u] = std::make_pair(v, is_head(v));

        // Detects edges that go outside of a component.
        auto& u_outermost = rep[dsets.find_set(u)];
        if (is_head(u_outermost)) {
          auto head = is_head(u) ? u : _parent[u];
          while (head != u_outermost) {
            _wto._children_with_post_table[head].insert(u);
            head = _parent[head];
          }
          _wto._children_with_post_table[head].insert(u);
        }
      }
    }
    // Invariant: WTO for the input graph is constructed.
  }

  void construct_meta_info() {
    for (auto& p : _succ_table) {
      auto u = p.first;
      auto v = p.second.first;
      auto comp_pred = p.second.second;
      if (comp_pred) {
        _wto._comp_preds_table[v].insert(u);
      } else {
        _wto._last_user_table[u] = v;
      }
    }

    for (auto c : this->_wto._has_check_set) {
      // Don't set c as head, even if c is a head node.
      // This interferes with widening.
      // In particular, don't use the below commented out code.
      // auto prev = nullptr;
      // auto head = is_head(c) ? c : _parent[c];
      NodeRef prev = c;
      auto head = _parent[c];
      while (head != prev) {
        _wto._children_with_checks_table[head].insert(c);
        prev = head;
        head = _parent[head];
      }
    }

    for (auto c : _calls) {
      NodeRef prev = nullptr;
      auto head = is_head(c) ? c : _parent[c];
      while (head != prev) {
        _wto._children_with_calls_table[head].insert(c);
        prev = head;
        head = _parent[head];
      }
    }
  }

  Dfn get_dfn(NodeRef node) {
    auto it = this->_dfn.find(node);
    if (it != this->_dfn.end()) {
      return it->second;
    }
    return Dfn(0);
  }

  NodeRef get_ref(Dfn dfn) {
    auto it = this->_ref.find(dfn);
    if (it != this->_ref.end()) {
      return it->second;
    }
    return nullptr;
  }

  void set_dfn(NodeRef node, const Dfn& dfn) {
    auto res = this->_dfn.insert(std::make_pair(node, dfn));
    if (!res.second) {
      (res.first)->second = dfn;
    }

    auto res2 = this->_ref.insert(std::make_pair(dfn, node));
    if (!res2.second) {
      (res2.first)->second = node;
    }
  }

  Dfn get_post_dfn(NodeRef node) {
    auto it = this->_post_dfn.find(node);
    if (it != this->_post_dfn.end()) {
      return it->second;
    }
    return Dfn(0);
  }

  /// \brief Set the post depth-first number of the given node
  void set_post_dfn(NodeRef node, std::size_t dfn) {
    auto res = this->_post_dfn.insert(std::make_pair(node, dfn));
    if (!res.second) {
      (res.first)->second = dfn;
    }
  }

  Dfn get_next_dfn() const {
    // Includes exits.
    // 0 represents invalid node.
    return _next_dfn;
  }

  void set_comp(NodeRef node, std::unique_ptr<WtoComponentT>&& comp) {
    auto res = this->_comp_table.insert(std::make_pair(node, std::move(comp)));
    if (!res.second) {
      (res.first)->second = std::move(comp);
    }
  }

  void set_call_check(NodeRef node) {
    for (ar::Statement* stmt : *node) {
      if (isa<ar::CallBase>(stmt)) {
        _calls.push_back(node);
        break;
      }
    }

    for (ar::Statement* stmt : *node) {
      for (const auto& checker : this->_checkers) {
        if (checker->has_check(stmt)) {
          this->_wto._has_check_set.insert(node);
          return;
        }
      }
    }
  }

  bool is_head(NodeRef node) const {
    return this->_is_head.find(node) != this->_is_head.end();
  }

  void set_head(NodeRef node) {
    this->_is_head.insert(node);
  }

  void set_in_loop(NodeRef node) {
    this->_wto._is_in_loop_set.insert(node);
  }

  void set_outermost_component(NodeRef node) {
    this->_wto._is_outermost_component_set.insert(node);
  }

  /// \brief A reference to Wto space (array of Wto nodes).
  WtoT& _wto;
  DfnTable _dfn;
  DfnInverseTable _ref;
  DfnTable& _post_dfn;
  /// \brief Next DFN to assign.
  Dfn _next_dfn;
  /// \brief Next post DFN to assign.
  Dfn _next_post_dfn;

  /// \brief A map from node to its backedge predecessors.
  std::unordered_map<NodeRef, std::vector<NodeRef>> _back_preds;
  /// \brief A map from node to its non-backedge predecessors.
  std::unordered_map<NodeRef, std::vector<NodeRef>> _non_back_preds;
  /// \brief A map from node to cross/forward edges (key is the edge's lowest common ancestor).
  std::unordered_map<NodeRef, std::vector<std::pair<NodeRef, NodeRef>>> _cross_fwds;

  std::unordered_map<NodeRef, std::unique_ptr<WtoComponentT>> _comp_table;
  SuccTable _succ_table;

  const std::vector<std::unique_ptr<analyzer::Checker>>& _checkers;
  /// \brief A map from node to the head of minimal component that contains it.
  /// If given node is a header, it returns the header of its parent component.
  std::unordered_map<NodeRef, NodeRef> _parent;
  std::unordered_set<NodeRef> _is_head;
  std::vector<NodeRef> _calls;
}; // end class wto_builder

} // end namespace wto_impl

} // end namespace memory
} // end namespace core
} // end namespace ikos
