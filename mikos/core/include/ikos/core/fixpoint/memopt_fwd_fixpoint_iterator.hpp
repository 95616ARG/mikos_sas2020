#pragma once

#include <memory>
#include <unordered_map>
#include <utility>

#include <ikos/core/fixpoint/memopt_fixpoint_iterator.hpp>
#include <ikos/core/fixpoint/memopt_wto.hpp>

namespace ikos {
namespace core {
namespace memory {

namespace interleaved_fwd_fixpoint_iterator_impl {

template < typename GraphRef, typename AbstractValue, typename GraphTrait >
class WtoIterator;

template < typename GraphRef, typename AbstractValue, typename GraphTrait >
class WtoProcessor;

} // end namespace interleaved_fwd_fixpoint_iterator_impl

/// \brief Interleaved forward fixpoint iterator
///
/// This class computes a fixpoint on a control flow graph.
template < typename GraphRef,
           typename AbstractValue,
           typename GraphTrait = GraphTraits< GraphRef > >
class InterleavedFwdFixpointIterator
    : public MForwardFixpointIterator< GraphRef, AbstractValue, GraphTrait > {
  friend class interleaved_fwd_fixpoint_iterator_impl::
      WtoIterator< GraphRef, AbstractValue, GraphTrait >;

private:
  using NodeRef = typename GraphTrait::NodeRef;
  using InvariantTable = std::unordered_map< NodeRef, AbstractValue >;
  using WtoT = Wto< GraphRef, GraphTrait >;
  using WtoIterator = interleaved_fwd_fixpoint_iterator_impl::
      WtoIterator< GraphRef, AbstractValue, GraphTrait >;
  using WtoProcessor = interleaved_fwd_fixpoint_iterator_impl::
      WtoProcessor< GraphRef, AbstractValue, GraphTrait >;

private:
  GraphRef _cfg;
  WtoT _wto;
  InvariantTable _pre;
  InvariantTable _post;
  bool _defer_checks;
  NodeRef _exit;
  AbstractValue _bottom;

public:
  /// \brief Create an interleaved forward fixpoint iterator
  ///
  /// \param cfg The control flow graph
  /// \param bottom The bottom abstract value
  InterleavedFwdFixpointIterator(
      GraphRef cfg,
      AbstractValue bottom,
      const std::vector<std::unique_ptr<analyzer::Checker>>& checkers,
      bool defer_checks)
      : _cfg(cfg),
        _wto(cfg, checkers),
        _defer_checks(defer_checks),
        _exit(cfg->exit_block_or_null()),
        _bottom(std::move(bottom)) {}

  /// \brief No copy constructor
  InterleavedFwdFixpointIterator(const InterleavedFwdFixpointIterator&) =
      delete;

  /// \brief Move constructor
  InterleavedFwdFixpointIterator(InterleavedFwdFixpointIterator&&) = default;

  /// \brief No copy assignment operator
  InterleavedFwdFixpointIterator& operator=(
      const InterleavedFwdFixpointIterator&) = delete;

  /// \brief Move assignment operator
  InterleavedFwdFixpointIterator& operator=(InterleavedFwdFixpointIterator&&) =
      default;

  /// \brief Get the control flow graph
  GraphRef cfg() const { return this->_cfg; }

  /// \brief Get the weak topological order of the graph
  const WtoT& wto() const { return this->_wto; }

  /// \brief Get the bottom abstract value
  const AbstractValue& bottom() const { return this->_bottom; }

private:
  /// \brief Set the invariant for the given node
  void set(InvariantTable& table, NodeRef node, AbstractValue inv) const {
    auto it = table.find(node);
    ikos_assert(it == table.end());
    if (it != table.end()) {
      exit(101);
    }
    table.emplace(node, std::move(inv));
  }

  /// \brief Set the pre invariant for the given node
  void set_pre(NodeRef node, AbstractValue inv) {
    this->set(this->_pre, node, std::move(inv));
  }

  /// \brief Set the post invariant for the given node
  void set_post(NodeRef node, AbstractValue inv) {
    if (node->num_successors() == 0 && node != _exit) {
      return;
    }
    this->set(this->_post, node, std::move(inv));
  }

  /// \brief Erase the invariant for the given node
  void erase(InvariantTable& table, NodeRef node) const {
    auto it = table.find(node);
    ikos_assert(it != table.end());
    if (it == table.end()) {
      exit(102);
    }
    it->second.set_to_bottom();
    table.erase(node);
  }

  /// \brief Get the invariant for the given node
  const AbstractValue& get(const InvariantTable& table, NodeRef node) const {
    auto it = table.find(node);
    if (it != table.end()) {
      return it->second;
    } else {
      return this->_bottom;
    }
  }

protected:
  /// \brief Erase the pre invariant for the given node
  void erase_pre(NodeRef node) {
    this->erase(this->_pre, node);
  }

  /// \brief Erase the post invariant for the given node
  void erase_post(NodeRef node) {
    this->erase(this->_post, node);
  }

  /// \brief Accessor for pre
  const InvariantTable& pre() const {
    return this->_pre;
  }

public:
  /// \brief Get the pre invariant for the given node
  const AbstractValue& pre(NodeRef node) const {
    return this->get(this->_pre, node);
  }

  /// \brief Get the post invariant for the given node
  const AbstractValue& post(NodeRef node) const {
    return this->get(this->_post, node);
  }

  /// \brief Extrapolate the new state after an increasing iteration
  ///
  /// This is called after each iteration of a cycle, until the fixpoint is
  /// reached. In order to converge, the widening operator must be applied.
  /// This method gives the user the ability to use different widening
  /// strategies.
  ///
  /// By default, it applies a join for the first iteration, and then the
  /// widening until it reaches the fixpoint.
  ///
  /// \param head Head of the cycle
  /// \param iteration Iteration number
  /// \param before Abstract value before the iteration
  /// \param after Abstract value after the iteration
  virtual AbstractValue extrapolate(NodeRef head,
                                    unsigned iteration,
                                    const AbstractValue& before,
                                    const AbstractValue& after) {
    ikos_ignore(head);

    if (iteration <= 1) {
      return before.join_iter(after);
    } else {
      return before.widening(after);
    }
  }

  /// \brief Check if the increasing iterations fixpoint is reached
  ///
  /// \param head Head of the cycle
  /// \param iteration Iteration number
  /// \param before Abstract value before the iteration
  /// \param after Abstract value after the iteration
  virtual bool is_increasing_iterations_fixpoint(NodeRef head,
                                                 unsigned iteration,
                                                 const AbstractValue& before,
                                                 const AbstractValue& after) {
    ikos_ignore(head);
    ikos_ignore(iteration);

    return after.leq(before);
  }

  /// \brief Refine the new state after a decreasing iteration
  ///
  /// This is called after each iteration of a cycle, until the post fixpoint
  /// is reached. In order to converge, the narrowing operator must be applied.
  /// This method gives the user the ability to use different narrowing
  /// strategies.
  ///
  /// By default, it applies the narrowing until it reaches the post fixpoint.
  ///
  /// \param head Head of the cycle
  /// \param iteration Iteration number
  /// \param before Abstract value before the iteration
  /// \param after Abstract value after the iteration
  virtual AbstractValue refine(NodeRef head,
                               unsigned iteration,
                               const AbstractValue& before,
                               const AbstractValue& after) {
    ikos_ignore(head);
    ikos_ignore(iteration);

    return before.narrowing(after);
  }

  /// \brief Check if the decreasing iterations fixpoint is reached
  ///
  /// \param head Head of the cycle
  /// \param iteration Iteration number
  /// \param before Abstract value before the iteration
  /// \param after Abstract value after the iteration
  virtual bool is_decreasing_iterations_fixpoint(NodeRef head,
                                                 unsigned iteration,
                                                 const AbstractValue& before,
                                                 const AbstractValue& after) {
    ikos_ignore(head);
    ikos_ignore(iteration);

    return before.leq(after);
  }

  /// \brief Notify the beginning of the analysis of a cycle
  ///
  /// This method is called before analyzing a cycle.
  virtual void notify_enter_cycle(NodeRef head) { ikos_ignore(head); }

  /// \brief Notify the beginning of an iteration on a cycle
  ///
  /// This method is called for each iteration on a cycle.
  virtual void notify_cycle_iteration(NodeRef head,
                                      unsigned iteration,
                                      FixpointIterationKind kind) {
    ikos_ignore(head);
    ikos_ignore(iteration);
    ikos_ignore(kind);
  }

  /// \brief Notify the end of the analysis of a cycle
  ///
  /// This method is called after reaching a fixpoint on a cycle.
  virtual void notify_leave_cycle(NodeRef head) { ikos_ignore(head); }

  /// \brief Compute the fixpoint with the given initial abstract value
  void run(AbstractValue init) {
    this->set_pre(GraphTrait::entry(this->_cfg), std::move(init));

    // Compute the fixpoint
    WtoIterator iterator(*this);
    this->_wto.accept(iterator);

    // Call process_pre/process_post methods
    WtoProcessor processor(*this);
    this->_wto.accept(processor);

    auto exit_block = this->_cfg->exit_block_or_null();
    if (exit_block != nullptr) {
      this->erase_post(exit_block);
    }

    ikos_assert(this->_defer_checks || this->_pre.empty());
    if (!this->_defer_checks && !this->_pre.empty()) {
      exit(104);
    }
    ikos_assert(this->_post.empty());
    if (!this->_post.empty()) {
      exit(105);
    }

    this->_wto.clear();
  }

  /// \brief Clear the pre invariants
  void clear_pre() { this->_pre.clear(); }

  /// \brief Clear the post invariants
  void clear_post() { this->_post.clear(); }

  /// \brief Clear the current fixpoint
  void clear() {
    this->_pre.clear();
    this->_post.clear();
  }

  /// \brief Destructor
  ~InterleavedFwdFixpointIterator() override = default;

}; // end class InterleavedFwdFixpointIterator

namespace interleaved_fwd_fixpoint_iterator_impl {

template < typename GraphRef, typename AbstractValue, typename GraphTrait >
class WtoIterator final : public WtoComponentVisitor< GraphRef, GraphTrait > {
public:
  using InterleavedIterator =
      InterleavedFwdFixpointIterator< GraphRef, AbstractValue, GraphTrait >;
  using NodeRef = typename GraphTrait::NodeRef;
  using WtoVertexT = WtoVertex< GraphRef, GraphTrait >;
  using WtoCycleT = WtoCycle< GraphRef, GraphTrait >;
  using WtoT = Wto< GraphRef, GraphTrait >;
  using WtoNestingT = WtoNesting< GraphRef, GraphTrait >;

private:
  /// \brief Fixpoint engine
  InterleavedIterator& _iterator;

  /// \brief Graph entry point
  NodeRef _entry;

public:
  explicit WtoIterator(InterleavedIterator& iterator)
      : _iterator(iterator), _entry(GraphTrait::entry(iterator.cfg())) {}

  void visit(const WtoVertexT& vertex) override {
    NodeRef node = vertex.node();
    AbstractValue pre = this->_iterator.bottom();

    // Use the invariant for the entry point
    if (node == this->_entry) {
      pre = std::move(this->_iterator.pre(node));
      this->_iterator.erase_pre(node);
    }

    // Determine whether to defer the checks and cache PRE values.
    bool is_in_loop = this->_iterator.wto().is_in_loop(node);
    bool defer_checks = this->_iterator._defer_checks || is_in_loop;
    bool cache_values = defer_checks && this->_iterator.wto().has_check(node);

    // Collect invariants from incoming edges
    for (auto it = GraphTrait::predecessor_begin(node),
              et = GraphTrait::predecessor_end(node);
         it != et;
         ++it) {
      NodeRef pred = *it;
      pre.join_with(
          std::move(this->_iterator.analyze_edge(pred, node, this->_iterator.post(pred))));

      // Delete pred's POST value if node is its last user.
      if (this->_iterator.wto().last_user(pred) == node) {
        this->_iterator.erase_post(pred);
      }
    }

    // Delete node's PRE value if the node is toplevel or does not has check.
    // That is, cache if node is in some loop and has check.
    if (cache_values) {
      this->_iterator.set_pre(node, pre);
    }

    this->_iterator.set_post(node, std::move(this->_iterator.analyze_node(node,
                                                                          std::move(pre),
                                                                          defer_checks,
                                                                          cache_values)));
  }

  void visit(const WtoCycleT& cycle) override {
    NodeRef head = cycle.head();
    AbstractValue pre = this->_iterator.bottom();

    this->_iterator.notify_enter_cycle(head);

    // Collect invariants from incoming edges
    for (auto it = GraphTrait::predecessor_begin(head),
              et = GraphTrait::predecessor_end(head);
         it != et;
         ++it) {
      NodeRef pred = *it;
      if (this->_iterator.wto().is_from_outside(head, pred)) {
        // DAG edges.
        AbstractValue inv =
            std::move(this->_iterator.analyze_edge(pred,
                                                   head,
                                                   this->_iterator.post(pred)));
        // Because head is in a loop, deallocations of POST values are deferred
        // until the component's stabilization.
        pre.join_with(std::move(inv));
      }
    }


    // Fixpoint iterations
    FixpointIterationKind kind = FixpointIterationKind::Increasing;
    for (unsigned iteration = 1;; ++iteration) {
      this->_iterator.notify_cycle_iteration(head, iteration, kind);
      this->_iterator.set_post(head, std::move(this->_iterator.analyze_node(head,
                                                                            pre,
                                                                            /*defer_checks=*/true,
                                                                            /*cache_calls=*/this->_iterator.wto().has_check(head))));

      for (auto it = cycle.begin(), et = cycle.end(); it != et; ++it) {
        it->accept(*this);
      }

      // Invariant from the head of the loop
      AbstractValue new_pre_in = this->_iterator.bottom();

      // Invariant from the tail of the loop
      AbstractValue new_pre_back = this->_iterator.bottom();

      for (auto it = GraphTrait::predecessor_begin(head),
                et = GraphTrait::predecessor_end(head);
           it != et;
           ++it) {
        NodeRef pred = *it;
        AbstractValue inv =
            std::move(this->_iterator.analyze_edge(pred,
                                                   head,
                                                   this->_iterator.post(pred)));
        if (this->_iterator.wto().is_from_outside(head, pred)) {
          // DAG edges.
          // Because head is in a loop, deallocations of POST values are deferred
          // until the component's stabilization.
          new_pre_in.join_with(std::move(inv));
        } else {
          // Back edges.
          // Delete pred's POST value if head is its last user.
          if (this->_iterator.wto().last_user(pred) == head) {
            this->_iterator.erase_post(pred);
          }
          new_pre_back.join_with(std::move(inv));
        }
      }

      new_pre_in.join_loop_with(std::move(new_pre_back));
      AbstractValue new_pre(std::move(new_pre_in));

      if (kind == FixpointIterationKind::Increasing) {
        // Increasing iteration with widening
        AbstractValue inv =
            std::move(this->_iterator.extrapolate(head, iteration, pre, new_pre));
        if (this->_iterator.is_increasing_iterations_fixpoint(head,
                                                              iteration,
                                                              pre,
                                                              inv)) {
          // Post-fixpoint reached
          // Use this iteration as a decreasing iteration
          kind = FixpointIterationKind::Decreasing;
          iteration = 1;
        } else {
          pre = std::move(inv);
          this->_iterator.erase_values_cached_for_deferred_checks(head);
        }
      }

      if (kind == FixpointIterationKind::Decreasing) {
        // Decreasing iteration with narrowing
        AbstractValue inv =
            std::move(this->_iterator.refine(head, iteration, pre, new_pre));
        if (this->_iterator.is_decreasing_iterations_fixpoint(head,
                                                              iteration,
                                                              pre,
                                                              inv)) {
          // No more refinement possible
          //
          // Delete POST values that were deferred until the stabilization
          // of this component.
          for (auto& pred : this->_iterator.wto().get_component_predecessors(head)) {
            this->_iterator.erase_post(pred);
          }
          // If the head does not have a check, delete its PRE value.
          // That is, do not cache it.
          if (this->_iterator.wto().has_check(head)) {
            this->_iterator.set_pre(head, std::move(inv));
          }
          // Run the deferred checks if this is the outermost component.
          if (!this->_iterator._defer_checks &&
              this->_iterator.wto().is_outermost_component(head)) {
            this->_iterator.run_deferred_checks_in_component(head);
          }
          break;
        } else {
          pre = std::move(inv);
          this->_iterator.erase_values_cached_for_deferred_checks(head);
        }
      }
    }

    this->_iterator.notify_leave_cycle(head);
  }

}; // end class WtoIterator

template < typename GraphRef, typename AbstractValue, typename GraphTrait >
class WtoProcessor final : public WtoComponentVisitor< GraphRef, GraphTrait > {
public:
  using InterleavedIterator =
      InterleavedFwdFixpointIterator< GraphRef, AbstractValue, GraphTrait >;
  using NodeRef = typename GraphTrait::NodeRef;
  using WtoVertexT = WtoVertex< GraphRef, GraphTrait >;
  using WtoCycleT = WtoCycle< GraphRef, GraphTrait >;

private:
  InterleavedIterator& _iterator;

public:
  explicit WtoProcessor(InterleavedIterator& iterator) : _iterator(iterator) {}

  void visit(const WtoVertexT& vertex) override {
    NodeRef node = vertex.node();
    this->_iterator.process_pre(node, this->_iterator.pre(node));
    this->_iterator.process_post(node, this->_iterator.post(node));
  }

  void visit(const WtoCycleT& cycle) override {
    NodeRef head = cycle.head();
    this->_iterator.process_pre(head, this->_iterator.pre(head));
    this->_iterator.process_post(head, this->_iterator.post(head));

    for (auto it = cycle.begin(), et = cycle.end(); it != et; ++it) {
      it->accept(*this);
    }
  }

}; // end class WtoProcessor

} // end namespace interleaved_fwd_fixpoint_iterator_impl

} // end namespace memory
} // end namespace core
} // end namespace ikos
