
#pragma once

#include <graphblas.hpp>

#include "detail/detail.hpp"
#include "util/util.hpp"

#include <concepts>
#include <memory>

namespace GRB_SPEC_NAMESPACE {

template <typename T,
          std::integral I = std::size_t,
          typename Hint = GRB_SPEC_NAMESPACE::sparse,
          typename Allocator = std::allocator<T>>
class matrix {
public:
  /// Type of scalar elements stored in the matrix.
  using scalar_type = T;

  /// Type used to reference the indices of elements stored in the matrix.
  using index_type = I;

  /// A tuple-like type containing:
  /// 1. A tuple-like type with two `index_type` elements storing the row and column
  ///    index of the element.
  /// 2. An element of type `scalar_type`.
  // using value_type = matrix_entry<T, I>;

  using key_type = index<I>;
  using map_type = T;

  /// Allocator type
  using allocator_type = Allocator;

  /// A large unsigned integral type
  using size_type = std::size_t;

  /// A large signed integral type
  using difference_type = std::ptrdiff_t;

  using hint_type = Hint;

  using backend_type = grb::Matrix<T, grb::config::default_backend, I, I>;

  /// Construct an empty matrix of dimension `shape[0]` x `shape[1]`
  matrix(index<I> shape) : backend_(shape[0], shape[1]) {}

  matrix(std::initializer_list<I> shape)
    : backend_(*shape.begin(), *(shape.begin()+1)) {}

  index<I> shape() const {
    return index<I>(grb::nrows(backend_), grb::ncols(backend_));
  }

  size_type size() const {
    return grb::nnz(backend_);
  }

/*
  template <class M>
  void insert_or_assign(key_type k, M&& obj) {
    // auto rv = grb::setElement(backend_, std::forward<M>(obj), k[0], k[1]);
  }
  */

  matrix() = default;
  matrix(const Allocator& allocator) {}

  matrix(const matrix&) = default;
  matrix(matrix&&) = default;
  matrix& operator=(const matrix&) = default;
  matrix& operator=(matrix&&) = default;

private:
  backend_type backend_;
};

};