/*******************************************************************************
 *
 * \file
 * \brief Fixpoint on a function body
 *
 * Notices:
 *
 * Copyright (c) 2019 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Disclaimers:
 *
 * No Warranty: THE SUBJECT SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY OF
 * ANY KIND, EITHER EXPRESSED, IMPLIED, OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL CONFORM TO SPECIFICATIONS,
 * ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 * OR FREEDOM FROM INFRINGEMENT, ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL BE
 * ERROR FREE, OR ANY WARRANTY THAT DOCUMENTATION, IF PROVIDED, WILL CONFORM TO
 * THE SUBJECT SOFTWARE. THIS AGREEMENT DOES NOT, IN ANY MANNER, CONSTITUTE AN
 * ENDORSEMENT BY GOVERNMENT AGENCY OR ANY PRIOR RECIPIENT OF ANY RESULTS,
 * RESULTING DESIGNS, HARDWARE, SOFTWARE PRODUCTS OR ANY OTHER APPLICATIONS
 * RESULTING FROM USE OF THE SUBJECT SOFTWARE.  FURTHER, GOVERNMENT AGENCY
 * DISCLAIMS ALL WARRANTIES AND LIABILITIES REGARDING THIRD-PARTY SOFTWARE,
 * IF PRESENT IN THE ORIGINAL SOFTWARE, AND DISTRIBUTES IT "AS IS."
 *
 * Waiver and Indemnity:  RECIPIENT AGREES TO WAIVE ANY AND ALL CLAIMS AGAINST
 * THE UNITED STATES GOVERNMENT, ITS CONTRACTORS AND SUBCONTRACTORS, AS WELL
 * AS ANY PRIOR RECIPIENT.  IF RECIPIENT'S USE OF THE SUBJECT SOFTWARE RESULTS
 * IN ANY LIABILITIES, DEMANDS, DAMAGES, EXPENSES OR LOSSES ARISING FROM SUCH
 * USE, INCLUDING ANY DAMAGES FROM PRODUCTS BASED ON, OR RESULTING FROM,
 * RECIPIENT'S USE OF THE SUBJECT SOFTWARE, RECIPIENT SHALL INDEMNIFY AND HOLD
 * HARMLESS THE UNITED STATES GOVERNMENT, ITS CONTRACTORS AND SUBCONTRACTORS,
 * AS WELL AS ANY PRIOR RECIPIENT, TO THE EXTENT PERMITTED BY LAW.
 * RECIPIENT'S SOLE REMEDY FOR ANY SUCH MATTER SHALL BE THE IMMEDIATE,
 * UNILATERAL TERMINATION OF THIS AGREEMENT.
 *
 ******************************************************************************/

#pragma once

#include <ikos/ar/semantic/code.hpp>
#include <ikos/ar/semantic/function.hpp>

#include <ikos/core/fixpoint/fwd_fixpoint_iterator.hpp>
#include <ikos/core/fixpoint/memopt_fwd_fixpoint_iterator.hpp>

#include <ikos/analyzer/analysis/call_context.hpp>
#include <ikos/analyzer/analysis/context.hpp>
#include <ikos/analyzer/analysis/execution_engine/memopt_inliner.hpp>
#include <ikos/analyzer/analysis/execution_engine/numerical.hpp>
#include <ikos/analyzer/analysis/fixpoint_parameters.hpp>
#include <ikos/analyzer/analysis/value/abstract_domain.hpp>
#include <ikos/analyzer/analysis/value/interprocedural/progress.hpp>
#include <ikos/analyzer/checker/checker.hpp>

namespace ikos {
namespace analyzer {
namespace value {
namespace interprocedural {
namespace memory {

/// \brief Fixpoint on a function body
class FunctionFixpoint final
    : public core::memory::InterleavedFwdFixpointIterator< ar::Code*, AbstractDomain > {
private:
  /// \brief Parent class
  using FwdFixpointIterator =
      core::memory::InterleavedFwdFixpointIterator< ar::Code*, AbstractDomain >;

  /// \brief Numerical execution engine
  using NumericalExecutionEngineT = NumericalExecutionEngine< AbstractDomain >;

  /// \brief Inliner
  using InlineCallExecutionEngineT =
      analyzer::memory::InlineCallExecutionEngine< FunctionFixpoint, AbstractDomain >;

private:
  /// \brief Analyzed function
  ar::Function* _function;

  /// \brief Current call context
  CallContext* _call_context;

  /// \brief Fixpoint parameters
  const CodeFixpointParameters& _fixpoint_parameters;

  /// \brief List of property checks to run
  const std::vector< std::unique_ptr< Checker > >& _checkers;

  /// \brief Progress logger
  ProgressLogger& _logger;

  /// \brief Numerical execution engine
  NumericalExecutionEngineT _exec_engine;

  /// \brief Call execution engine
  InlineCallExecutionEngineT _call_exec_engine;

  /// \brief Cache of the values of calls
  std::unordered_map< ar::BasicBlock*,
    std::unordered_map< ar::CallBase*, AbstractDomain >> _call_cache;

  /// \brief Cache of the callee analyzers
  std::unordered_map< ar::BasicBlock*,
    std::vector< std::unique_ptr< FunctionFixpoint >>> _callee_cache;

public:
  /// \brief Constructor for an entry point
  ///
  /// \param ctx Analysis context
  /// \param checkers List of checkers to run
  /// \param entry_point Function to analyze
  FunctionFixpoint(Context& ctx,
                   const std::vector< std::unique_ptr< Checker > >& checkers,
                   ProgressLogger& logger,
                   ar::Function* entry_point);

  /// \brief Constructor for a callee
  ///
  /// \param ctx Analysis context
  /// \param caller Parent function fixpoint
  /// \param call Call statement
  /// \param callee Called function
  /// \param defer_checks Boolean flag on whether to defer running the checks
  FunctionFixpoint(Context& ctx,
                   const FunctionFixpoint& caller,
                   ar::CallBase* call,
                   ar::Function* callee,
                   bool defer_checks);

  /// \brief Compute the fixpoint
  void run(AbstractDomain inv);

  /// \brief Post invariants are cleared to save memory
  const AbstractDomain& post(ar::BasicBlock*) const = delete;

  /// \brief Extrapolate the new state after an increasing iteration
  AbstractDomain extrapolate(ar::BasicBlock* head,
                             unsigned iteration,
                             const AbstractDomain& before,
                             const AbstractDomain& after) override;

  /// \brief Refine the new state after a decreasing iteration
  AbstractDomain refine(ar::BasicBlock* head,
                        unsigned iteration,
                        const AbstractDomain& before,
                        const AbstractDomain& after) override;

  /// \brief Check if the decreasing iterations fixpoint is reached
  bool is_decreasing_iterations_fixpoint(ar::BasicBlock* head,
                                         unsigned iteration,
                                         const AbstractDomain& before,
                                         const AbstractDomain& after) override;

  /// \brief Propagate the invariant through the basic block
  AbstractDomain analyze_node(ar::BasicBlock* bb,
                              AbstractDomain pre,
                              bool defer_checks,
                              bool cache_calls) override;

  /// \brief Run deferred checks with the previously computed fix-points
  void run_deferred_checks_in_component(ar::BasicBlock* head) override;

  /// \brief Erase previously computed fix-point cached for deferred checks
  void erase_values_cached_for_deferred_checks(ar::BasicBlock* head) override;

  /// \brief Propagate the invariant through an edge
  AbstractDomain analyze_edge(ar::BasicBlock* src,
                              ar::BasicBlock* dest,
                              AbstractDomain pre) override;

  /// \brief Notify the beginning of the analysis of a cycle
  void notify_enter_cycle(ar::BasicBlock* head) override;

  /// \brief Notify the beginning of an iteration on a cycle
  void notify_cycle_iteration(ar::BasicBlock* head,
                              unsigned iteration,
                              core::FixpointIterationKind kind) override;

  /// \brief Notify the end of the analysis of a cycle
  void notify_leave_cycle(ar::BasicBlock* head) override;

  /// \brief Process the computed abstract value for a node
  void process_pre(ar::BasicBlock* bb, const AbstractDomain& pre) override;

  /// \brief Process the computed abstract value for a node
  void process_post(ar::BasicBlock* bb, const AbstractDomain& post) override;

  /// \brief Run the deferred checks with the previously computed fix-point
  void run_deferred_checks(ar::BasicBlock* bb);

  /// \brief Run the deferred checks for the callees in the basic block
  void run_deferred_checks_in_callees(ar::BasicBlock* bb);

  /// \brief Run all remaining deferred checks in this function
  void run_all_deferred_checks();

  /// \name Helpers for InlineCallExecutionEngine
  /// @{

  /// \brief Return the function
  ar::Function* function() const { return this->_function; }

  /// \brief Return the call context
  CallContext* call_context() const { return this->_call_context; }

  /// \brief Return the exit invariant, or bottom
  AbstractDomain& exit_invariant() {
    return this->_call_exec_engine.exit_invariant();
  }

  /// \brief Return the return statement, or null
  ar::ReturnValue* return_stmt() const {
    return this->_call_exec_engine.return_stmt();
  }

  /// \brief Set the call result for the given call statement in the basic block
  void set_call_cache(ar::BasicBlock* bb, ar::CallBase* call, AbstractDomain inv) {
    auto it = this->_call_cache[bb].find(call);
    if (it != this->_call_cache[bb].end()) {
      it->second = std::move(inv);
    } else {
      this->_call_cache[bb].emplace(call, std::move(inv));
    }
  }

  /// \brief Erase the call results for all calls in the given basic block
  void erase_call_cache(ar::BasicBlock* bb) {
    auto it = this->_call_cache.find(bb);
    if (it != this->_call_cache.end()) {
      it->second.clear();
      this->_call_cache.erase(bb);
    }
  }

  /// \brief Append the callee analyzer to the given basic block
  void append_to_callee_cache(ar::BasicBlock* bb, std::unique_ptr< FunctionFixpoint >& callee) {
    this->_callee_cache[bb].emplace_back(std::move(callee));
  }

  /// \brief Erase the callees analyzers for the given basic block
  void erase_callee_cache(ar::BasicBlock* bb) {
    auto it = this->_callee_cache.find(bb);
    if (it != this->_callee_cache.end()) {
      it->second.clear();
      this->_callee_cache.erase(bb);
    }
  }

  /// \brief Check if there are deferred checks in this function
  bool has_deferred_checks() const {
    return !this->_callee_cache.empty() || !this->pre().empty();
  }

  /// @}

}; // end class FunctionFixpoint

} // end namespace memory
} // end namespace interprocedural
} // end namespace value
} // end namespace analyzer
} // end namespace ikos
