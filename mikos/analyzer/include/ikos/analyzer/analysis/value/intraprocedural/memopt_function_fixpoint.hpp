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
#include <ikos/analyzer/analysis/fixpoint_parameters.hpp>
#include <ikos/analyzer/analysis/value/abstract_domain.hpp>
#include <ikos/analyzer/checker/checker.hpp>

namespace ikos {
namespace analyzer {
namespace value {
namespace intraprocedural {
namespace memory {

/// \brief Fixpoint on a function body
class FunctionFixpoint final
    : public core::memory::InterleavedFwdFixpointIterator< ar::Code*, AbstractDomain > {
private:
  /// \brief Parent class
  using FwdFixpointIterator =
      core::memory::InterleavedFwdFixpointIterator< ar::Code*, AbstractDomain >;

private:
  /// \brief Analysis context
  Context& _ctx;

  /// \brief Empty call context
  CallContext* _empty_call_context;

  /// \brief List of property checks to run
  const std::vector< std::unique_ptr< Checker > >& _checkers;

  /// \brief Fixpoint parameters
  const CodeFixpointParameters& _fixpoint_parameters;

public:
  /// \brief Create a function fixpoint iterator
  FunctionFixpoint(Context& ctx,
                   const std::vector< std::unique_ptr< Checker > >& checkers,
                   ar::Function* function);

  /// \brief Compute the fixpoint
  void run(AbstractDomain inv);

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

  /// \brief Process the computed abstract value for a node
  void process_pre(ar::BasicBlock* bb, const AbstractDomain& pre) override;

  /// \brief Process the computed abstract value for a node
  void process_post(ar::BasicBlock* bb, const AbstractDomain& post) override;

  /// \brief Run the deferred checks with the previously computed fix-point
  void run_deferred_checks(ar::BasicBlock* bb);

}; // end class FunctionFixpoint

} // end namespace memory
} // end namespace intraprocedural
} // end namespace value
} // end namespace analyzer
} // end namespace ikos
