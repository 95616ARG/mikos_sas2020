#pragma once

#include <ikos/core/domain/abstract_domain.hpp>
#include <ikos/core/semantic/graph.hpp>

namespace ikos {
namespace core {

/// \brief Base class for forward fixpoint iterators
template < typename GraphRef,
           typename AbstractValue,
           typename GraphTrait = GraphTraits< GraphRef > >
class MForwardFixpointIterator {
public:
  static_assert(IsGraph< GraphRef, GraphTrait >::value,
                "GraphRef does not implement GraphTraits");
  static_assert(IsAbstractDomain< AbstractValue >::value,
                "AbstractValue does not implement AbstractDomain");

public:
  /// \brief Reference to a node of the graph
  using NodeRef = typename GraphTrait::NodeRef;

public:
  /// \brief Default constructor
  MForwardFixpointIterator() = default;

  /// \brief Copy constructor
  MForwardFixpointIterator(const MForwardFixpointIterator&) noexcept = default;

  /// \brief Move constructor
  MForwardFixpointIterator(MForwardFixpointIterator&&) noexcept = default;

  /// \brief Copy assignment operator
  MForwardFixpointIterator& operator=(const MForwardFixpointIterator&) noexcept =
      default;

  /// \brief Move assignment operator
  MForwardFixpointIterator& operator=(MForwardFixpointIterator&&) noexcept =
      default;

  /// \brief Semantic transformer for a node
  ///
  /// This method is called with the abstract value representing the state
  /// of the program upon entering the node. The method should return an
  /// abstract value representing the state of the program after the node.
  virtual AbstractValue analyze_node(NodeRef node,
                                     AbstractValue state,
                                     bool defer_checks,
                                     bool cache_calls) = 0;

  /// \brief Run deferred checks for a component
  ///
  /// This method is called to run deferred checks in the component.
  virtual void run_deferred_checks_in_component(NodeRef head) = 0;

  /// \brief Erase values required for deferred checks for a loop header
  ///
  /// This method is called to erase values required for deferred checks.
  virtual void erase_values_cached_for_deferred_checks(NodeRef head) = 0;

  /// \brief Semantic transformer for an edge
  ///
  /// This method is called with the abstract value representing the state of
  /// the program after exiting the source node. The method should return an
  /// abstract value representing the state of the program after the edge, at
  /// the entry of the destination node.
  virtual AbstractValue analyze_edge(NodeRef src,
                                     NodeRef dest,
                                     AbstractValue state) = 0;

  /// \brief Process the computed abstract value for a node
  ///
  /// This method is called when the fixpoint is reached, and with the abstract
  /// value representing the state of the program upon entering the node.
  virtual void process_pre(NodeRef, const AbstractValue&) = 0;

  /// \brief Process the computed abstract value for a node
  ///
  /// This method is called when the fixpoint is reached, and with the abstract
  /// value representing the state of the program after the node.
  virtual void process_post(NodeRef, const AbstractValue&) = 0;

  /// \brief Destructor
  virtual ~MForwardFixpointIterator() = default;

}; // end class MForwardFixpointIterator

} // end namespace core
} // end namespace ikos
