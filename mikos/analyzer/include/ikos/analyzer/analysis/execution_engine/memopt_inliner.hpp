/*******************************************************************************
 *
 * \file
 * \brief Dynamic inlining call semantic
 *
 * Notices:
 *
 * Copyright (c) 2011-2019 United States Government as represented by the
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

#include <memory>

#include <boost/container/flat_map.hpp>

#include <llvm/ADT/DenseMap.h>

#include <ikos/ar/semantic/function.hpp>
#include <ikos/ar/verify/type.hpp>

#include <ikos/analyzer/analysis/execution_engine/engine.hpp>
#include <ikos/analyzer/analysis/execution_engine/numerical.hpp>
#include <ikos/analyzer/analysis/pointer/value.hpp>
#include <ikos/analyzer/util/demangle.hpp>
#include <ikos/analyzer/util/log.hpp>

namespace ikos {
namespace analyzer {
namespace memory {

/// \brief Inliner of function calls.
///
/// The inlining of a function is done dynamically by matching formal and actual
/// parameters and analyzing recursively the callee (FunctionFixpoint) and after
/// the callee returns by simulating call-by-ref and updating the return value
/// at the call site. The inlining also supports function pointers by resolving
/// first the set of possible callees and joining the results.
template < typename FunctionAnalyzer, typename AbstractDomain >
class InlineCallExecutionEngine final : public CallExecutionEngine {
public:
  using InlineCallExecutionEngineT =
      InlineCallExecutionEngine< FunctionAnalyzer, AbstractDomain >;
  using NumericalExecutionEngineT = NumericalExecutionEngine< AbstractDomain >;

private:
  /// \brief Map from callee function to FunctionAnalyzer
  using CalleeMap =
      boost::container::flat_map< ar::Function*,
                                  std::unique_ptr< FunctionAnalyzer > >;

  /// \brief Map from call statement to CalleeMap
  using CallMap = llvm::DenseMap< ar::CallBase*, CalleeMap >;

private:
  /// \brief Analysis context
  Context& _ctx;

  /// \brief Numerical execution engine
  NumericalExecutionEngineT& _engine;

  /// \brief Function analyzer of the caller
  FunctionAnalyzer& _caller;

  /// \brief Invariant at the end of the function
  AbstractDomain _exit_inv;

  /// \brief Return statement, or null
  ar::ReturnValue* _return_stmt;

  /// \brief Number of analyzed callees in the last call
  std::size_t _num_analyzed_callees;

  /// \brief Basic block that contains the current call
  ar::BasicBlock* _bb;

  /// \brief True to defer the checks in the callees
  bool _defer_checks;

public:
  /// \brief Constructor
  InlineCallExecutionEngine(Context& ctx,
                            NumericalExecutionEngineT& engine,
                            FunctionAnalyzer& caller,
                            AbstractDomain bottom)
      : _ctx(ctx),
        _engine(engine),
        _caller(caller),
        _exit_inv(std::move(bottom)),
        _return_stmt(nullptr),
        _num_analyzed_callees(0),
        _defer_checks(false) {}

  /// \brief Set _defer_checks
  void set_defer_checks(bool defer_checks) { this->_defer_checks = defer_checks; }

  /// \brief Set _bb
  void set_bb(ar::BasicBlock* bb) { this->_bb = bb; }

  /// \brief Return the exit invariant, or bottom
  AbstractDomain& exit_invariant() { return this->_exit_inv; }

  /// \brief Return the return statement, or null
  ar::ReturnValue* return_stmt() const { return this->_return_stmt; }

  std::size_t num_analyzed_callees() {
    auto ret = this->_num_analyzed_callees;
    this->_num_analyzed_callees = 0;
    return ret;
  }

  /// \brief Exit a function
  ///
  /// This is called whenever we reach the exit node (if there is one).
  ///
  /// Note that this is different from exec(ar::Returnvalue*) if there is
  /// exceptions. This can be used to catch the invariant, including pending
  /// exceptions.
  void exec_exit(ar::Function* fun) override {
    this->_engine.deallocate_local_variables(fun->local_variable_begin(),
                                             fun->local_variable_end());
    this->_exit_inv = this->_engine.inv();
  }

  /// \brief Execute a ReturnValue statement
  void exec(ar::ReturnValue* s) override {
    ikos_assert_msg(this->_return_stmt == nullptr || this->_return_stmt == s,
                    "input code has more than one return statement");
    this->_return_stmt = s;
  }

  /// \brief Execute a Call statement
  void exec(ar::Call* s) override {
    // Execute the call base statement
    this->exec(cast< ar::CallBase >(s));

    // Exceptions aren't caught, propagate them
    this->inv().merge_caught_in_propagated_exceptions();
  }

  /// \brief Execute an Invoke statement
  void exec(ar::Invoke* s) override {
    // Execute the call base statement
    this->exec(cast< ar::CallBase >(s));

    // Exceptions are caught.
    // Nothing to do here.
    // see NumericalExecutionEngine::exec_edge()
  }

private:
  /// \brief Return a non-const reference on the current invariant
  AbstractDomain& inv() { return this->_engine.inv(); }

  /// \brief Execute any call statement
  void exec(ar::CallBase* call) {
    if (this->inv().is_normal_flow_bottom()) {
      return;
    }

    //
    // Collect potential callees
    //
    auto callees = PointsToSet::bottom();
    ar::Value* called = call->called();

    if (isa< ar::UndefinedConstant >(called)) {
      // Call on undefined pointer: error
      this->inv().set_normal_flow_to_bottom();
      return;
    } else if (isa< ar::NullConstant >(called)) {
      // Call on null pointer: error
      this->inv().set_normal_flow_to_bottom();
      return;
    } else if (auto cst = dyn_cast< ar::FunctionPointerConstant >(called)) {
      callees = {_ctx.mem_factory->get_function(cst->function())};
    } else if (isa< ar::InlineAssemblyConstant >(called)) {
      // Call to assembly
      this->_engine.exec_unknown_extern_call(call);
      return;
    } else if (isa< ar::GlobalVariable >(called)) {
      // Call to global variable: error
      this->inv().set_normal_flow_to_bottom();
      return;
    } else if (isa< ar::LocalVariable >(called)) {
      // Call to local variable: error
      this->inv().set_normal_flow_to_bottom();
      return;
    } else if (auto ptr = dyn_cast< ar::InternalVariable >(called)) {
      // Indirect call through a function pointer
      Variable* ptr_var = _ctx.var_factory->get_internal(ptr);

      // Assert `ptr != null`
      this->inv().normal().nullity_assert_non_null(ptr_var);

      // Reduction between value and pointer analysis
      const PointerInfo* pointer_info = this->_engine.pointer_info();
      if (pointer_info != nullptr) {
        PointsToSet points_to = pointer_info->get(ptr_var).points_to();

        // Pointer analysis and value analysis can be inconsistent
        if (!points_to.is_bottom() && !points_to.is_top()) {
          this->inv().normal().pointer_refine(ptr_var, points_to);
        }
      }

      if (this->inv().is_normal_flow_bottom()) {
        return;
      }

      // Get the callees
      callees = this->inv().normal().pointer_to_points_to(ptr_var);
    } else {
      ikos_unreachable("unexpected called operand");
    }

    //
    // Check callees
    //
    ikos_assert(!callees.is_bottom());
    if (callees.is_empty()) {
      // Invalid pointer dereference
      this->inv().set_normal_flow_to_bottom();
      return;
    } else if (callees.is_top()) {
      // No points-to information
      // ASSUMPTION: the callee has no side effects.
      // Just set lhs and all actual parameters of pointer type to TOP.
      this->_engine.exec_unknown_extern_call(call);
      return;
    }

    //
    // Compute the post invariant
    //

    // By default, propagate the exception states
    AbstractDomain post = this->inv();
    post.set_normal_flow_to_bottom();

    // For each callee
    // If this include a recursive function call, inliner should detect it
    // before analyzing any callees.
    for (MemoryLocation* mem : callees) {
      if (!isa< FunctionMemoryLocation >(mem)) {
        // Not a call to a function memory location
        continue;
      }

      ar::Function* callee = cast< FunctionMemoryLocation >(mem)->function();

      if (!ar::TypeVerifier::is_valid_call(call, callee->type())) {
        // Ill-formed function call
        //
        // This could be because of an imprecision of the pointer analysis.
        continue;
      }

      if (callee->is_declaration()) {
        continue;
      }
      ikos_assert(callee->is_definition());

      if (this->_caller.function() == callee ||
          this->_caller.call_context()->contains(callee)) {
        // Recursive function call
        //
        // TODO(jnavas): we can be more precise by making top only lhs of
        // call_stmt, actual parameters of pointer type and any global variable
        // that might be touched by the recursive function.
        this->_engine.exec_unknown_intern_call(call);
        return;
      }
    }

    // For each callee
    for (MemoryLocation* mem : callees) {
      if (!isa< FunctionMemoryLocation >(mem)) {
        // Not a call to a function memory location
        continue;
      }

      ar::Function* callee = cast< FunctionMemoryLocation >(mem)->function();

      if (!ar::TypeVerifier::is_valid_call(call, callee->type())) {
        // Ill-formed function call
        //
        // This could be because of an imprecision of the pointer analysis.
        continue;
      }

      if (callee->is_declaration()) {
        // Call to an extern function
        //
        // ASSUMPTION: if this is a call to an extern non-intrinsic function,
        // treat it as a function call that has no side effects.
        NumericalExecutionEngineT engine(this->_engine.fork());
        engine.inv().ignore_exceptions();
        engine.exec_extern_call(call, callee);
        engine.inv().merge_propagated_in_caught_exceptions();
        post.join_with(engine.inv());
        continue;
      }
      ikos_assert(callee->is_definition());

      NumericalExecutionEngineT engine = this->_engine.fork();

      // Do not propagate exceptions from the caller to the callee
      engine.inv().ignore_exceptions();

      // Assign parameters
      engine.match_down(call, callee);

      //
      // Analyze recursively the callee
      //

      this->_num_analyzed_callees++;
      std::unique_ptr< FunctionAnalyzer > callee_analyzer = nullptr;

      // Create a fixpoint on the callee
      callee_analyzer = std::make_unique<
          FunctionAnalyzer >(_ctx,
                             _caller,
                             call,
                             callee,
                             this->_defer_checks);

      // Run analysis on callee
      log::debug("Analyzing function '" + demangle(callee->name()) + "'");
      callee_analyzer->run(engine.inv());

      // Return statement in the callee, or null
      ar::ReturnValue* return_stmt = callee_analyzer->return_stmt();

      engine.set_inv(callee_analyzer->exit_invariant());
      callee_analyzer->exit_invariant().set_to_bottom();
      // TODO(sungkook): deallocate the exit_invariant.

      if (this->_defer_checks && callee_analyzer->has_deferred_checks()) {
        this->_caller.append_to_callee_cache(this->_bb, callee_analyzer);
      } else {
        callee_analyzer.reset();
      }

      // Merge exceptions in caught_exceptions, in case it's an invoke
      engine.inv().merge_propagated_in_caught_exceptions();

      if (engine.inv().is_normal_flow_bottom()) {
        post.join_with(engine.inv()); // collect the exception states
        continue;
      }

      engine.match_up(call, return_stmt);
      post.join_with(engine.inv());
    }

    this->_engine.set_inv(std::move(post));
  }

}; // end class InlineCallExecutionEngine

} // end namespace memory
} // end namespace analyzer
} // end namespace ikos
