/*******************************************************************************
 *
 * \file
 * \brief Fixpoint on a function body
 *
 * Author: Maxime Arthaud
 *
 * Contact: ikos@lists.nasa.gov
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

#include <ikos/analyzer/analysis/pointer/pointer.hpp>
#include <ikos/analyzer/analysis/value/interprocedural/function_fixpoint.hpp>

namespace ikos {
namespace analyzer {
namespace value {
namespace interprocedural {

FunctionFixpoint::FunctionFixpoint(
    Context& ctx,
    const std::vector< std::unique_ptr< Checker > >& checkers,
    ProgressLogger& logger,
    ar::Function* entry_point)
    : FwdFixpointIterator(entry_point->body(), make_bottom_abstract_value(ctx)),
      _function(entry_point),
      _call_context(ctx.call_context_factory->get_empty()),
      _fixpoint_parameters(ctx.fixpoint_parameters->get(entry_point)),
      _checkers(checkers),
      _logger(logger),
      _exec_engine(make_bottom_abstract_value(ctx),
                   ctx,
                   this->_call_context,
                   ExecutionEngine::UpdateAllocSizeVar,
                   /* liveness = */ ctx.liveness,
                   /* pointer_info = */ ctx.pointer == nullptr
                       ? nullptr
                       : &ctx.pointer->results()),
      _call_exec_engine(ctx,
                        _exec_engine,
                        *this,
                        make_bottom_abstract_value(ctx),
                        /* context_stable = */ true,
                        /* convergence_achieved = */ false) {}

FunctionFixpoint::FunctionFixpoint(Context& ctx,
                                   const FunctionFixpoint& caller,
                                   ar::CallBase* call,
                                   ar::Function* callee,
                                   bool context_stable)
    : FwdFixpointIterator(callee->body(), make_bottom_abstract_value(ctx)),
      _function(callee),
      _call_context(
          ctx.call_context_factory->get_context(caller._call_context, call)),
      _fixpoint_parameters(ctx.fixpoint_parameters->get(callee)),
      _checkers(caller._checkers),
      _logger(caller._logger),
      _exec_engine(make_bottom_abstract_value(ctx),
                   ctx,
                   this->_call_context,
                   ExecutionEngine::UpdateAllocSizeVar,
                   /* liveness = */ ctx.liveness,
                   /* pointer_info = */ ctx.pointer == nullptr
                       ? nullptr
                       : &ctx.pointer->results()),
      _call_exec_engine(ctx,
                        _exec_engine,
                        *this,
                        make_bottom_abstract_value(ctx),
                        /* context_stable = */ context_stable,
                        /* convergence_achieved = */ false) {}

void FunctionFixpoint::run(AbstractDomain inv) {
  if (!this->_call_context->empty()) {
    this->_logger.start_callee(this->_call_context, this->_function);
  }

  // Compute the fixpoint
  FwdFixpointIterator::run(std::move(inv));

  // Fixpoint reached
  this->_call_exec_engine.mark_convergence_achieved();

  // Clear post invariants, save a lot of memory
  this->clear_post();

  if (!this->_call_context->empty()) {
    this->_logger.end_callee(this->_call_context, this->_function);
  }
}

AbstractDomain FunctionFixpoint::extrapolate(ar::BasicBlock* head,
                                             unsigned iteration,
                                             const AbstractDomain& before,
                                             const AbstractDomain& after) {
  if (iteration <= this->_fixpoint_parameters.widening_delay) {
    // Fixed number of iterations using join
    return before.join_iter(after);
  }

  iteration -= this->_fixpoint_parameters.widening_delay;
  iteration--;

  if (iteration % this->_fixpoint_parameters.widening_period != 0) {
    // Not the period, iteration using join
    return before.join_iter(after);
  }

  switch (this->_fixpoint_parameters.widening_strategy) {
    case WideningStrategy::Widen: {
      if (iteration == 0) {
        if (auto threshold =
                this->_fixpoint_parameters.widening_hints.get(head)) {
          // One iteration using widening with threshold
          return before.widening_threshold(after, *threshold);
        }
      }

      // Iterations using widening until convergence
      return before.widening(after);
    }
    case WideningStrategy::Join: {
      // Iterations using join until convergence
      return before.join_iter(after);
    }
    default: {
      ikos_unreachable("unexpected strategy");
    }
  }
}

AbstractDomain FunctionFixpoint::refine(ar::BasicBlock* head,
                                        unsigned iteration,
                                        const AbstractDomain& before,
                                        const AbstractDomain& after) {
  switch (this->_fixpoint_parameters.narrowing_strategy) {
    case NarrowingStrategy::Narrow: {
      if (iteration == 1) {
        if (auto threshold =
                this->_fixpoint_parameters.widening_hints.get(head)) {
          // First iteration using narrowing with threshold
          return before.narrowing_threshold(after, *threshold);
        }
      }

      // Iterations using narrowing
      return before.narrowing(after);
    }
    case NarrowingStrategy::Meet: {
      // Iterations using meet
      return before.meet(after);
    }
    default: {
      ikos_unreachable("unexpected strategy");
    }
  }
}

bool FunctionFixpoint::is_decreasing_iterations_fixpoint(
    ar::BasicBlock* /*head*/,
    unsigned iteration,
    const AbstractDomain& before,
    const AbstractDomain& after) {
  // Check if we reached the number of requested iterations, or convergence
  return (this->_fixpoint_parameters.narrowing_iterations &&
          iteration >= *this->_fixpoint_parameters.narrowing_iterations) ||
         before.leq(after);
}

AbstractDomain FunctionFixpoint::analyze_node(ar::BasicBlock* bb,
                                              AbstractDomain pre) {
  this->_exec_engine.set_inv(std::move(pre));
  this->_exec_engine.exec_enter(bb);
  for (ar::Statement* stmt : *bb) {
    transfer_function(this->_exec_engine, this->_call_exec_engine, stmt);
  }
  this->_exec_engine.exec_leave(bb);
  return std::move(this->_exec_engine.inv());
}

AbstractDomain FunctionFixpoint::analyze_edge(ar::BasicBlock* src,
                                              ar::BasicBlock* dest,
                                              AbstractDomain pre) {
  this->_exec_engine.set_inv(std::move(pre));
  this->_exec_engine.exec_edge(src, dest);
  return std::move(this->_exec_engine.inv());
}

void FunctionFixpoint::notify_enter_cycle(ar::BasicBlock* head) {
  this->_logger.start_cycle(head);
}

void FunctionFixpoint::notify_cycle_iteration(
    ar::BasicBlock* head,
    unsigned iteration,
    core::FixpointIterationKind kind) {
  this->_logger.start_cycle_iter(head, iteration, kind);
}

void FunctionFixpoint::notify_leave_cycle(ar::BasicBlock* head) {
  this->_logger.end_cycle(head);
}

void FunctionFixpoint::process_pre(ar::BasicBlock* /*bb*/,
                                   const AbstractDomain& /*pre*/) {}

void FunctionFixpoint::process_post(ar::BasicBlock* bb,
                                    const AbstractDomain& post) {
  if (this->_function->body()->exit_block_or_null() == bb) {
    this->_exec_engine.set_inv(post);
    this->_call_exec_engine.exec_exit(this->_function);
  }
}

void FunctionFixpoint::run_checks() {
  if (!this->_call_context->empty()) {
    this->_logger.start_callee(this->_call_context, this->_function);
  }

  // Check called functions during the transfer function
  this->_call_exec_engine.mark_check_callees();

  for (ar::BasicBlock* bb : *this->cfg()) {
    this->_exec_engine.set_inv(this->pre(bb));
    this->_exec_engine.exec_enter(bb);

    for (ar::Statement* stmt : *bb) {
      // Check the statement if it's related to an llvm instruction
      if (stmt->has_frontend()) {
        for (const auto& checker : this->_checkers) {
          checker->check(stmt, this->_exec_engine.inv(), this->_call_context);
        }
      }

      // Propagate
      transfer_function(this->_exec_engine, this->_call_exec_engine, stmt);
    }

    this->_exec_engine.exec_leave(bb);
  }

  if (!this->_call_context->empty()) {
    this->_logger.end_callee(this->_call_context, this->_function);
  }
}

} // end namespace interprocedural
} // end namespace value
} // end namespace analyzer
} // end namespace ikos
