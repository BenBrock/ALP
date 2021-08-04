
/*
 *   Copyright 2021 Huawei Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * @author A. N. Yzelman
 * @date 5th of December 2016
 */

#if ! defined _H_GRB_REFERENCE_BLAS1 || defined _H_GRB_REFERENCE_OMP_BLAS1
#define _H_GRB_REFERENCE_BLAS1

#include <iostream>    //for printing to stderr
#include <type_traits> //for std::enable_if

#include <graphblas/backends.hpp>
#include <graphblas/blas0.hpp>
#include <graphblas/descriptors.hpp>
#include <graphblas/internalops.hpp>
#include <graphblas/ops.hpp>
#include <graphblas/rc.hpp>
#include <graphblas/semiring.hpp>

#include "coordinates.hpp"
#include "vector.hpp"

#ifdef _H_GRB_REFERENCE_OMP_BLAS1
#include <omp.h>
#endif

#define NO_CAST_ASSERT( x, y, z )                                              \
	static_assert( x,                                                          \
		"\n\n"                                                                 \
		"********************************************************************" \
		"********************************************************************" \
		"******************************\n"                                     \
		"*     ERROR      | " y " " z ".\n"                                    \
		"********************************************************************" \
		"********************************************************************" \
		"******************************\n"                                     \
		"* Possible fix 1 | Remove no_casting from the template parameters "   \
		"in this call to " y ".\n"                                             \
		"* Possible fix 2 | Provide a value that matches the expected type.\n" \
		"********************************************************************" \
		"********************************************************************" \
		"******************************\n" );

#define NO_CAST_OP_ASSERT( x, y, z )                                           \
	static_assert( x,                                                          \
		"\n\n"                                                                 \
		"********************************************************************" \
		"********************************************************************" \
		"******************************\n"                                     \
		"*     ERROR      | " y " " z ".\n"                                    \
		"********************************************************************" \
		"********************************************************************" \
		"******************************\n"                                     \
		"* Possible fix 1 | Remove no_casting from the template parameters "   \
		"in this call to " y ".\n"                                             \
		"* Possible fix 2 | For all mismatches in the domains of input "       \
		"parameters and the operator domains, as specified in the "            \
		"documentation of the function " y ", supply an input argument of "    \
		"the expected type instead.\n"                                         \
		"* Possible fix 3 | Provide a compatible operator where all domains "  \
		"match those of the input parameters, as specified in the "            \
		"documentation of the function " y ".\n"                               \
		"********************************************************************" \
		"********************************************************************" \
		"******************************\n" );

namespace grb {

	namespace internal {

#ifndef _H_GRB_REFERENCE_OMP_BLAS1
		template< Descriptor descr, typename OutputType, typename IndexType, typename ValueType >
		OutputType
		setIndexOrValue( const IndexType & index, const ValueType & value, const typename std::enable_if< std::is_convertible< IndexType, OutputType >::value, void >::type * const = NULL ) {
			if( descr & grb::descriptors::use_index ) {
				return static_cast< OutputType >( index );
			} else {
				return static_cast< OutputType >( value );
			}
		}

		template< Descriptor descr, typename OutputType, typename IndexType, typename ValueType >
		OutputType
		setIndexOrValue( const IndexType & index, const ValueType & value, const typename std::enable_if< ! std::is_convertible< IndexType, OutputType >::value, void >::type * const = NULL ) {
			(void)index;
			static_assert( ! ( descr & grb::descriptors::use_index ),
				"use_index descriptor passed while the index type cannot be cast "
				"to the output type" );
			return static_cast< OutputType >( value );
		}
#endif

	} // namespace internal

	/**
	 * \defgroup BLAS1 The Level-1 Basic Linear Algebra Subroutines (BLAS)
	 *
	 * A collection of functions that allow GraphBLAS operators, monoids, and
	 * semirings work on a mix of zero-dimensional and one-dimensional containers;
	 * i.e., allows various linear algebra operations on scalars and objects of
	 * type grb::Vector.
	 *
	 * All functions except for grb::size and grb::nnz return an error code of
	 * the enum-type grb::RC. The two functions for retrieving the size and the
	 * nonzeroes of two vectors are excluded from this because they are never
	 * allowed to fail.
	 *
	 * Operations which require a single input vector only and produce scalar
	 * output:
	 *   -# grb::size,
	 *   -# grb::nnz, and
	 *   -# grb::set (three variants).
	 * These do not require an operator, monoid, nor semiring. The following
	 * require an operator:
	 *   -# grb::foldr (reduction to the right),
	 *   -# grb::foldl (reduction to the left).
	 * Operators can only be applied on \em dense vectors. Operations on sparse
	 * vectors require a well-defined way to handle missing vector elements. The
	 * following functions require a monoid instead of an operator and are able
	 * to handle sparse vectors by interpreting missing items as an identity
	 * value:
	 *   -# grb::reducer (reduction to the right),
	 *   -# grb::reducel (reduction to the left).
	 *
	 * Operations which require two input vectors and produce scalar output:
	 *   -# grb::dot   (dot product-- requires a semiring).
	 * Sparse vectors under a semiring have their missing values interpreted as a
	 * zero element under the given semiring; i.e., the identity of the additive
	 * operator.
	 *
	 * Operations which require one input vector and one input/output vector for
	 * full and efficient in-place operations:
	 *   -# grb::foldr (reduction to the right-- requires an operator),
	 *   -# grb::foldl (reduction to the left-- requires an operator).
	 * For grb::foldr, the left-hand side input vector may be replaced by an
	 * input scalar. For grb::foldl, the right-hand side input vector may be
	 * replaced by an input scalar. In either of those cases, the reduction
	 * is equivalent to an in-place vector scaling.
	 *
	 * Operations which require two input vectors and one output vector for
	 * out-of-place operations:
	 *   -# grb::eWiseApply (requires an operator),
	 *   -# grb::eWiseMul   (requires a semiring),
	 *   -# grb::eWiseAdd   (requires a semiring).
	 * Note that multiplication will consider any zero elements as an annihilator
	 * to the multiplicative operator. Therefore, the operator will only be
	 * applied at vector indices where both input vectors have nonzeroes. This is
	 * different from eWiseAdd. This difference only manifests itself when dealing
	 * with semirings, and reflects the intuitively expected behaviour. Any of the
	 * two input vectors (or both) may be replaced with an input scalar instead.
	 *
	 * Operations which require three input vectors and one output vector for
	 * out-of-place operations:
	 *   -# grb::eWiseMulAdd (requires a semiring).
	 * This function can be emulated by first successive calls to grb::eWiseMul
	 * and grb::eWiseAdd. This specialised function, however, has better
	 * performance semantics. This function is closest to the standard axpy
	 * BLAS1 call, with out-of-place semantics. The first input vector may be
	 * replaced by a scalar.
	 *
	 * Again, each of grb::eWiseMul, grb::eWiseAdd, grb::eWiseMulAdd accept sparse
	 * vectors as input and output (since they operate on semirings), while
	 * grb::eWiseApply.
	 *
	 * For fusing multiple BLAS-1 style operations on any number of inputs and
	 * outputs, users can pass their own operator function to be executed for
	 * every index \a i.
	 *   -# grb::eWiseLambda.
	 * This requires manual application of operators, monoids, and/or semirings
	 * via the BLAS-0 interface (see grb::apply, grb::foldl, and grb::foldr).
	 *
	 * For all of these functions, the element types of input and output types
	 * do not have to match the domains of the given operator, monoid, or
	 * semiring unless the grb::descriptors::no_casting descriptor was passed.
	 *
	 * An implementation, whether blocking or non-blocking, should have clear
	 * performance semantics for every sequence of graphBLAS calls, no matter
	 * whether those are made from sequential or parallel contexts.
	 *
	 * @{
	 */

	/**
	 * Clears all elements from the given vector \a x.
	 *
	 * At the end of this operation, the number of nonzero elements in this vector
	 * will be zero. The size of the vector remains unchanged.
	 *
	 * @return grb::SUCCESS When the vector is successfully cleared.
	 *
	 * \note This function cannot fail.
	 *
	 * \parblock
	 * \par Performance guarantees
	 *      This function
	 *        -# contains \f$ \mathcal{O}(n) \f$ work,
	 *        -# will not allocate new dynamic memory,
	 *        -# will take at most \f$ \Theta(1) \f$ memory beyond the memory
	 *           already used by the application before the call to this
	 *           function.
	 *        -# will move at most \f$ \mathit{sizeof}(\mathit{bool}) +
	 *           \mathit{sizeof}(\mathit{size\_t}) \f$ bytes of data.
	 * \endparblock
	 */
	template< typename DataType, typename Coords >
	RC clear( Vector< DataType, reference, Coords > & x ) noexcept {
		internal::getCoordinates( x ).clear();
		return SUCCESS;
	}

	/**
	 * Request the size (dimension) of a given vector.
	 *
	 * The dimension is set at construction of the given vector and cannot be
	 * changed. A call to this function shall always succeed.
	 *
	 * @tparam DataType The type of elements contained in the vector \a x.
	 *
	 * @param[in] x The vector of which to retrieve the size.
	 *
	 * @return The size of the vector \a x.
	 *
	 * \parblock
	 * \par Performance guarantees
	 * A call to this function
	 *  -# consists of \f$ \Theta(1) \f$ work;
	 *  -# moves \f$ \Theta(1) \f$ bytes of memory;
	 *  -# does not allocate any dynamic memory;
	 *  -# shall not make any system calls.
	 * \endparblock
	 */
	template< typename DataType, typename Coords >
	size_t size( const Vector< DataType, reference, Coords > & x ) noexcept {
		return internal::getCoordinates( x ).size();
	}

	/**
	 * Request the number of nonzeroes in a given vector.
	 *
	 * A call to this function always succeeds.
	 *
	 * @tparam DataType The type of elements contained in this vector.
	 *
	 * @param[in] x The vector of which to retrieve the number of nonzeroes.
	 *
	 * @return The number of nonzeroes in \a x.
	 *
	 * \parblock
	 * \par Performance guarantees
	 * A call to this function
	 *   -# consists of \f$ \Theta(1) \f$ work;
	 *   -# moves \f$ \Theta(1) \f$ bytes of memory;
	 *   -# does not allocate nor free any dynamic memory;
	 *   -# shall not make any system calls.
	 * \endparblock
	 */
	template< typename DataType, typename Coords >
	size_t nnz( const Vector< DataType, reference, Coords > & x ) noexcept {
		return internal::getCoordinates( x ).nonzeroes();
	}

	/** \todo add documentation. In particular, think about the meaning with \a P > 1. */
	template< typename InputType, typename length_type, typename Coords >
	RC resize( Vector< InputType, reference, Coords > & x, const length_type new_nz ) {
		// check if we have a mismatch
		if( new_nz > grb::size( x ) ) {
			return MISMATCH;
		}
		// in the reference implementation, vectors are of static size
		// so this function immediately succeeds
		return SUCCESS;
	}

	/**
	 * Sets all elements of a vector to the given value. This makes the given
	 * vector completely dense.
	 *
	 * This code is functionally equivalent to both
	 * \code
	 * grb::operators::right_assign< DataType > op;
	 * return foldl< descr >( x, val, op );
	 * \endcode
	 * and
	 * \code
	 * grb::operators::left_assign< DataType > op;
	 * return foldr< descr >( val, x, op );
	 * \endcode
	 *
	 * Their performance semantics also match.
	 *
	 * @tparam descr    The descriptor used for this operation.
	 * @tparam DataType The type of each element in the given vector.
	 * @tparam T        The type of the given value.
	 *
	 * @param[in,out] x The vector of which every element is to be set to equal
	 *                  \a val. If the capacity of this vector is insufficient to
	 *                  hold \a n values, where \a n is the size of this vector,
	 *                  then the below performance guarantees will not be met.
	 * @param[in]   val The value to set each element of \a x equal to.
	 *
	 * @returns SUCCESS       When the call completes successfully.
	 * @returns OUT_OF_MEMORY If the capacity of \a x was insufficient to hold a
	 *                        completely dense vector and not enough memory could
	 *                        be allocated to remedy this. When this error code
	 *                        is returned, the state of the program shall be as
	 *                        though the call to this function had never occurred.
	 *
	 * \parblock
	 * \par Accepted descriptors
	 *   -# grb::descriptors::no_operation
	 *   -# grb::descriptors::no_casting
	 * \endparblock
	 *
	 * When \a descr includes grb::descriptors::no_casting and if \a T does not
	 * match \a DataType, the code shall not compile.
	 *
	 * \parblock
	 * \par Performance guarantees
	 * A call to this function
	 *   -# consists of \f$ \Theta(n) \f$ work;
	 *   -# moves \f$ \Theta(n) \f$ bytes of memory;
	 *   -# does not allocate nor free any dynamic memory;
	 *   -# shall not make any system calls.
	 * \endparblock
	 *
	 * \warning If the capacity of \a x was insufficient to store a dense vector
	 *          then a call to this function may make the appropriate system calls
	 *          to allocate \f$ \Theta( n \mathit{sizeof}(DataType) ) \f$ bytes of
	 *          memory.
	 *
	 * @see grb::foldl.
	 * @see grb::foldr.
	 * @see grb::operators::left_assign.
	 * @see grb::operators::right_assign.
	 */
	template< Descriptor descr = descriptors::no_operation, typename DataType, typename T, typename Coords >
	RC set( Vector< DataType, reference, Coords > & x, const T val, const typename std::enable_if< ! grb::is_object< DataType >::value && ! grb::is_object< T >::value, void >::type * const = NULL ) {
		// static sanity checks
		NO_CAST_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< DataType, T >::value ), "grb::set (Vector, unmasked)",
			"called with a value type that does not match that of the given "
			"vector" );

		// pre-cast value to be copied
		const DataType toCopy = static_cast< DataType >( val );

		// make vector dense if it was not already
		internal::getCoordinates( x ).assignAll();
		DataType * const raw = internal::getRaw( x );
		const size_t n = internal::getCoordinates( x ).size();
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
		// do set in parallel
		{
			if( descr & descriptors::use_index ) {
				#pragma omp parallel for schedule( static )
				for( size_t i = 0; i < n; ++i ) {
					raw[ i ] = static_cast< DataType >( i );
				}
			} else {
				#pragma omp parallel for schedule( static )
				for( size_t i = 0; i < n; ++i ) {
					raw[ i ] = toCopy;
				}
			}
		}
#else
		if( descr & descriptors::use_index ) {
			for( size_t i = 0; i < n; ++i ) {
				raw[ i ] = static_cast< DataType >( i );
			}
		} else {
			for( size_t i = 0; i < n; ++i ) {
				raw[ i ] = toCopy;
			}
		}
#endif
		// sanity check
		assert( internal::getCoordinates( x ).nonzeroes() == internal::getCoordinates( x ).size() );

		// done
		return SUCCESS;
	}

	/**
	 * Set vector to value. Masked version.
	 */
	template< Descriptor descr = descriptors::no_operation, typename DataType, typename MaskType, typename T, typename Coords >
	RC set( Vector< DataType, reference, Coords > & x,
		const Vector< MaskType, reference, Coords > & m,
		const T val,
		const typename std::enable_if< ! grb::is_object< DataType >::value && ! grb::is_object< T >::value, void >::type * const = NULL ) {
		// static sanity checks
		NO_CAST_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< DataType, T >::value ), "grb::set (Vector to scalar, masked)",
			"called with a value type that does not match that of the given "
			"vector" );

		// catch empty mask
		if( size( m ) == 0 ) {
			return set< descr >( x, val );
		}

		// dynamic sanity checks
		if( size( x ) != size( m ) ) {
			return MISMATCH;
		}

		// pre-cast value to be copied
		const DataType toCopy = static_cast< DataType >( val );

		// make vector dense if it was not already
		DataType * const raw = internal::getRaw( x );
		auto & coors = internal::getCoordinates( x );
		const auto & m_coors = internal::getCoordinates( m );
		auto m_p = internal::getRaw( m );
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
		#pragma omp parallel
		{
			auto localUpdate = coors.EMPTY_UPDATE();
			const size_t maxAsyncAssigns = coors.maxAsyncAssigns();
			size_t asyncAssigns = 0;
#endif
			const size_t n = ( descr & descriptors::invert_mask ) ? coors.size() : m_coors.nonzeroes();
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
			#pragma omp for schedule(dynamic,config::CACHE_LINE_SIZE::value()) nowait
#endif
			for( size_t i = 0; i < n; ++i ) {
				const size_t index = ( descr & descriptors::invert_mask ) ? i : m_coors.index( i );
				if( ! m_coors.template mask< descr >( index, m_p ) ) {
					continue;
				}
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
				if( ! coors.asyncAssign( index, localUpdate ) ) {
					(void)++asyncAssigns;
				}
				if( asyncAssigns == maxAsyncAssigns ) {
					(void)coors.joinUpdate( localUpdate );
					asyncAssigns = 0;
				}
#else
			(void)coors.assign( index );
#endif
				if( descr & descriptors::use_index ) {
					raw[ index ] = static_cast< DataType >( index );
				} else {
					raw[ index ] = toCopy;
				}
			}
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
			while( ! coors.joinUpdate( localUpdate ) ) {}
		} // end pragma omp parallel
#endif
		// done
		return SUCCESS;
	}

	/**
	 * Sets the element of a given vector at a given position to a given value.
	 *
	 * If the input vector \a x already has an element \f$ x_i \f$, that element
	 * is overwritten to the given value \a val. If no such element existed, it
	 * is added and set equal to \a val. The number of nonzeroes in \a x may thus
	 * be increased by one due to a call to this function.
	 *
	 * The parameter \a i may not be greater or equal than the size of \a x.
	 *
	 * @tparam descr    The descriptor to be used during evaluation of this
	 *                  function.
	 * @tparam DataType The type of the elements of \a x.
	 * @tparam T        The type of the value to be set.
	 *
	 * @param[in,out] x The vector to be modified.
	 * @param[in]   val The value \f$ x_i \f$ should read after function exit.
	 * @param[in]     i The index of the element of \a x to set.
	 *
	 * @return grb::SUCCESS   Upon successful execution of this operation.
	 * @return grb::MISMATCH  If \a i is greater or equal than the dimension of
	 *                        \a x.
	 * @returns OUT_OF_MEMORY If the capacity of \a x was insufficient to add the
	 *                        new value \a val at index \a i, \em and not enough
	 *                        memory could be allocated to remedy this. When this
	 *                        error code is returned, the state of the program
	 *                        shall be as though the call to this function had
	 *                        never occurred.
	 *
	 * \parblock
	 * \par Accepted descriptors
	 *   -# grb::descriptors::no_operation
	 *   -# grb::descriptors::no_casting
	 * \endparblock
	 *
	 * When \a descr includes grb::descriptors::no_casting and if \a T does not
	 * match \a DataType, the code shall not compile.
	 *
	 * \parblock
	 * \par Performance guarantees
	 * A call to this function
	 *   -# consists of \f$ \Theta(1) \f$ work;
	 *   -# moves \f$ \Theta(1) \f$ bytes of memory;
	 *   -# does not allocate nor free any dynamic memory;
	 *   -# shall not make any system calls.
	 * \endparblock
	 *
	 * \warning If the capacity of \a x was insufficient to store a dense vector
	 *          then a call to this function may make the appropriate system calls
	 *          to allocate \f$ \Theta( n \mathit{sizeof}(DataType) ) \f$ bytes of
	 *          memory, where \a n is the new size of the vector \a x. This will
	 *          cause additional memory movement and work complexity as well.
	 */
	template< Descriptor descr = descriptors::no_operation, typename DataType, typename T, typename Coords >
	RC setElement( Vector< DataType, reference, Coords > & x,
		const T val,
		const size_t i,
		const typename std::enable_if< ! grb::is_object< DataType >::value && ! grb::is_object< T >::value, void >::type * const = NULL ) {
		// static sanity checks
		NO_CAST_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< DataType, T >::value ), "grb::set (Vector, at index)",
			"called with a value type that does not match that of the given "
			"vector" );

		// dynamic sanity checks
		if( i >= internal::getCoordinates( x ).size() ) {
			return MISMATCH;
		}

		// do set
		(void)internal::getCoordinates( x ).assign( i );
		internal::getRaw( x )[ i ] = static_cast< DataType >( val );

#ifdef _DEBUG
		std::cout << "setElement (reference) set index " << i << " to value " << internal::getRaw( x )[ i ] << "\n";
#endif

		// done
		return SUCCESS;
	}

	/**
	 * Sets the content of a given vector \a x to be equal to that of
	 * another given vector \a y.
	 *
	 * The vector \a x may not equal \a y or undefined behaviour will occur.
	 *
	 * \parblock
	 * \par Accepted descriptors
	 *   -# grb::descriptors::no_operation
	 *   -# grb::descriptors::no_casting
	 * \endparblock
	 *
	 * When \a descr includes grb::descriptors::no_casting and if \a InputType
	 * does not match \a OutputType, the code shall not compile.
	 *
	 * \parblock
	 * \par Performance guarantees
	 * A call to this function
	 *   -# consists of \f$ \mathcal{O}(n) \f$ work;
	 *   -# moves \f$ \mathcal{O}(n) \f$ bytes of memory;
	 *   -# does not allocate nor free any dynamic memory;
	 *   -# shall not make any system calls.
	 *
	 * \note The use of big-Oh instead of big-Theta is intentional.
	 *       Implementations that chose to emulate sparse vectors using dense
	 *       storage are allowed, but clearly better performance can be attained.
	 * \endparblock
	 *
	 * \warning If the capacity of \a x was insufficient to store a dense vector
	 *          then a call to this function may make the appropriate system calls
	 *          to allocate \f$ \Theta( n \mathit{sizeof}(DataType) ) \f$ bytes of
	 *          memory.
	 *
	 * \todo This documentation is to be extended.
	 */
	template< Descriptor descr = descriptors::no_operation, typename OutputType, typename InputType, typename Coords >
	RC set( Vector< OutputType, reference, Coords > & x, const Vector< InputType, reference, Coords > & y ) {
		// static sanity checks
		NO_CAST_ASSERT(
			( ! ( descr & descriptors::no_casting ) || std::is_same< OutputType, InputType >::value ), "grb::copy (Vector)", "called with vector parameters whose element data types do not match" );
		constexpr bool out_is_void = std::is_void< OutputType >::value;
		constexpr bool in_is_void = std::is_void< OutputType >::value;
		static_assert( ! in_is_void || out_is_void,
			"grb::set (reference, vector <- vector, masked): "
			"if input is void, then the output must be also" );
		static_assert( ! ( descr & descriptors::use_index ) || ! out_is_void,
			"grb::set (reference, vector <- vector, masked): "
			"use_index descriptor cannot be set if output vector is void" );

		// check contract
		if( reinterpret_cast< void * >( &x ) == reinterpret_cast< const void * >( &y ) ) {
			return ILLEGAL;
		}

		// get length
		const size_t n = internal::getCoordinates( y ).size();

		// get raw value arrays
		OutputType * __restrict__ const dst = internal::getRaw( x );
		const InputType * __restrict__ const src = internal::getRaw( y );

		// dynamic sanity checks
		if( n != internal::getCoordinates( x ).size() ) {
			return MISMATCH;
		}

		// catch boundary case
		if( n == 0 ) {
			return SUCCESS;
		}

		// get #nonzeroes
		const size_t nz = internal::getCoordinates( y ).nonzeroes();
#ifdef _DEBUG
		std::cout << "grb::set called with source vector containing " << nz << " nonzeroes." << std::endl;
#endif

		// first copy contents
		if( src == NULL && dst == NULL ) {
			// then source is a pattern vector, just copy its pattern
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
			#pragma omp parallel for schedule( dynamic, config::CACHE_LINE_SIZE::value() )
#endif
			for( size_t i = 0; i < nz; ++i ) {
				(void)internal::getCoordinates( x ).asyncCopy( internal::getCoordinates( y ), i );
			}
		} else {
#ifndef NDEBUG
			if( src == NULL ) {
				assert( dst == NULL );
			}
#endif
			// otherwise, the regular copy variant:
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
			#pragma omp parallel for schedule( static, config::CACHE_LINE_SIZE::value() )
#endif
			for( size_t i = 0; i < nz; ++i ) {
				const auto index = internal::getCoordinates( x ).asyncCopy( internal::getCoordinates( y ), i );
				if( ! out_is_void && ! in_is_void ) {
					dst[ index ] = internal::setIndexOrValue< descr, OutputType >( index, src[ index ] );
				}
			}
		}

		// set number of nonzeroes
		internal::getCoordinates( x ).joinCopy( internal::getCoordinates( y ) );

		// done
		return SUCCESS;
	}

	/**
	 * Masked variant of grb::set (vector copy).
	 *
	 * @see grb::set
	 */
	template< Descriptor descr = descriptors::no_operation, typename OutputType, typename MaskType, typename InputType, typename Coords >
	RC set( Vector< OutputType, reference, Coords > & x,
		const Vector< MaskType, reference, Coords > & mask,
		const Vector< InputType, reference, Coords > & y,
		const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< MaskType >::value && ! grb::is_object< InputType >::value, void >::type * const = NULL ) {
		// static sanity checks
		NO_CAST_ASSERT(
			( ! ( descr & descriptors::no_casting ) || std::is_same< OutputType, InputType >::value ), "grb::set (Vector)", "called with vector parameters whose element data types do not match" );
		NO_CAST_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< MaskType, bool >::value ), "grb::set (Vector)", "called with non-bool mask element types" );
		constexpr bool out_is_void = std::is_void< OutputType >::value;
		constexpr bool in_is_void = std::is_void< OutputType >::value;
		static_assert( ! in_is_void || out_is_void,
			"grb::set (reference, vector <- vector, masked): "
			"if input is void, then the output must be also" );
		static_assert( ! ( descr & descriptors::use_index ) || ! out_is_void,
			"grb::set (reference, vector <- vector, masked): "
			"use_index descriptor cannot be set if output vector is void" );

		// delegate if possible
		if( internal::getCoordinates( mask ).size() == 0 ) {
			return set( x, y );
		}

		// catch contract violations
		if( reinterpret_cast< void * >( &x ) == reinterpret_cast< const void * >( &y ) ) {
			return ILLEGAL;
		}

		// get relevant descriptors
		constexpr const bool use_index = descr & descriptors::use_index;

		// get length
		const size_t n = internal::getCoordinates( y ).size();

		// dynamic sanity checks
		if( n != internal::getCoordinates( x ).size() ) {
			return MISMATCH;
		}
		if( internal::getCoordinates( mask ).size() != n ) {
			return MISMATCH;
		}

		// catch trivial case
		if( n == 0 ) {
			return SUCCESS;
		}

		// return code
		RC ret = SUCCESS;

		// handle non-trivial, fully masked vector copy
		const auto & m_coors = internal::getCoordinates( mask );
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
		// keeps track of updates of the sparsity pattern
		#pragma omp parallel
		{
			// keeps track of nonzeroes that the mask ignores
			internal::Coordinates< reference >::Update local_update = internal::getCoordinates( x ).EMPTY_UPDATE();
			const size_t maxAsyncAssigns = internal::getCoordinates( x ).maxAsyncAssigns();
			size_t asyncAssigns = 0;
			RC local_rc = SUCCESS;
			#pragma omp for schedule( dynamic, config::CACHE_LINE_SIZE::value() ) nowait
			for( size_t i = 0; i < internal::getCoordinates( y ).size(); ++i ) {
				// if not masked, continue
				if( ! m_coors.template mask< descr >( i, internal::getRaw( mask ) ) ) {
					continue;
				}
				// if source has nonzero
				if( internal::getCoordinates( y ).assigned( i ) ) {
					// get value
					if( ! out_is_void && ! in_is_void ) {
						const InputType value = use_index ? static_cast< InputType >( i ) : internal::getRaw( y )[ i ];
						internal::getRaw( x )[ i ] = value;
					}
					// check if destination has nonzero
					if( ! internal::getCoordinates( x ).asyncAssign( i, local_update ) ) {
						(void)++asyncAssigns;
					}
				}
				if( asyncAssigns == maxAsyncAssigns ) {
					const bool was_empty = internal::getCoordinates( x ).joinUpdate( local_update );
#ifdef NDEBUG
					(void)was_empty;
#else
					assert( ! was_empty );
#endif
					asyncAssigns = 0;
				}
			}
			while( ! internal::getCoordinates( x ).joinUpdate( local_update ) ) {}
			if( local_rc != SUCCESS ) {
				ret = local_rc;
			}
		}
#else
		for( size_t i = 0; ret == SUCCESS && i < internal::getCoordinates( y ).size(); ++i ) {
			if( ! m_coors.template mask< descr >( i, internal::getRaw( mask ) ) ) {
				continue;
			}
			if( internal::getCoordinates( y ).assigned( i ) ) {
				if( ! out_is_void && ! in_is_void ) {
					// get value
					const InputType value = use_index ? static_cast< InputType >( i ) : internal::getRaw( y )[ i ];
					(void)internal::getCoordinates( x ).assign( i );
					internal::getRaw( x )[ i ] = value;
				}
			}
		}
#endif
		// done
		return ret;
	}

	namespace internal {

		template< Descriptor descr = descriptors::no_operation,
			bool masked,
			bool left, // if this is false, the right-looking fold is assumed
			class OP,
			typename IOType,
			typename InputType,
			typename MaskType,
			typename Coords >
		RC fold_from_vector_to_scalar_generic( IOType & fold_into, const Vector< InputType, reference, Coords > & to_fold, const Vector< MaskType, reference, Coords > & mask, const OP & op = OP() ) {
			// static sanity checks
			static_assert( grb::is_associative< OP >::value,
				"grb::foldl can only be called on associate operators. This "
				"function should not have been called-- please submit a "
				"bugreport." );

			// fold is only defined on dense vectors
			if( nnz( to_fold ) < size( to_fold ) ) {
				return ILLEGAL;
			}

			// mask must be of equal size as input vector
			if( masked && size( to_fold ) != size( mask ) ) {
				return MISMATCH;
			}

			// handle trivial case
			if( masked && nnz( mask ) == 0 ) {
				return SUCCESS;
			}

			// some globals used during the folding
			RC ret = SUCCESS;         // final return code
			IOType global = IOType(); // global variable in which to fold
			size_t root = 0;          // which process is the root of the fold (in case we have multiple processes)
#ifndef _H_GRB_REFERENCE_OMP_BLAS1
			// handle trivial sequential cases
			if( ! masked ) {
				// this op is only defined on dense vectors, check this is the case
				assert( internal::getCoordinates( to_fold ).nonzeroes() == internal::getCoordinates( to_fold ).size() );
				// no mask, vectors are dense, sequential execution-- so rely on underlying operator
				if( left ) {
					global = internal::getRaw( to_fold )[ 0 ];
					op.foldlArray( global, internal::getRaw( to_fold ) + 1, internal::getCoordinates( to_fold ).size() - 1 );
				} else {
					global = internal::getRaw( to_fold )[ internal::getCoordinates( to_fold ).size() - 1 ];
					op.foldrArray( internal::getRaw( to_fold ), global, internal::getCoordinates( to_fold ).size() - 1 );
				}
			} else {
				// masked sequential case
				const size_t n = internal::getCoordinates( to_fold ).size();
				constexpr size_t s = 0;
				constexpr size_t P = 1;
				size_t i = 0;
				const size_t end = internal::getCoordinates( to_fold ).size();
#else
			{
				#pragma omp parallel
				{
					// parallel case (masked & unmasked)
					const size_t n = internal::getCoordinates( to_fold ).size();
					const size_t s = omp_get_thread_num();
					const size_t P = omp_get_num_threads();
					assert( s < P );
					const size_t blocksize = n / P + ( ( n % P ) > 0 ? 1 : 0 );
					size_t i = s * blocksize > n ? n : s * blocksize;
					const size_t end = ( s + 1 ) * blocksize > n ? n : ( s + 1 ) * blocksize;

					// DBG
					//#pragma omp critical
					// std::cout << "DBG " << s << ": start = " << i << ", end = " << end << ". Descriptor inversion set = " << (descr & descriptors::invert_mask) << ".\n";

					#pragma omp single
					{ root = P; }
					#pragma omp barrier
#endif
				// some sanity checks
				assert( i <= end );
				assert( end <= n );
#ifdef NDEBUG
				(void)n;
#endif
				// assume current i needs to be processed
				bool process_current_i = true;
				// i is at relative position -1. We keep forwarding until we find an index we should process
				//(or until we hit the end of our block)
				if( masked && i < end ) {

					// DBG
					//#pragma omp critical
					// std::cout << "DBG considering start at ( " << i << ", " << internal::getRaw(mask)[i] << ", " << utils::interpretMask< descr >( internal::getCoordinates(mask).assigned( i ),
					// internal::getRaw(mask), i ) << " )\n";

					// check if we need to process current i
					process_current_i = utils::interpretMask< descr >( internal::getCoordinates( mask ).assigned( i ), internal::getRaw( mask ), i );
					// if not
					while( ! process_current_i ) {
						// forward to next element
						++i;
						// check that we are within bounds
						if( i == end ) {
							break;
						}
						// evaluate whether we should process this i-th element
						process_current_i = utils::interpretMask< descr >( internal::getCoordinates( mask ).assigned( i ), internal::getRaw( mask ), i );
					}

					// DBG
					//#pragma omp critical
					// std::cout << "DBG starting at ( " << i << ", " << internal::getRaw(mask)[i] << ", " << utils::interpretMask< descr >( internal::getCoordinates(mask).assigned( i ),
					// internal::getRaw(mask), i ) << " )\n";
				}

				// whether we have any nonzeroes assigned at all
				const bool empty = i >= end;

#ifndef _H_GRB_REFERENCE_OMP_BLAS1
				// in the sequential case, the empty case should have been handled earlier
				assert( ! empty );
#else
					// select root
					#pragma omp critical
#endif
				{
					// check if we have a root already
					if( ! empty && root == P ) {
						// no, so take it
						root = s;
					}
				}
				// declare thread-local variable and set our variable to the first value in our block
				IOType local = i < end ? static_cast< IOType >( internal::getRaw( to_fold )[ i ] ) : static_cast< IOType >( internal::getRaw( to_fold )[ 0 ] );
				// if we have a value to fold
				if( ! empty ) {
					// DBG
					//#pragma omp critical
					// std::cout << "DBG start ( " << i << ", " << internal::getRaw(to_fold)[ i ] << ", " << internal::getRaw(mask)[i] << ", " << utils::interpretMask< descr >(
					// internal::getCoordinates(mask).assigned( i ), internal::getRaw(mask), i ) << " )\n"; loop over all remaining values, if any
					while( true ) {
						// forward to next variable
						++i;
						// forward more (possibly) if in the masked case
						if( masked ) {
							process_current_i = utils::interpretMask< descr >( internal::getCoordinates( mask ).assigned( i ), internal::getRaw( mask ), i );
							while( ! process_current_i && i + 1 < end ) {
								++i;
								process_current_i = utils::interpretMask< descr >( internal::getCoordinates( mask ).assigned( i ), internal::getRaw( mask ), i );
							}
						}
						// stop if past end
						if( i >= end || ! process_current_i ) {
							break;
						}
						// store result of fold in local variable
						RC rc;

						// DBG
						/*#pragma omp critical
						std::cout << "DBG ( " << i << ", " << internal::getRaw(to_fold)[ i ] << ", " << internal::getRaw(mask)[ i ] << ", " << utils::interpretMask< descr >(
						internal::getCoordinates(mask).assigned( i ), internal::getRaw(mask), i ) << ", local = " << local << " )\n";*/

						if( left ) {
							rc = foldl< descr >( local, internal::getRaw( to_fold )[ i ], op );
						} else {
							rc = foldr< descr >( internal::getRaw( to_fold )[ i ], local, op );
						}
						// sanity check
						assert( rc == SUCCESS );
						// error propagation
						if( rc != SUCCESS ) {
							ret = rc;
							break;
						}
					}
				}
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
				#pragma omp critical
				{
					// if I am root
					if( root == s ) {
						// then I should be non-empty
						assert( ! empty );
						// set global value to locally computed value
						global = local;

						// DBG
						// std::cout << "Root process " << s << " wrote global = " << global << "\n";
					}
				}
				#pragma omp barrier
				#pragma omp critical
				{
					// if non-root, fold local variable into global one
					if( ! empty && root != s ) {
						RC rc;
						if( left ) {
							rc = foldl< descr >( global, local, op );
						} else {
							rc = foldr< descr >( local, global, op );
						}
						assert( rc == SUCCESS );
						if( rc != SUCCESS ) {
							ret = rc;
						}

						// DBG
						// std::cout << "Process " << s << " added = " << local << " and thus set global to " << global << "\n";
					}
				}
			} // end pragma omp parallel for
#endif
		}
#ifdef _DEBUG
		std::cout << "Accumulating " << global << " into " << fold_into << " using foldl\n";
#endif
		// accumulate
		if( ret == SUCCESS ) {
			ret = foldl< descr >( fold_into, global, op );
		}

		// done
		return ret;
	}

	/**
	 * \internal Only applies to sparse vectors and non-monoid folding.
	 */
	template< Descriptor descr, bool left, bool masked, typename IOType, typename MaskType, typename Coords, typename InputType, class OP >
	RC fold_from_scalar_to_vector_generic_vectorDriven( Vector< IOType, reference, Coords > & vector,
		const MaskType * __restrict__ m,
		const Coords * const m_coors,
		const InputType & scalar,
		const OP & op ) {
		assert( ! masked || ( m_coors != NULL ) );
		IOType * __restrict__ x = internal::getRaw( vector );
		Coords & coors = internal::getCoordinates( vector );
		assert( coors.nonzeroes() < coors.size() );
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
		#pragma omp parallel for schedule( dynamic, config::CACHE_LINE_SIZE::value() )
#endif
		for( size_t i = 0; i < coors.nonzeroes(); ++i ) {
			const size_t index = coors.index( i );
			if( masked ) {
				if( ! ( m_coors->template mask< descr >( index, m ) ) ) {
					continue;
				}
			}
			if( left ) {
				(void)foldl< descr >( x[ index ], scalar, op );
			} else {
				(void)foldr< descr >( scalar, x[ index ], op );
			}
		}
		return SUCCESS;
	}

	/**
	 * \internal Only applies to masked folding.
	 */
	template< Descriptor descr, bool left, bool sparse, bool monoid, typename IOType, typename MaskType, typename Coords, typename InputType, class OP >
	RC fold_from_scalar_to_vector_generic_maskDriven( Vector< IOType, reference, Coords > & vector, const MaskType * __restrict__ m, const Coords & m_coors, const InputType & scalar, const OP & op ) {
		IOType * __restrict__ x = internal::getRaw( vector );
		Coords & coors = internal::getCoordinates( vector );
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
		#pragma omp parallel
		{
			auto localUpdate = coors.EMPTY_UPDATE();
			const size_t maxAsyncAssigns = coors.maxAsyncAssigns();
			size_t asyncAssigns = 0;
			#pragma omp for schedule( dynamic, config::CACHE_LINE_SIZE::value() ) nowait
			for( size_t i = 0; i < m_coors.nonzeroes(); ++i ) {
				const size_t index = m_coors.index( i );
				if( ! m_coors.template mask< descr >( index, m ) ) {
					continue;
				}
				if( ! sparse || coors.asyncAssign( index, localUpdate ) ) {
					if( left ) {
						(void)foldl< descr >( x[ index ], scalar, op );
					} else {
						(void)foldr< descr >( scalar, x[ index ], op );
					}
				} else if( sparse && monoid ) {
					x[ index ] = static_cast< IOType >( scalar );
					(void)asyncAssigns++;
					if( asyncAssigns == maxAsyncAssigns ) {
						(void)coors.joinUpdate( localUpdate );
						asyncAssigns = 0;
					}
				}
			}
			while( sparse && monoid && ! coors.joinUpdate( localUpdate ) ) {}
		} // end pragma omp parallel
#else
				for( size_t i = 0; i < m_coors.nonzeroes(); ++i ) {
					const size_t index = m_coors.index( i );
					if( ! m_coors.template mask< descr >( index, m ) ) {
						continue;
					}
					if( ! sparse || coors.assign( index ) ) {
						if( left ) {
							(void)foldl< descr >( x[ index ], scalar, op );
						} else {
							(void)foldr< descr >( scalar, x[ index ], op );
						}
					} else if( sparse && monoid ) {
						x[ index ] = static_cast< IOType >( scalar );
					}
				}
#endif
		return SUCCESS;
	}

	template< Descriptor descr,
		bool left,   // if this is false, the right-looking fold is assumed
		bool sparse, // whether \a vector was sparse
		bool masked,
		bool monoid, // whether \a op was passed as a monoid
		typename MaskType,
		typename IOType,
		typename InputType,
		typename Coords,
		class OP >
	RC fold_from_scalar_to_vector_generic( Vector< IOType, reference, Coords > & vector, const MaskType * __restrict__ m, const Coords * const m_coors, const InputType & scalar, const OP & op ) {
		assert( ! masked || m != NULL );
		assert( ! masked || m_coors != NULL );
		auto & coor = internal::getCoordinates( vector );
		const size_t n = coor.size();
		IOType * __restrict__ const x = internal::getRaw( vector );

		if( sparse && monoid && ! masked ) {
			// output will become dense, use Theta(n) loop
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
			#pragma omp parallel for schedule( static, config::CACHE_LINE_SIZE::value() )
#endif
			for( size_t i = 0; i < n; ++i ) {
				if( coor.assigned( i ) ) {
					if( left ) {
						(void)foldl< descr >( x[ i ], scalar, op );
					} else {
						(void)foldr< descr >( scalar, x[ i ], op );
					}
				} else {
					x[ i ] = static_cast< IOType >( scalar );
				}
			}
			coor.assignAll();
			return SUCCESS;
		} else if( sparse && monoid && masked ) {
			return fold_from_scalar_to_vector_generic_maskDriven< descr, left, true, true >( vector, m, *m_coors, scalar, op );
		} else if( sparse && ! monoid ) {
			const bool maskDriven = masked ? m_coors->nonzeroes() < coor.nonzeroes() : false;
			if( maskDriven ) {
				return fold_from_scalar_to_vector_generic_maskDriven< descr, left, true, false >( vector, m, *m_coors, scalar, op );
			} else {
				return fold_from_scalar_to_vector_generic_vectorDriven< descr, left, masked >( vector, m, m_coors, scalar, op );
			}
		} else if( ! sparse && masked ) {
			return fold_from_scalar_to_vector_generic_maskDriven< descr, left, false, monoid >( vector, m, *m_coors, scalar, op );
		} else {
			// if target vector is dense and there is no mask, then
			// there is no difference between monoid or non-monoid behaviour.
			assert( ! sparse );
			assert( ! masked );
			assert( coor.nonzeroes() == coor.size() );
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
			#pragma omp parallel
			{
				size_t start, end;
				config::OMP::localRange( start, end, 0, coor.size() );
#else
					const size_t start = 0;
					const size_t end = coor.size();
#endif
				const size_t local_n = end - start;
				if( local_n > 0 ) {
					if( left ) {
						op.eWiseFoldlAS( internal::getRaw( vector ) + start, scalar, local_n );
					} else {
						op.eWiseFoldrSA( scalar, internal::getRaw( vector ) + start, local_n );
					}
				}
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
			} // end pragma omp parallel
#endif
		}
		return SUCCESS;
	}

	/**
	 * Generic fold implementation on two vectors.
	 *
	 * @tparam descr  The descriptor under which the operation takes place.
	 * @tparam left   Whether we are folding left (or right, otherwise).
	 * @tparam sparse Whether one of \a fold_into or \a to_fold is sparse.
	 * @tparam OP     The operator to use while folding.
	 * @tparam IType  The input data type (of \a to_fold).
	 * @tparam IOType The input/output data type (of \a fold_into).
	 *
	 * \note Sparseness is passed explicitly since it is illegal when not
	 *       called using a monoid. This function, however, has no way to
	 *       check for this user input.
	 *
	 * @param[in,out] fold_into The vector whose elements to fold into.
	 * @param[in]     to_fold   The vector whose elements to fold.
	 * @param[in]     op        The operator to use while folding.
	 *
	 * The sizes of \a fold_into and \a to_fold must match; this is an elementwise
	 * fold.
	 *
	 * @returns #ILLEGAL  If \a sparse is <tt>false</tt> while one of \a fold_into
	 *                    or \a to_fold is sparse.
	 * @returns #MISMATCH If the sizes of \a fold_into and \a to_fold do not
	 *                    match.
	 * @returns #SUCCESS  On successful completion of this function call.
	 */
	template< Descriptor descr,
		bool left, // if this is false, the right-looking fold is assumed
		bool sparse,
		bool masked,
		bool monoid,
		typename MaskType,
		class OP,
		typename IOType,
		typename IType,
		typename Coords >
	RC fold_from_vector_to_vector_generic( Vector< IOType, reference, Coords > & fold_into,
		const Vector< MaskType, reference, Coords > * const m,
		const Vector< IType, reference, Coords > & to_fold,
		const OP & op ) {
		assert( ! masked || ( m != NULL ) );
		// take at least a number of elements so that no two threads operate on the same cache line
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
		const constexpr size_t blocksize = config::SIMD_BLOCKSIZE< IOType >::value() > config::SIMD_BLOCKSIZE< IType >::value() ? config::SIMD_BLOCKSIZE< IOType >::value() :
                                                                                                                                  config::SIMD_BLOCKSIZE< IType >::value();
		static_assert( blocksize > 0, "Config error: zero blocksize in call to fold_from_vector_to_vector_generic!" );
#endif
		const size_t n = size( fold_into );
		if( n != size( to_fold ) ) {
			return MISMATCH;
		}
		if( ! sparse && nnz( fold_into ) < n ) {
			return ILLEGAL;
		}
		if( ! sparse && nnz( to_fold ) < n ) {
			return ILLEGAL;
		}
		if( ! sparse && ! masked ) {
#ifdef _DEBUG
			std::cout << "fold_from_vector_to_vector_generic: in dense variant\n";
#endif
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
#ifdef _DEBUG
			std::cout << "fold_from_vector_to_vector_generic: in OpenMP variant\n";
#endif
			#pragma omp parallel
			{
				size_t start, end;
				config::OMP::localRange( start, end, 0, n, blocksize );
				const size_t range = end - start;
				if( left ) {
					op.eWiseFoldlAA( internal::getRaw( fold_into ) + start, internal::getRaw( to_fold ) + start, range );
				} else {
					op.eWiseFoldrAA( internal::getRaw( to_fold ) + start, internal::getRaw( fold_into ) + start, range );
				}
			}
#else
#ifdef _DEBUG
					std::cout << "fold_from_vector_to_vector_generic: in sequential variant\n";
#endif
					if( left ) {
						op.eWiseFoldlAA( internal::getRaw( fold_into ), internal::getRaw( to_fold ), n );
					} else {
						op.eWiseFoldrAA( internal::getRaw( to_fold ), internal::getRaw( fold_into ), n );
					}
#endif
		} else {
#ifdef _DEBUG
			std::cout << "fold_from_vector_to_vector_generic: in sparse variant\n";
			std::cout << "\tfolding vector of " << nnz( to_fold ) << " nonzeroes into a vector of " << nnz( fold_into ) << " nonzeroes...\n";
#endif
			if( masked && nnz( fold_into ) == n && nnz( to_fold ) == n ) {
				// use sparsity structure of mask for this eWiseFold
				if( left ) {
#ifdef _DEBUG
					std::cout << "fold_from_vector_to_vector_generic: using eWiseLambda, foldl, using to_fold's sparsity\n";
#endif
					return eWiseLambda(
						[ &fold_into, &to_fold, &op ]( const size_t i ) {
#ifdef _DEBUG
							std::cout << "Left-folding " << to_fold[ i ] << " into " << fold_into[ i ];
#endif
							(void)foldl< descr >( fold_into[ i ], to_fold[ i ], op );
#ifdef _DEBUG
							std::cout << " resulting into " << fold_into[ i ] << "\n";
#endif
						},
						*m, to_fold, fold_into );
				} else {
#ifdef _DEBUG
					std::cout << "fold_from_vector_to_vector_generic: using eWiseLambda, foldl, using to_fold's sparsity\n";
#endif
					return eWiseLambda(
						[ &fold_into, &to_fold, &op ]( const size_t i ) {
#ifdef _DEBUG
							std::cout << "Right-folding " << to_fold[ i ] << " into " << fold_into[ i ];
#endif
							(void)foldr< descr >( to_fold[ i ], fold_into[ i ], op );
#ifdef _DEBUG
							std::cout << " resulting into " << fold_into[ i ] << "\n";
#endif
						},
						*m, to_fold, fold_into );
				}
			} else if( ! masked && nnz( fold_into ) == n ) {
				// use sparsity structure of to_fold for this eWiseFold
				if( left ) {
#ifdef _DEBUG
					std::cout << "fold_from_vector_to_vector_generic: using eWiseLambda, foldl, using to_fold's sparsity\n";
#endif
					return eWiseLambda(
						[ &fold_into, &to_fold, &op ]( const size_t i ) {
#ifdef _DEBUG
							std::cout << "Left-folding " << to_fold[ i ] << " into " << fold_into[ i ];
#endif
							(void)foldl< descr >( fold_into[ i ], to_fold[ i ], op );
#ifdef _DEBUG
							std::cout << " resulting into " << fold_into[ i ] << "\n";
#endif
						},
						to_fold, fold_into );
				} else {
#ifdef _DEBUG
					std::cout << "fold_from_vector_to_vector_generic: using eWiseLambda, foldl, using to_fold's sparsity\n";
#endif
					return eWiseLambda(
						[ &fold_into, &to_fold, &op ]( const size_t i ) {
#ifdef _DEBUG
							std::cout << "Right-folding " << to_fold[ i ] << " into " << fold_into[ i ];
#endif
							(void)foldr< descr >( to_fold[ i ], fold_into[ i ], op );
#ifdef _DEBUG
							std::cout << " resulting into " << fold_into[ i ] << "\n";
#endif
						},
						to_fold, fold_into );
				}
			} else if( ! masked && nnz( to_fold ) == n ) {
				// use sparsity structure of fold_into for this eWiseFold
				if( left ) {
#ifdef _DEBUG
					std::cout << "fold_from_vector_to_vector_generic: using eWiseLambda, foldl, using fold_into's sparsity\n";
#endif
					return eWiseLambda(
						[ &fold_into, &to_fold, &op ]( const size_t i ) {
#ifdef _DEBUG
							std::cout << "Left-folding " << to_fold[ i ] << " into " << fold_into[ i ];
#endif
							(void)foldl< descr >( fold_into[ i ], to_fold[ i ], op );
#ifdef _DEBUG
							std::cout << " resulting into " << fold_into[ i ] << "\n";
#endif
						},
						fold_into, to_fold );
				} else {
#ifdef _DEBUG
					std::cout << "fold_from_vector_to_vector_generic: using eWiseLambda, foldr, using fold_into's sparsity\n";
#endif
					return eWiseLambda(
						[ &fold_into, &to_fold, &op ]( const size_t i ) {
#ifdef _DEBUG
							std::cout << "Right-folding " << to_fold[ i ] << " into " << fold_into[ i ];
#endif
							(void)foldr< descr >( to_fold[ i ], fold_into[ i ], op );
#ifdef _DEBUG
							std::cout << " resulting into " << fold_into[ i ] << "\n";
#endif
						},
						fold_into, to_fold );
				}
				/* TODO: issue #66. Also replaces the above eWiseLambda
				} else if( !monoid ) {
#ifdef _DEBUG
				    std::cout << "fold_from_vector_to_vector_generic (non-monoid): using specialised code to merge two sparse vectors and/or to handle output masks\n";
#endif
				    //both sparse, cannot rely on #eWiseLambda
				    const bool intoDriven = nnz( fold_into ) < nnz( to_fold );
				    if( masked ) {
				        if( intoDriven && (nnz( *m ) < nnz( fold_into )) ) {
				            // maskDriven
				        } else {
				            // dstDriven
				        }
				    } else if( intoDriven ) {
				        //dstDriven
				    } else {
				        //srcDriven
				    }
				} else {
				    const bool vectorDriven = nnz( fold_into ) + nnz( to_fold ) < size( fold_into );
				    const bool maskDriven = masked ?
				        nnz( *m ) < (nnz( fold_into ) + nnz( to_fold )) :
				        false;
				    if( maskDriven ) {
				        // maskDriven
				    } else if( vectorDriven ) {
				        // vectorDriven (monoid)
				    } else {
				        // Theta(n) loop
				    }
				}*/
			} else {
#ifdef _DEBUG
				std::cout << "fold_from_vector_to_vector_generic (non-monoid): using specialised code to merge two sparse vectors and/or to handle output masks\n";
#endif
				assert( ! monoid );
				const IType * __restrict__ const tf_raw = internal::getRaw( to_fold );
				IOType * __restrict__ const fi_raw = internal::getRaw( fold_into );
				auto & fi = internal::getCoordinates( fold_into );
				const auto & tf = internal::getCoordinates( to_fold );
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
#ifdef _DEBUG
				std::cout << "\tfold_from_vector_to_vector_generic, in OpenMP parallel code\n";
#endif
				#pragma omp parallel
				{
					size_t start, end;
					config::OMP::localRange( start, end, 0, tf.nonzeroes() );
					internal::Coordinates< reference >::Update local_update = fi.EMPTY_UPDATE();
					const size_t maxAsyncAssigns = fi.maxAsyncAssigns();
					size_t asyncAssigns = 0;
					for( size_t k = start; k < end; ++k ) {
						const size_t i = tf.index( k );
						if( masked && ! internal::getCoordinates( *m ).template mask< descr >( i, internal::getRaw( *m ) ) ) {
							continue;
						}
						if( fi.assigned( i ) ) {
							if( left ) {
#ifdef _DEBUG
								#pragma omp critical
								std::cout << "\tfoldl< descr >( fi_raw[ i ], tf_raw[ i ], op ), i = " << i << ": " << tf_raw[ i ] << " goes into " << fi_raw[ i ];
#endif
								(void)foldl< descr >( fi_raw[ i ], tf_raw[ i ], op );
#ifdef _DEBUG
								#pragma omp critical
								std::cout << " which results in " << fi_raw[ i ] << "\n";
#endif
							} else {
#ifdef _DEBUG
								#pragma omp critical
								std::cout << "\tfoldr< descr >( tf_raw[ i ], fi_raw[ i ], op ), i = " << i << ": " << tf_raw[ i ] << " goes into " << fi_raw[ i ];
#endif
								(void)foldr< descr >( tf_raw[ i ], fi_raw[ i ], op );
#ifdef _DEBUG
								#pragma omp critical
								std::cout << " which results in " << fi_raw[ i ] << "\n";
#endif
							}
						} else if( monoid ) {
#ifdef _DEBUG
							#pragma omp critical
							std::cout << " index " << i << " is unset. Old value " << fi_raw[ i ] << " will be overwritten with " << tf_raw[ i ] << "\n";
#endif
							fi_raw[ i ] = tf_raw[ i ];
							if( ! fi.asyncAssign( i, local_update ) ) {
								(void)++asyncAssigns;
							}
						}
						if( asyncAssigns == maxAsyncAssigns ) {
							const bool was_empty = fi.joinUpdate( local_update );
#ifdef NDEBUG
							(void)was_empty;
#else
							assert( ! was_empty );
#endif
							asyncAssigns = 0;
						}
					}
					while( ! fi.joinUpdate( local_update ) ) {}
				}
#else
#ifdef _DEBUG
						std::cout << "\tin sequential version...\n";
#endif
						for( size_t k = 0; k < tf.nonzeroes(); ++k ) {
							const size_t i = tf.index( k );
							if( masked && ! internal::getCoordinates( *m ).template mask< descr >( i, internal::getRaw( *m ) ) ) {
								continue;
							}
							assert( i < n );
							if( fi.assigned( i ) ) {
								if( left ) {
#ifdef _DEBUG
									std::cout << "\tfoldl< descr >( fi_raw[ i ], "
												 "tf_raw[ i ], op ), i = "
											  << i << ": " << tf_raw[ i ] << " goes into " << fi_raw[ i ];
#endif
									(void)foldl< descr >( fi_raw[ i ], tf_raw[ i ], op );
#ifdef _DEBUG
									std::cout << " which results in " << fi_raw[ i ] << "\n";
#endif
								} else {
#ifdef _DEBUG
									std::cout << "\tfoldr< descr >( tf_raw[ i ], "
												 "fi_raw[ i ], op ), i = "
											  << i << ": " << tf_raw[ i ] << " goes into " << fi_raw[ i ];
#endif
									(void)foldr< descr >( tf_raw[ i ], fi_raw[ i ], op );
#ifdef _DEBUG
									std::cout << " which results in " << fi_raw[ i ] << "\n";
#endif
								}
							} else if( monoid ) {
#ifdef _DEBUG
								std::cout << "\tindex " << i << " is unset. Old value " << fi_raw[ i ] << " will be overwritten with " << tf_raw[ i ] << "\n";
#endif
								fi_raw[ i ] = tf_raw[ i ];
								(void)fi.assign( i );
							}
						}
#endif
			}
		}

#ifdef _DEBUG
		std::cout << "\tCall to fold_from_vector_to_vector_generic done. "
					 "Output now contains "
				  << nnz( fold_into ) << " / " << size( fold_into ) << " nonzeroes.\n";
#endif
		// done
		return SUCCESS;
	}

} // namespace internal

/**
 * Folds all elements in a GraphBLAS vector \a x into a single value \a beta.
 *
 * The original value of \a beta is used as the right-hand side input of the
 * operator \a op. A left-hand side input for \a op is retrieved from the
 * input vector \a x. The result of the operation is stored in \a beta. This
 * process is repeated for every element in \a x.
 *
 * At function exit, \a beta will equal
 * \f$ \beta \odot x_0 \odot x_1 \odot \ldots x_{n-1} \f$.
 *
 * @tparam descr     The descriptor used for evaluating this function. By
 *                   default, this is grb::descriptors::no_operation.
 * @tparam OP        The type of the operator to be applied.
 *                   The operator must be associative.
 * @tparam InputType The type of the elements of \a x.
 * @tparam IOType    The type of the value \a y.
 *
 * @param[in]     x    The input vector \a x that will not be modified. This
 *                     input vector must be dense.
 * @param[in,out] beta On function entry: the initial value to be applied to
 *                     \a op from the right-hand side.
 *                     On function exit: the result of repeated applications
 *                     from the left-hand side of elements of \a x.
 * @param[in]    op    The monoid under which to perform this right-folding.
 *
 * \note We only define fold under monoids, not under plain operators.
 *
 * @returns grb::SUCCESS This function always succeeds.
 * @returns grb::ILLEGAL When a sparse vector is passed. In this case, the call
 *                       to this function will have no other effects.
 *
 * \warning Since this function folds from left-to-right using binary
 *          operators, this function \em cannot take sparse vectors as input--
 *          a monoid is required to give meaning to missing vector entries.
 *          See grb::reducer for use with sparse vectors instead.
 *
 * \parblock
 * \par Valid descriptors
 * grb::descriptors::no_operation, grb::descriptors::no_casting.
 *
 * \note Invalid descriptors will be ignored.
 *
 * If grb::descriptors::no_casting is specified, then 1) the first domain of
 * \a op must match \a IOType, 2) the second domain of \a op must match
 * \a InputType, and 3) the third domain must match \a IOType. If one of these
 * is not true, the code shall not compile.
 *
 * \endparblock
 *
 * \parblock
 * \par Valid operator types
 * The given operator \a op is required to be:
 *   -# associative.
 * \endparblock
 *
 * \parblock
 * \par Performance guarantees
 *      -# This call comprises \f$ \Theta(n) \f$ work, where \f$ n \f$ equals
 *         the size of the vector \a x. The constant factor depends on the
 *         cost of evaluating the underlying binary operator. A good
 *         implementation uses vectorised instructions whenever the input
 *         domains, the output domain, and the operator used allow for this.
 *
 *      -# This call will not result in additional dynamic memory allocations.
 *
 *      -# This call takes \f$ \mathcal{O}(1) \f$ memory beyond the memory
 *         used by the application at the point of a call to this function.
 *
 *      -# This call incurs at most
 *         \f$ n \cdot\mathit{sizeof}(\mathit{InputType}) + \mathcal{O}(1) \f$
 *         bytes of data movement. A good implementation will rely on in-place
 *         operators.
 * \endparblock
 *
 * @see grb::operators::internal::Operator for a discussion on when in-place
 *      and/or vectorised operations are used.
 */
template< Descriptor descr = descriptors::no_operation, class Monoid, typename InputType, typename IOType, typename Coords >
RC foldr( const Vector< InputType, reference, Coords > & x,
	IOType & beta,
	const Monoid & monoid = Monoid(),
	const typename std::enable_if< ! grb::is_object< InputType >::value && ! grb::is_object< IOType >::value && grb::is_monoid< Monoid >::value, void >::type * const = NULL ) {
	grb::Vector< bool, reference, Coords > mask( 0 );
	return internal::fold_from_vector_to_scalar_generic< descr, false, false >( beta, x, mask, monoid.getOperator() );
}

/**
 * For all elements in a GraphBLAS vector \a y, fold the value \f$ \alpha \f$
 * into each element.
 *
 * The original value of \f$ \alpha \f$ is used as the left-hand side input
 * of the operator \a op. The right-hand side inputs for \a op are retrieved
 * from the input vector \a y. The result of the operation is stored in \a y,
 * thus overwriting its previous values.
 *
 * The value of \f$ y_i \f$ after a call to thus function thus equals
 * \f$ \alpha \odot y_i \f$, for all \f$ i \in \{ 0, 1, \dots, n - 1 \} \f$.
 *
 * @tparam descr     The descriptor used for evaluating this function. By
 *                   default, this is grb::descriptors::no_operation.
 * @tparam OP        The type of the operator to be applied.
 * @tparam InputType The type of \a alpha.
 * @tparam IOType    The type of the elements in \a y.
 *
 * @param[in]     alpha The input value to apply as the left-hand side input
 *                      to \a op.
 * @param[in,out] y     On function entry: the initial values to be applied as
 *                      the right-hand side input to \a op.
 *                      On function exit: the output data.
 * @param[in]     op    The monoid under which to perform this left-folding.
 *
 * @returns grb::SUCCESS This function always succeeds.
 *
 * \note We only define fold under monoids, not under plain operators.
 *
 * \parblock
 * \par Valid descriptors
 * grb::descriptors::no_operation, grb::descriptors::no_casting.
 *
 * \note Invalid descriptors will be ignored.
 *
 * If grb::descriptors::no_casting is specified, then 1) the first domain of
 * \a op must match \a IOType, 2) the second domain of \a op must match
 * \a InputType, and 3) the third domain must match \a IOType. If one of these
 * is not true, the code shall not compile.
 *
 * \endparblock
 *
 * \parblock
 * \par Valid operator types
 * The given operator \a op is required to be:
 *   -# (no requirements).
 * \endparblock
 *
 * \parblock
 * \par Performance guarantees
 *      -# This call comprises \f$ \Theta(n) \f$ work, where \f$ n \f$ equals
 *         the size of the vector \a x. The constant factor depends on the
 *         cost of evaluating the underlying binary operator. A good
 *         implementation uses vectorised instructions whenever the input
 *         domains, the output domain, and the operator used allow for this.
 *
 *      -# This call will not result in additional dynamic memory allocations.
 *
 *      -# This call takes \f$ \mathcal{O}(1) \f$ memory beyond the memory
 *         used by the application at the point of a call to this function.
 *
 *      -# This call incurs at most
 *         \f$ 2n \cdot \mathit{sizeof}(\mathit{IOType}) + \mathcal{O}(1) \f$
 *         bytes of data movement.
 * \endparblock
 *
 * @see grb::operators::internal::Operator for a discussion on when in-place
 *      and/or vectorised operations are used.
 */
template< Descriptor descr = descriptors::no_operation, class Monoid, typename IOType, typename InputType, typename Coords >
RC foldr( const InputType & alpha,
	Vector< IOType, reference, Coords > & y,
	const Monoid & monoid = Monoid(),
	const typename std::enable_if< ! grb::is_object< InputType >::value && ! grb::is_object< IOType >::value && grb::is_monoid< Monoid >::value, void >::type * const = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Monoid::D1, IOType >::value ), "grb::foldl",
		"called with a vector x of a type that does not match the first domain "
		"of the given operator" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Monoid::D2, InputType >::value ), "grb::foldl",
		"called on a vector y of a type that does not match the second domain "
		"of the given operator" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Monoid::D3, IOType >::value ), "grb::foldl",
		"called on a vector x of a type that does not match the third domain "
		"of the given operator" );

	// if no monoid was given, then we can only handle dense vectors
	auto null_coor = &( internal::getCoordinates( y ) );
	null_coor = NULL;
	if( nnz( y ) < size( y ) ) {
		return internal::fold_from_scalar_to_vector_generic< descr, false, true, false, true, void >( y, NULL, null_coor, alpha, monoid.getOperator() );
	} else {
		return internal::fold_from_scalar_to_vector_generic< descr, false, false, false, true, void >( y, NULL, null_coor, alpha, monoid.getOperator() );
	}
}

/**
 * Computes y = x + y, operator variant.
 *
 * Specialisation for scalar \a x.
 */
template< Descriptor descr = descriptors::no_operation, class OP, typename IOType, typename InputType, typename Coords >
RC foldr( const InputType & alpha,
	Vector< IOType, reference, Coords > & y,
	const OP & op = OP(),
	const typename std::enable_if< ! grb::is_object< InputType >::value && ! grb::is_object< IOType >::value && grb::is_operator< OP >::value, void >::type * const = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename OP::D1, IOType >::value ), "grb::foldl",
		"called with a vector x of a type that does not match the first domain "
		"of the given operator" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename OP::D2, InputType >::value ), "grb::foldl",
		"called on a vector y of a type that does not match the second domain "
		"of the given operator" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename OP::D3, IOType >::value ), "grb::foldl",
		"called on a vector x of a type that does not match the third domain "
		"of the given operator" );

	// if no monoid was given, then we can only handle dense vectors
	auto null_coor = &( internal::getCoordinates( y ) );
	null_coor = NULL;
	if( nnz( y ) < size( y ) ) {
		return internal::fold_from_scalar_to_vector_generic< descr, false, true, false, false, void >( y, NULL, null_coor, alpha, op );
	} else {
		return internal::fold_from_scalar_to_vector_generic< descr, false, false, false, false, void >( y, NULL, null_coor, alpha, op );
	}
}

/**
 * Folds all elements in a GraphBLAS vector \a x into the corresponding
 * elements from an input/output vector \a y. The vectors must be of equal
 * size \f$ n \f$. For all \f$ i \in \{0,1,\ldots,n-1\} \f$, the new value
 * of at the i-th index of \a y after a call to this function thus equals
 * \f$ x_i \odot y_i \f$.
 *
 * @tparam descr     The descriptor used for evaluating this function. By
 *                   default, this is grb::descriptors::no_operation.
 * @tparam OP        The type of the operator to be applied.
 * @tparam IOType    The type of the elements of \a x.
 * @tparam InputType The type of the elements of \a y.
 *
 * @param[in]     x  The input vector \a y that will not be modified.
 * @param[in,out] y  On function entry: the initial value to be applied to
 *                   \a op as the right-hand side input.
 *                   On function exit: the result of repeated applications
 *                   from the right-hand side using elements from \a y.
 * @param[in]     op The operator under which to perform this right-folding.
 *
 * @returns grb::SUCCESS This function always succeeds.
 *
 * \note The element-wise fold is also defined for monoids.
 *
 * \parblock
 * \par Valid descriptors
 * grb::descriptors::no_operation, grb::descriptors::no_casting.
 *
 * \note Invalid descriptors will be ignored.
 *
 * If grb::descriptors::no_casting is specified, then 1) the first domain of
 * \a op must match \a InputType, 2) the second domain of \a op must match
 * \a IOType, and 3) the third domain must match \a IOType. If one of these
 * is not true, the code shall not compile.
 *
 * \endparblock
 *
 * \parblock
 * \par Valid operator types
 * The given operator \a op is required to be:
 *   -# (no requirements).
 * \endparblock
 *
 * \parblock
 * \par Performance guarantees
 *      -# This call comprises \f$ \Theta(n) \f$ work, where \f$ n \f$ equals
 *         the size of the vector \a x. The constant factor depends on the
 *         cost of evaluating the underlying binary operator. A good
 *         implementation uses vectorised instructions whenever the input
 *         domains, the output domain, and the operator used allow for this.
 *
 *      -# This call will not result in additional dynamic memory allocations.
 *
 *      -# This call takes \f$ \mathcal{O}(1) \f$ memory beyond the memory
 *         used by the application at the point of a call to this function.
 *
 *      -# This call incurs at most
 *         \f$ n \cdot (
 *                       \mathit{sizeof}(InputType) + 2\mathit{sizeof}(IOType)
 *                     ) + \mathcal{O}(1)
 *         \f$
 *         bytes of data movement. A good implementation will rely on in-place
 *         operators whenever allowed.
 * \endparblock
 *
 * @see grb::operators::internal::Operator for a discussion on when in-place
 *      and/or vectorised operations are used.
 */
template< Descriptor descr = descriptors::no_operation, class OP, typename IOType, typename InputType, typename Coords >
RC foldr( const Vector< InputType, reference, Coords > & x,
	Vector< IOType, reference, Coords > & y,
	const OP & op = OP(),
	const typename std::enable_if< grb::is_operator< OP >::value && ! grb::is_object< InputType >::value && ! grb::is_object< IOType >::value, void >::type * = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename OP::D1, InputType >::value ), "grb::eWiseFoldr",
		"called with a vector x of a type that does not match the first domain "
		"of the given operator" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename OP::D2, IOType >::value ), "grb::eWiseFoldr",
		"called on a vector y of a type that does not match the second domain "
		"of the given operator" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename OP::D3, IOType >::value ), "grb::eWiseFoldr",
		"called on a vector y of a type that does not match the third domain "
		"of the given operator" );

	if( size( x ) != size( y ) ) {
		return MISMATCH;
	}

#ifdef _DEBUG
	std::cout << "In foldr ([T]<-[T])\n";
#endif

	const Vector< bool, reference, Coords > * const null_mask = NULL;
	if( nnz( x ) < size( x ) || nnz( y ) < size( y ) ) {
		return internal::fold_from_vector_to_vector_generic< descr, false, true, false, false >( y, null_mask, x, op );
	} else {
		return internal::fold_from_vector_to_vector_generic< descr, false, false, false, false >( y, null_mask, x, op );
	}
}

/**
 * Calculates \f$ x = x . y \f$ using a given operator.
 *
 * Masked variant.
 */
template< Descriptor descr = descriptors::no_operation, class OP, typename IOType, typename MaskType, typename InputType, typename Coords >
RC foldr( const Vector< InputType, reference, Coords > & x,
	const Vector< MaskType, reference, Coords > & m,
	Vector< IOType, reference, Coords > & y,
	const OP & op = OP(),
	const typename std::enable_if< grb::is_operator< OP >::value && ! grb::is_object< InputType >::value && ! grb::is_object< MaskType >::value && ! grb::is_object< IOType >::value, void >::type * =
		NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename OP::D1, InputType >::value ), "grb::eWiseFoldr",
		"called with a vector x of a type that does not match the first domain "
		"of the given operator" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename OP::D2, IOType >::value ), "grb::eWiseFoldr",
		"called on a vector y of a type that does not match the second domain "
		"of the given operator" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename OP::D3, IOType >::value ), "grb::eWiseFoldr",
		"called on a vector y of a type that does not match the third domain "
		"of the given operator" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< bool, MaskType >::value ), "grb::eWiseFoldr", "called with a non-Boolean mask" );

	if( size( m ) == 0 ) {
		return foldr< descr >( x, y, op );
	}

	const size_t n = size( x );
	if( n != size( y ) || n != size( m ) ) {
		return MISMATCH;
	}

	if( nnz( x ) < n || nnz( y ) < n ) {
		return internal::fold_from_vector_to_vector_generic< descr, false, true, true, false >( y, &m, x, op );
	} else {
		return internal::fold_from_vector_to_vector_generic< descr, false, false, true, false >( y, &m, x, op );
	}
}

/**
 * Folds all elements in a GraphBLAS vector \a x into the corresponding
 * elements from an input/output vector \a y. The vectors must be of equal
 * size \f$ n \f$. For all \f$ i \in \{0,1,\ldots,n-1\} \f$, the new value
 * of at the i-th index of \a y after a call to this function thus equals
 * \f$ x_i \odot y_i \f$.
 *
 * @tparam descr     The descriptor used for evaluating this function. By
 *                   default, this is grb::descriptors::no_operation.
 * @tparam Monoid    The type of the monoid to be applied.
 * @tparam IOType    The type of the elements of \a x.
 * @tparam InputType The type of the elements of \a y.
 *
 * @param[in]       x    The input vector \a y that will not be modified.
 * @param[in,out]   y    On function entry: the initial value to be applied
 *                       to \a op as the right-hand side input.
 *                       On function exit: the result of repeated applications
 *                       from the right-hand side using elements from \a y.
 * @param[in]     monoid The monoid under which to perform this right-folding.
 *
 * @returns grb::SUCCESS This function always succeeds.
 *
 * \note The element-wise fold is also defined for operators.
 *
 * \parblock
 * \par Valid descriptors
 * grb::descriptors::no_operation, grb::descriptors::no_casting.
 *
 * \note Invalid descriptors will be ignored.
 *
 * If grb::descriptors::no_casting is specified, then 1) the first domain of
 * \a op must match \a InputType, 2) the second domain of \a op must match
 * \a IOType, and 3) the third domain must match \a IOType. If one of these
 * is not true, the code shall not compile.
 *
 * \endparblock
 *
 * \parblock
 * \par Valid monoid types
 * The given operator \a op is required to be:
 *   -# (no requirements).
 * \endparblock
 *
 * \parblock
 * \par Performance guarantees
 *      -# This call comprises \f$ \Theta(n) \f$ work, where \f$ n \f$ equals
 *         the size of the vector \a x. The constant factor depends on the
 *         cost of evaluating the underlying binary operator. A good
 *         implementation uses vectorised instructions whenever the input
 *         domains, the output domain, and the operator used allow for this.
 *
 *      -# This call will not result in additional dynamic memory allocations.
 *
 *      -# This call takes \f$ \mathcal{O}(1) \f$ memory beyond the memory
 *         used by the application at the point of a call to this function.
 *
 *      -# This call incurs at most
 *         \f$ n \cdot (
 *                       \mathit{sizeof}(InputType) + 2\mathit{sizeof}(IOType)
 *                     ) + \mathcal{O}(1)
 *         \f$
 *         bytes of data movement. A good implementation will rely on in-place
 *         operators whenever allowed.
 * \endparblock
 *
 * @see grb::operators::internal::Operator for a discussion on when in-place
 *      and/or vectorised operations are used.
 */
template< Descriptor descr = descriptors::no_operation, class Monoid, typename IOType, typename InputType, typename Coords >
RC foldr( const Vector< InputType, reference, Coords > & x,
	Vector< IOType, reference, Coords > & y,
	const Monoid & monoid = Monoid(),
	const typename std::enable_if< grb::is_monoid< Monoid >::value && ! grb::is_object< InputType >::value && ! grb::is_object< IOType >::value, void >::type * = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Monoid::D1, InputType >::value ), "grb::eWiseFoldr",
		"called with a vector x of a type that does not match the first domain "
		"of the given monoid" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Monoid::D2, IOType >::value ), "grb::eWiseFoldr",
		"called on a vector y of a type that does not match the second domain "
		"of the given monoid" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Monoid::D3, IOType >::value ), "grb::eWiseFoldr",
		"called on a vector y of a type that does not match the third domain "
		"of the given monoid" );

	// dynamic sanity checks
	const size_t n = size( x );
	if( n != size( y ) ) {
		return MISMATCH;
	}

	const Vector< bool, reference, Coords > * const null_mask = NULL;
	if( nnz( x ) < n || nnz( y ) < n ) {
		return internal::fold_from_vector_to_vector_generic< descr, false, true, false, true >( y, null_mask, x, monoid.getOperator() );
	} else {
		return internal::fold_from_vector_to_vector_generic< descr, false, false, false, true >( y, null_mask, x, monoid.getOperator() );
	}
}

/**
 * Calculates \f$ x = x + y \f$ for a given monoid.
 *
 * Masked variant.
 */
template< Descriptor descr = descriptors::no_operation, class Monoid, typename IOType, typename MaskType, typename InputType, typename Coords >
RC foldr( const Vector< InputType, reference, Coords > & x,
	const Vector< MaskType, reference, Coords > & m,
	Vector< IOType, reference, Coords > & y,
	const Monoid & monoid = Monoid(),
	const typename std::enable_if< grb::is_monoid< Monoid >::value && ! grb::is_object< MaskType >::value && ! grb::is_object< InputType >::value && ! grb::is_object< IOType >::value, void >::type * =
		NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Monoid::D1, InputType >::value ), "grb::eWiseFoldr",
		"called with a vector x of a type that does not match the first domain "
		"of the given monoid" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Monoid::D2, IOType >::value ), "grb::eWiseFoldr",
		"called on a vector y of a type that does not match the second domain "
		"of the given monoid" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Monoid::D3, IOType >::value ), "grb::eWiseFoldr",
		"called on a vector y of a type that does not match the third domain "
		"of the given monoid" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< bool, MaskType >::value ), "grb::eWiseFoldr", "called with a mask of non-Boolean type" );

	// check empty mask
	if( size( m ) == 0 ) {
		return foldr< descr >( x, y, monoid );
	}

	// dynamic sanity checks
	const size_t n = size( x );
	if( n != size( y ) || n != size( m ) ) {
		return MISMATCH;
	}

	if( nnz( x ) < n || nnz( y ) < n ) {
		return internal::fold_from_vector_to_vector_generic< descr, false, true, true, true >( y, &m, x, monoid.getOperator() );
	} else {
		return internal::fold_from_vector_to_vector_generic< descr, false, false, true, true >( y, &m, x, monoid.getOperator() );
	}
}

/**
 * For all elements in a GraphBLAS vector \a x, fold the value \f$ \beta \f$
 * into each element.
 *
 * The original value of \f$ \beta \f$ is used as the right-hand side input
 * of the operator \a op. The left-hand side inputs for \a op are retrieved
 * from the input vector \a x. The result of the operation is stored in
 * \f$ \beta \f$, thus overwriting its previous value. This process is
 * repeated for every element in \a y.
 *
 * The value of \f$ x_i \f$ after a call to thus function thus equals
 * \f$ x_i \odot \beta \f$, for all \f$ i \in \{ 0, 1, \dots, n - 1 \} \f$.
 *
 * @tparam descr     The descriptor used for evaluating this function. By
 *                   default, this is grb::descriptors::no_operation.
 * @tparam OP        The type of the operator to be applied.
 * @tparam IOType    The type of the value \a beta.
 * @tparam InputType The type of the elements of \a x.
 *
 * @param[in,out] x    On function entry: the initial values to be applied as
 *                     the left-hand side input to \a op. The input vector must
 *                     be dense.
 *                     On function exit: the output data.
 * @param[in]     beta The input value to apply as the right-hand side input
 *                     to \a op.
 * @param[in]     op   The operator under which to perform this left-folding.
 *
 * @returns grb::SUCCESS This function always succeeds.
 *
 * \note This function is also defined for monoids.
 *
 * \warning If \a x is sparse and this operation is requested, a monoid instead
 *          of an operator is required!
 *
 * \parblock
 * \par Valid descriptors
 * grb::descriptors::no_operation, grb::descriptors::no_casting.
 *
 * \note Invalid descriptors will be ignored.
 *
 * If grb::descriptors::no_casting is specified, then 1) the first domain of
 * \a op must match \a IOType, 2) the second domain of \a op must match
 * \a InputType, and 3) the third domain must match \a IOType. If one of these
 * is not true, the code shall not compile.
 *
 * \endparblock
 *
 * \parblock
 * \par Valid operator types
 * The given operator \a op is required to be:
 *   -# (no requirement).
 * \endparblock
 *
 * \parblock
 * \par Performance guarantees
 *      -# This call comprises \f$ \Theta(n) \f$ work, where \f$ n \f$ equals
 *         the size of the vector \a x. The constant factor depends on the
 *         cost of evaluating the underlying binary operator. A good
 *         implementation uses vectorised instructions whenever the input
 *         domains, the output domain, and the operator used allow for this.
 *
 *      -# This call will not result in additional dynamic memory allocations.
 *
 *      -# This call takes \f$ \mathcal{O}(1) \f$ memory beyond the memory
 *         used by the application at the point of a call to this function.
 *
 *      -# This call incurs at most
 *         \f$ 2n \cdot \mathit{sizeof}(\mathit{IOType}) + \mathcal{O}(1) \f$
 *         bytes of data movement.
 * \endparblock
 *
 * @see grb::operators::internal::Operator for a discussion on when in-place
 *      and/or vectorised operations are used.
 */
template< Descriptor descr = descriptors::no_operation, class Op, typename IOType, typename InputType, typename Coords >
RC foldl( Vector< IOType, reference, Coords > & x,
	const InputType beta,
	const Op & op = Op(),
	const typename std::enable_if< ! grb::is_object< IOType >::value && ! grb::is_object< InputType >::value && grb::is_operator< Op >::value, void >::type * = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Op::D1, IOType >::value ), "grb::foldl",
		"called with a vector x of a type that does not match the first domain "
		"of the given operator" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Op::D2, InputType >::value ), "grb::foldl",
		"called on a vector y of a type that does not match the second domain "
		"of the given operator" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Op::D3, IOType >::value ), "grb::foldl",
		"called on a vector x of a type that does not match the third domain "
		"of the given operator" );

	// if no monoid was given, then we can only handle dense vectors
	auto null_coor = &( internal::getCoordinates( x ) );
	null_coor = NULL;
	if( nnz( x ) < size( x ) ) {
		return internal::fold_from_scalar_to_vector_generic< descr, true, true, false, false, void >( x, NULL, null_coor, beta, op );
	} else {
		return internal::fold_from_scalar_to_vector_generic< descr, true, false, false, false, void >( x, NULL, null_coor, beta, op );
	}
}

/**
 * For all elements in a GraphBLAS vector \a x, fold the value \f$ \beta \f$
 * into each element.
 *
 * Masked operator variant.
 */
template< Descriptor descr = descriptors::no_operation, class Op, typename IOType, typename MaskType, typename InputType, typename Coords >
RC foldl( Vector< IOType, reference, Coords > & x,
	const Vector< MaskType, reference, Coords > & m,
	const InputType beta,
	const Op & op = Op(),
	const typename std::enable_if< ! grb::is_object< IOType >::value && ! grb::is_object< MaskType >::value && ! grb::is_object< InputType >::value && grb::is_operator< Op >::value, void >::type * =
		NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Op::D1, IOType >::value ), "grb::foldl",
		"called with a vector x of a type that does not match the first domain "
		"of the given operator" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Op::D2, InputType >::value ), "grb::foldl",
		"called on a vector y of a type that does not match the second domain "
		"of the given operator" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Op::D3, IOType >::value ), "grb::foldl",
		"called on a vector x of a type that does not match the third domain "
		"of the given operator" );
	NO_CAST_OP_ASSERT(
		( ! ( descr & descriptors::no_casting ) || std::is_same< bool, MaskType >::value ), "grb::foldl (reference, vector <- scalar, masked)", "provided mask does not have boolean entries" );
	if( size( m ) == 0 ) {
		return foldl< descr >( x, beta, op );
	}
	const auto m_coor = &( internal::getCoordinates( m ) );
	const auto m_p = internal::getRaw( m );
	if( nnz( x ) < size( x ) ) {
		return internal::fold_from_scalar_to_vector_generic< descr, true, true, true, false >( x, m_p, m_coor, beta, op );
	} else {
		return internal::fold_from_scalar_to_vector_generic< descr, true, false, true, false >( x, m_p, m_coor, beta, op );
	}
}

/**
 * For all elements in a GraphBLAS vector \a x, fold the value \f$ \beta \f$
 * into each element.
 *
 * The original value of \f$ \beta \f$ is used as the right-hand side input
 * of the operator \a op. The left-hand side inputs for \a op are retrieved
 * from the input vector \a x. The result of the operation is stored in
 * \f$ \beta \f$, thus overwriting its previous value. This process is
 * repeated for every element in \a y.
 *
 * The value of \f$ x_i \f$ after a call to thus function thus equals
 * \f$ x_i \odot \beta \f$, for all \f$ i \in \{ 0, 1, \dots, n - 1 \} \f$.
 *
 * @tparam descr     The descriptor used for evaluating this function. By
 *                   default, this is grb::descriptors::no_operation.
 * @tparam Monoid    The type of the monoid to be applied.
 * @tparam IOType    The type of the value \a beta.
 * @tparam InputType The type of the elements of \a x.
 *
 * @param[in,out] x    On function entry: the initial values to be applied as
 *                     the left-hand side input to \a op.
 *                     On function exit: the output data.
 * @param[in]     beta The input value to apply as the right-hand side input
 *                     to \a op.
 * @param[in]   monoid The monoid under which to perform this left-folding.
 *
 * @returns grb::SUCCESS This function always succeeds.
 *
 * \note This function is also defined for operators.
 *
 * \parblock
 * \par Valid descriptors
 * grb::descriptors::no_operation, grb::descriptors::no_casting.
 *
 * \note Invalid descriptors will be ignored.
 *
 * If grb::descriptors::no_casting is specified, then 1) the first domain of
 * \a op must match \a IOType, 2) the second domain of \a op must match
 * \a InputType, and 3) the third domain must match \a IOType. If one of these
 * is not true, the code shall not compile.
 *
 * \endparblock
 *
 * \parblock
 * \par Valid operator types
 * The given operator \a op is required to be:
 *   -# (no requirement).
 * \endparblock
 *
 * \parblock
 * \par Performance guarantees
 *      -# This call comprises \f$ \Theta(n) \f$ work, where \f$ n \f$ equals
 *         the size of the vector \a x. The constant factor depends on the
 *         cost of evaluating the underlying binary operator. A good
 *         implementation uses vectorised instructions whenever the input
 *         domains, the output domain, and the operator used allow for this.
 *
 *      -# This call will not result in additional dynamic memory allocations.
 *
 *      -# This call takes \f$ \mathcal{O}(1) \f$ memory beyond the memory
 *         used by the application at the point of a call to this function.
 *
 *      -# This call incurs at most
 *         \f$ 2n \cdot \mathit{sizeof}(\mathit{IOType}) + \mathcal{O}(1) \f$
 *         bytes of data movement.
 * \endparblock
 *
 * @see grb::operators::internal::Operator for a discussion on when in-place
 *      and/or vectorised operations are used.
 */
template< Descriptor descr = descriptors::no_operation, class Monoid, typename IOType, typename InputType, typename Coords >
RC foldl( Vector< IOType, reference, Coords > & x,
	const InputType beta,
	const Monoid & monoid = Monoid(),
	const typename std::enable_if< ! grb::is_object< IOType >::value && ! grb::is_object< InputType >::value && grb::is_monoid< Monoid >::value, void >::type * = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Monoid::D1, IOType >::value ), "grb::foldl",
		"called with a vector x of a type that does not match the first domain "
		"of the given monoid" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Monoid::D2, InputType >::value ), "grb::foldl",
		"called on a vector y of a type that does not match the second domain "
		"of the given monoid" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Monoid::D3, IOType >::value ), "grb::foldl",
		"called on a vector x of a type that does not match the third domain "
		"of the given monoid" );

	// delegate to generic case
	auto null_coor = &( internal::getCoordinates( x ) );
	null_coor = NULL;
	if( ( descr & descriptors::dense ) || internal::getCoordinates( x ).isDense() ) {
		return internal::fold_from_scalar_to_vector_generic< descr, true, false, false, true, void >( x, NULL, null_coor, beta, monoid.getOperator() );
	} else {
		return internal::fold_from_scalar_to_vector_generic< descr, true, true, false, true, void >( x, NULL, null_coor, beta, monoid.getOperator() );
	}
}

/**
 * For all elements in a GraphBLAS vector \a x, fold the value \f$ \beta \f$
 * into each element.
 *
 * Masked monoid variant.
 */
template< Descriptor descr = descriptors::no_operation, class Monoid, typename IOType, typename MaskType, typename InputType, typename Coords >
RC foldl( Vector< IOType, reference, Coords > & x,
	const Vector< MaskType, reference, Coords > & m,
	const InputType & beta,
	const Monoid & monoid = Monoid(),
	const typename std::enable_if< ! grb::is_object< IOType >::value && ! grb::is_object< MaskType >::value && ! grb::is_object< InputType >::value && grb::is_monoid< Monoid >::value, void >::type * =
		NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Monoid::D1, IOType >::value ), "grb::foldl",
		"called with a vector x of a type that does not match the first domain "
		"of the given monoid" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Monoid::D2, InputType >::value ), "grb::foldl",
		"called on a vector y of a type that does not match the second domain "
		"of the given monoid" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Monoid::D3, IOType >::value ), "grb::foldl",
		"called on a vector x of a type that does not match the third domain "
		"of the given monoid" );
	NO_CAST_OP_ASSERT(
		( ! ( descr & descriptors::no_casting ) || std::is_same< bool, MaskType >::value ), "grb::foldl (reference, vector <- scalar, masked, monoid)", "provided mask does not have boolean entries" );
	if( size( m ) == 0 ) {
		return foldl< descr >( x, beta, monoid );
	}

	// delegate to generic case
	auto m_coor = &( internal::getCoordinates( m ) );
	auto m_p = internal::getRaw( m );
	if( ( descr & descriptors::dense ) || internal::getCoordinates( x ).isDense() ) {
		return internal::fold_from_scalar_to_vector_generic< descr, true, false, true, true >( x, m_p, m_coor, beta, monoid.getOperator() );
	} else {
		return internal::fold_from_scalar_to_vector_generic< descr, true, true, true, true >( x, m_p, m_coor, beta, monoid.getOperator() );
	}
}

/**
 * Folds all elements in a GraphBLAS vector \a y into the corresponding
 * elements from an input/output vector \a x. The vectors must be of equal
 * size \f$ n \f$. For all \f$ i \in \{0,1,\ldots,n-1\} \f$, the new value
 * of at the i-th index of \a x after a call to this function thus equals
 * \f$ x_i \odot y_i \f$.
 *
 * @tparam descr     The descriptor used for evaluating this function. By
 *                   default, this is grb::descriptors::no_operation.
 * @tparam OP        The type of the operator to be applied.
 * @tparam IOType    The type of the value \a x.
 * @tparam InputType The type of the elements of \a y.
 *
 * @param[in,out] x On function entry: the vector whose elements are to be
 *                  applied to \a op as the left-hand side input.
 *                  On function exit: the vector containing the result of
 *                  the requested computation.
 * @param[in]    y  The input vector \a y whose elements are to be applied
 *                  to \a op as right-hand side input.
 * @param[in]    op The operator under which to perform this left-folding.
 *
 * @returns grb::SUCCESS This function always succeeds.
 *
 * \note This function is also defined for monoids.
 *
 * \parblock
 * \par Valid descriptors
 * grb::descriptors::no_operation, grb::descriptors::no_casting.
 *
 * \note Invalid descriptors will be ignored.
 *
 * If grb::descriptors::no_casting is specified, then 1) the first domain of
 * \a op must match \a IOType, 2) the second domain of \a op must match
 * \a InputType, and 3) the third domain must match \a IOType. If one of these
 * is not true, the code shall not compile.
 *
 * \endparblock
 *
 * \parblock
 * \par Valid operator types
 * The given operator \a op is required to be:
 *   -# (no requirements).
 * \endparblock
 *
 * \parblock
 * \par Performance guarantees
 *      -# This call comprises \f$ \Theta(n) \f$ work, where \f$ n \f$ equals
 *         the size of the vector \a x. The constant factor depends on the
 *         cost of evaluating the underlying binary operator. A good
 *         implementation uses vectorised instructions whenever the input
 *         domains, the output domain, and the operator used allow for this.
 *
 *      -# This call will not result in additional dynamic memory allocations.
 *
 *      -# This call takes \f$ \mathcal{O}(1) \f$ memory beyond the memory
 *         used by the application at the point of a call to this function.
 *
 *      -# This call incurs at most
 *         \f$ n \cdot (
 *                \mathit{sizeof}(\mathit{IOType}) +
 *                \mathit{sizeof}(\mathit{InputType})
 *             ) + \mathcal{O}(1) \f$
 *         bytes of data movement. A good implementation will apply in-place
 *         vectorised instructions whenever the input domains, the output
 *         domain, and the operator used allow for this.
 * \endparblock
 *
 * @see grb::operators::internal::Operator for a discussion on when in-place
 *      and/or vectorised operations are used.
 */
template< Descriptor descr = descriptors::no_operation, class OP, typename IOType, typename InputType, typename Coords >
RC foldl( Vector< IOType, reference, Coords > & x,
	const Vector< InputType, reference, Coords > & y,
	const OP & op = OP(),
	const typename std::enable_if< grb::is_operator< OP >::value && ! grb::is_object< IOType >::value && ! grb::is_object< InputType >::value, void >::type * = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename OP::D1, IOType >::value ), "grb::foldl",
		"called with a vector x of a type that does not match the first domain "
		"of the given operator" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename OP::D2, InputType >::value ), "grb::foldl",
		"called on a vector y of a type that does not match the second domain "
		"of the given operator" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename OP::D3, IOType >::value ), "grb::foldl",
		"called on a vector x of a type that does not match the third domain "
		"of the given operator" );

	// dynamic sanity checks
	const size_t n = size( x );
	if( n != size( y ) ) {
		return MISMATCH;
	}

	// all OK, execute
	const Vector< bool, reference, Coords > * const null_mask = NULL;
	if( nnz( x ) < n || nnz( y ) < n ) {
		return internal::fold_from_vector_to_vector_generic< descr, true, true, false, false >( x, null_mask, y, op );
	} else {
		assert( nnz( x ) == n );
		assert( nnz( y ) == n );
		return internal::fold_from_vector_to_vector_generic< descr, true, false, false, false >( x, null_mask, y, op );
	}
}

/**
 * Folds all elements in a GraphBLAS vector \a y into the corresponding
 * elements from an input/output vector \a x. The vectors must be of equal
 * size \f$ n \f$. For all \f$ i \in \{0,1,\ldots,n-1\} \f$, the new value
 * of at the i-th index of \a x after a call to this function thus equals
 * \f$ x_i \odot y_i \f$.
 *
 * @tparam descr     The descriptor used for evaluating this function. By
 *                   default, this is grb::descriptors::no_operation.
 * @tparam Monoid    The type of the monoid to be applied.
 * @tparam IOType    The type of the value \a x.
 * @tparam InputType The type of the elements of \a y.
 *
 * @param[in,out]  x    On function entry: the vector whose elements are to be
 *                      applied to \a op as the left-hand side input.
 *                      On function exit: the vector containing the result of
 *                      the requested computation.
 * @param[in]      y    The input vector \a y whose elements are to be applied
 *                      to \a op as right-hand side input.
 * @param[in]    monoid The operator under which to perform this left-folding.
 *
 * @returns grb::SUCCESS This function always succeeds.
 *
 * \note This function is also defined for operators.
 *
 * \parblock
 * \par Valid descriptors
 * grb::descriptors::no_operation, grb::descriptors::no_casting.
 *
 * \note Invalid descriptors will be ignored.
 *
 * If grb::descriptors::no_casting is specified, then 1) the first domain of
 * \a op must match \a IOType, 2) the second domain of \a op must match
 * \a InputType, and 3) the third domain must match \a IOType. If one of these
 * is not true, the code shall not compile.
 *
 * \endparblock
 *
 * \parblock
 * \par Valid operator types
 * The given operator \a op is required to be:
 *   -# (no requirements).
 * \endparblock
 *
 * \parblock
 * \par Performance guarantees
 *      -# This call comprises \f$ \Theta(n) \f$ work, where \f$ n \f$ equals
 *         the size of the vector \a x. The constant factor depends on the
 *         cost of evaluating the underlying binary operator. A good
 *         implementation uses vectorised instructions whenever the input
 *         domains, the output domain, and the operator used allow for this.
 *
 *      -# This call will not result in additional dynamic memory allocations.
 *
 *      -# This call takes \f$ \mathcal{O}(1) \f$ memory beyond the memory
 *         used by the application at the point of a call to this function.
 *
 *      -# This call incurs at most
 *         \f$ n \cdot (
 *                \mathit{sizeof}(\mathit{IOType}) +
 *                \mathit{sizeof}(\mathit{InputType})
 *             ) + \mathcal{O}(1) \f$
 *         bytes of data movement. A good implementation will apply in-place
 *         vectorised instructions whenever the input domains, the output
 *         domain, and the operator used allow for this.
 * \endparblock
 *
 * @see grb::operators::internal::Operator for a discussion on when in-place
 *      and/or vectorised operations are used.
 */
template< Descriptor descr = descriptors::no_operation, class Monoid, typename IOType, typename InputType, typename Coords >
RC foldl( Vector< IOType, reference, Coords > & x,
	const Vector< InputType, reference, Coords > & y,
	const Monoid & monoid = Monoid(),
	const typename std::enable_if< grb::is_monoid< Monoid >::value && ! grb::is_object< IOType >::value && ! grb::is_object< InputType >::value, void >::type * = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Monoid::D1, IOType >::value ), "grb::foldl",
		"called with a vector x of a type that does not match the first domain "
		"of the given operator" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Monoid::D2, InputType >::value ), "grb::foldl",
		"called on a vector y of a type that does not match the second domain "
		"of the given operator" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Monoid::D3, IOType >::value ), "grb::foldl",
		"called on a vector x of a type that does not match the third domain "
		"of the given operator" );

	// dynamic sanity checks
	const size_t n = size( x );
	if( n != size( y ) ) {
		return MISMATCH;
	}

	// all OK, execute
	const Vector< bool, reference, Coords > * const null_mask = NULL;
	if( nnz( x ) < n || nnz( y ) < n ) {
		return internal::fold_from_vector_to_vector_generic< descr, true, true, false, true >( x, null_mask, y, monoid.getOperator() );
	} else {
		assert( nnz( x ) == n );
		assert( nnz( y ) == n );
		return internal::fold_from_vector_to_vector_generic< descr, true, false, false, true >( x, null_mask, y, monoid.getOperator() );
	}
}

/**
 * Computes \f$ y = y . x \f$ for a given operator.
 *
 * Masked variant.
 */
template< Descriptor descr = descriptors::no_operation, class OP, typename IOType, typename MaskType, typename InputType, typename Coords >
RC foldl( Vector< IOType, reference, Coords > & x,
	const Vector< MaskType, reference, Coords > & m,
	const Vector< InputType, reference, Coords > & y,
	const OP & op = OP(),
	const typename std::enable_if< grb::is_operator< OP >::value && ! grb::is_object< IOType >::value && ! grb::is_object< MaskType >::value && ! grb::is_object< InputType >::value, void >::type * =
		NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename OP::D1, IOType >::value ), "grb::foldl",
		"called with a vector x of a type that does not match the first domain "
		"of the given operator" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename OP::D2, InputType >::value ), "grb::foldl",
		"called on a vector y of a type that does not match the second domain "
		"of the given operator" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename OP::D3, IOType >::value ), "grb::foldl",
		"called on a vector x of a type that does not match the third domain "
		"of the given operator" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< bool, MaskType >::value ), "grb::foldl", "called with a mask that does not have boolean entries " );

	// catch empty mask
	if( size( m ) == 0 ) {
		return foldl< descr >( x, y, op );
	}

	// dynamic sanity checks
	const size_t n = size( x );
	if( n != size( y ) || n != size( m ) ) {
		return MISMATCH;
	}

	// all OK, execute
	if( nnz( x ) < n || nnz( y ) < n ) {
		return internal::fold_from_vector_to_vector_generic< descr, true, true, true, false >( x, &m, y, op );
	} else {
		assert( nnz( x ) == n );
		assert( nnz( y ) == n );
		return internal::fold_from_vector_to_vector_generic< descr, true, false, true, false >( x, &m, y, op );
	}
}

/**
 * Computes \f$ y = y + x \f$ for a given monoid.
 *
 * Masked variant.
 */
template< Descriptor descr = descriptors::no_operation, class Monoid, typename IOType, typename MaskType, typename InputType, typename Coords >
RC foldl( Vector< IOType, reference, Coords > & x,
	const Vector< MaskType, reference, Coords > & m,
	const Vector< InputType, reference, Coords > & y,
	const Monoid & monoid = Monoid(),
	const typename std::enable_if< grb::is_monoid< Monoid >::value && ! grb::is_object< IOType >::value && ! grb::is_object< MaskType >::value && ! grb::is_object< InputType >::value, void >::type * =
		NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Monoid::D1, IOType >::value ), "grb::foldl",
		"called with a vector x of a type that does not match the first domain "
		"of the given operator" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Monoid::D2, InputType >::value ), "grb::foldl",
		"called on a vector y of a type that does not match the second domain "
		"of the given operator" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Monoid::D3, IOType >::value ), "grb::foldl",
		"called on a vector x of a type that does not match the third domain "
		"of the given operator" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< bool, MaskType >::value ), "grb::foldl", "called with a mask that does not have boolean entries " );

	// catch empty mask
	if( size( m ) == 0 ) {
		return foldl< descr >( x, y, monoid );
	}

	// dynamic sanity checks
	const size_t n = size( x );
	if( n != size( y ) || n != size( m ) ) {
		return MISMATCH;
	}

	// all OK, execute
	if( nnz( x ) < n || nnz( y ) < n ) {
		return internal::fold_from_vector_to_vector_generic< descr, true, true, true, true >( x, &m, y, monoid.getOperator() );
	} else {
		assert( nnz( x ) == n );
		assert( nnz( y ) == n );
		return internal::fold_from_vector_to_vector_generic< descr, true, false, true, true >( x, &m, y, monoid.getOperator() );
	}
}

namespace internal {

	/**
	 * \internal eWiseApply of guaranteed complexity Theta(n) that generates dense
	 *           outputs.
	 */
	template< bool left_scalar, bool right_scalar, bool left_sparse, bool right_sparse, Descriptor descr, class OP, typename OutputType, typename InputType1, typename InputType2 >
	RC dense_apply_generic( OutputType * const z_p,
		const InputType1 * const x_p,
		const Coordinates< reference > * const x_coors,
		const InputType2 * const y_p,
		const Coordinates< reference > * const y_coors,
		const OP & op,
		const size_t n ) {
#ifdef _DEBUG
		std::cout << "\t internal::dense_apply_generic called\n";
#endif
		static_assert( ! ( left_scalar && left_sparse ), "The left-hand side must be scalar OR sparse, but cannot be both!" );
		static_assert( ! ( right_scalar && right_sparse ), "The right-hand side must be scalar OR sparse, but cannot be both!" );
		static_assert( ! ( left_sparse && right_sparse ), "If both left- and right-hand sides are sparse, use sparse_apply_generic instead." );
		assert( ! left_sparse || x_coors != NULL );
		assert( ! right_sparse || y_coors != NULL );

		constexpr const size_t block_size = OP::blocksize;
		const size_t num_blocks = n / block_size;

#ifndef _H_GRB_REFERENCE_OMP_BLAS1
#ifndef NDEBUG
		const bool has_coda = n % block_size > 0;
#endif
		size_t i = 0;
		const size_t start = 0;
		const size_t end = num_blocks;
#else
				#pragma omp parallel
				{
					size_t start, end;
					config::OMP::localRange( start, end, 0, num_blocks, config::CACHE_LINE_SIZE::value() / block_size );
#endif

		// declare and initialise local buffers for SIMD
		OutputType z_b[ block_size ];
		InputType1 x_b[ block_size ];
		InputType2 y_b[ block_size ];
		bool x_m[ block_size ];
		bool y_m[ block_size ];
		for( size_t k = 0; k < block_size; ++k ) {
			if( left_scalar ) {
				x_b[ k ] = *x_p;
			}
			if( right_scalar ) {
				y_b[ k ] = *y_p;
			}
		}

		for( size_t block = start; block < end; ++block ) {
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
			size_t i = block * block_size;
#endif
			size_t local_i = i;
			for( size_t k = 0; k < block_size; ++k ) {
				if( ! left_scalar ) {
					x_b[ k ] = x_p[ local_i ];
				}
				if( ! right_scalar ) {
					y_b[ k ] = y_p[ local_i ];
				}
				if( left_sparse ) {
					x_m[ k ] = x_coors->assigned( local_i );
				}
				if( right_sparse ) {
					y_m[ k ] = y_coors->assigned( local_i );
				}
				(void)++local_i;
			}
			for( size_t k = 0; k < block_size; ++k ) {
				RC rc = SUCCESS;
				if( left_sparse && ! x_m[ k ] ) {
					z_b[ k ] = y_b[ k ]; // WARNING: assumes monoid semantics!
				} else if( right_sparse && ! y_m[ k ] ) {
					z_b[ k ] = x_b[ k ]; // WARNING: assumes monoid semantics!
				} else {
					rc = apply( z_b[ k ], x_b[ k ], y_b[ k ], op );
				}
				assert( rc == SUCCESS );
#ifdef NDEBUG
				(void)rc;
#endif
			}
			for( size_t k = 0; k < block_size; ++k, ++i ) {
				z_p[ i ] = z_b[ k ];
			}
		}
#ifndef _H_GRB_REFERENCE_OMP_BLAS1
#ifndef NDEBUG
		if( has_coda ) {
			assert( i < n );
		} else {
			assert( i == n );
		}
#endif
#else
					size_t i = end * block_size;
					#pragma omp single
#endif
		for( ; i < n; ++i ) {
			RC rc = SUCCESS;
			if( left_scalar && right_scalar ) {
				rc = apply( z_p[ i ], *x_p, *y_p, op );
			} else if( left_scalar && ! right_scalar ) {
				if( right_sparse && ! ( y_coors->assigned( i ) ) ) {
					z_p[ i ] = *x_p;
				} else {
					rc = apply( z_p[ i ], *x_p, y_p[ i ], op );
				}
			} else if( ! left_scalar && right_scalar ) {
				if( left_sparse && ! ( x_coors->assigned( i ) ) ) {
					z_p[ i ] = *y_p;
				} else {
					rc = apply( z_p[ i ], x_p[ i ], *y_p, op );
				}
			} else {
				assert( ! left_scalar && ! right_scalar );
				if( left_sparse && ! ( x_coors->assigned( i ) ) ) {
					z_p[ i ] = y_p[ i ];
				} else if( right_sparse && ! ( y_coors->assigned( i ) ) ) {
					z_p[ i ] = x_p[ i ];
				} else {
					assert( ! left_sparse && ! right_sparse );
					rc = apply( z_p[ i ], x_p[ i ], y_p[ i ], op );
				}
			}
			assert( rc == SUCCESS );
#ifdef NDEBUG
			(void)rc;
#endif
		}
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
	} // end pragma omp parallel
#endif
	return SUCCESS;
} // namespace internal

/**
 * \internal Implements generic eWiseApply that loops over input vector(s) to
 *           generate a (likely) sparse output.
 */
template< bool masked, bool monoid, bool x_scalar, bool y_scalar, Descriptor descr, class OP, typename OutputType, typename MaskType, typename InputType1, typename InputType2 >
RC sparse_apply_generic( OutputType * const z_p,
	Coordinates< reference > & z_coors,
	const MaskType * const mask_p,
	const Coordinates< reference > * const mask_coors,
	const InputType1 * x_p,
	const Coordinates< reference > * const x_coors,
	const InputType2 * y_p,
	const Coordinates< reference > * const y_coors,
	const OP & op,
	const size_t n ) {
#ifdef NDEBUG
	(void)n;
#endif
	// assertions
	assert( ! masked || mask_coors != NULL );
	assert( ! masked || mask_coors->size() == n );
	assert( y_scalar || ( y_coors != NULL ) );
	assert( x_scalar || ( x_coors != NULL ) );
	assert( x_scalar || x_coors->nonzeroes() <= n );
	assert( y_scalar || y_coors->nonzeroes() <= n );

#ifdef _DEBUG
	std::cout << "\tinternal::sparse_apply_generic called\n";
#endif
	constexpr const size_t block_size = OP::blocksize;

	// swap so that we do the expensive pass over the container with the fewest
	// nonzeroes first
	assert( ! x_scalar || ! y_scalar );
	const bool swap = ( x_scalar ? n : x_coors->nonzeroes() ) > ( y_scalar ? n : y_coors->nonzeroes() );
	const Coordinates< reference > & loop_coors = swap ? *y_coors : *x_coors;
	const Coordinates< reference > & chk_coors = swap ? *x_coors : *y_coors;
#ifdef _DEBUG
	std::cout << "\t\tfirst-phase loop of size " << loop_coors.size() << "\n";
	if( x_scalar || y_scalar ) {
		std::cout << "\t\tthere will be no second phase because one of the inputs is scalar\n";
	} else {
		std::cout << "\t\tsecond-phase loop of size " << chk_coors.size() << "\n";
	}
#endif

#ifdef _H_GRB_REFERENCE_OMP_BLAS1
	#pragma omp parallel
	{
		const size_t maxAsyncAssigns = z_coors.maxAsyncAssigns();
		auto update = z_coors.EMPTY_UPDATE();
		size_t asyncAssigns = 0;
		assert( maxAsyncAssigns >= block_size );
#endif
		// declare buffers for vectorisation
		size_t offsets[ block_size ];
		OutputType z_b[ block_size ];
		InputType1 x_b[ block_size ];
		InputType2 y_b[ block_size ];
		bool mask[ block_size ];
		bool x_m[ block_size ];
		bool y_m[ block_size ];

		if( x_scalar ) {
			for( size_t k = 0; k < block_size; ++k ) {
				x_b[ k ] = *x_p;
			}
		}
		if( y_scalar ) {
			for( size_t k = 0; k < block_size; ++k ) {
				y_b[ k ] = *y_p;
			}
		}

		// expensive pass #1
		size_t start = 0;
		size_t end = loop_coors.nonzeroes() / block_size;
#ifndef _H_GRB_REFERENCE_OMP_BLAS1
		size_t k = 0;
#else
					// use dynamic schedule as the timings of gathers and scatters may vary significantly
					#pragma omp for schedule( dynamic, config::CACHE_LINE_SIZE::value() ) nowait
#endif
		for( size_t b = start; b < end; ++b ) {
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
			size_t k = b * block_size;
#endif
			// perform gathers
			for( size_t i = 0; i < block_size; ++i ) {
				const size_t index = loop_coors.index( k++ );
				offsets[ i ] = index;
				assert( index < n );
				if( masked ) {
					mask[ i ] = mask_coors->template mask< descr >( index, mask_p );
				}
			}
			// perform gathers
			for( size_t i = 0; i < block_size; ++i ) {
				if( ! masked || mask[ i ] ) {
					if( ! x_scalar ) {
						x_b[ i ] = x_p[ offsets[ i ] ];
					}
					if( ! x_scalar && ! y_scalar ) {
						y_m[ i ] = chk_coors.assigned( offsets[ i ] );
					} else {
						y_m[ i ] = true;
					}
					if( ! y_scalar ) {
						y_b[ i ] = y_p[ offsets[ i ] ];
					}
				} else {
					y_m[ i ] = false;
				}
			}
			// perform compute
			for( size_t i = 0; i < block_size; ++i ) {
				RC rc = SUCCESS;
				if( y_m[ i ] ) {
					rc = apply( z_b[ i ], x_b[ i ], y_b[ i ], op );
				} else if( monoid ) {
					if( swap ) {
						z_b[ i ] = static_cast< typename OP::D3 >( x_b[ i ] );
					} else {
						z_b[ i ] = static_cast< typename OP::D3 >( y_b[ i ] );
					}
				}
				assert( rc == SUCCESS );
#ifdef NDEBUG
				(void)rc;
#endif
			}
			// part that may or may not be vectorised (can we do something about this??)
			for( size_t i = 0; i < block_size; ++i ) {
				if( ! masked || mask[ i ] ) {
#ifndef _H_GRB_REFERENCE_OMP_BLAS1
					(void)z_coors.assign( offsets[ i ] );
#else
								if( ! z_coors.asyncAssign( offsets[ i ], update ) ) {
									(void)++asyncAssigns;
#ifdef _DEBUG
									std::cout << "\t\t now made " << asyncAssigns
											  << " calls to asyncAssign; added "
												 "index "
											  << offsets[ i ] << "\n";
#endif
								}
#endif
				}
			}
			// perform scatter
			for( size_t i = 0; i < block_size; ++i ) {
				if( ! masked || mask[ i ] ) {
					if( monoid || y_m[ i ] ) {
						z_p[ offsets[ i ] ] = z_b[ i ];
					}
				}
			}
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
			if( asyncAssigns > maxAsyncAssigns - block_size ) {
#ifdef _DEBUG
				std::cout << "\t\t " << omp_get_thread_num() << ": clearing local update at block " << b << ". It locally holds " << asyncAssigns << " entries. Update is at " << ( (void *)update )
						  << "\n";
#endif
#ifndef NDEBUG
				const bool was_empty =
#else
				(void)
#endif
					z_coors.joinUpdate( update );
				assert( ! was_empty );
				asyncAssigns = 0;
			}
#endif
		}
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
		// coda should be handled by a single thread
		#pragma omp single nowait
		{
			size_t k = end * block_size;
#endif
			for( ; k < loop_coors.nonzeroes(); ++k ) {
				const size_t index = loop_coors.index( k );
				if( masked && mask_coors->template mask< descr >( index, mask_p ) ) {
					continue;
				}
				RC rc = SUCCESS;
#ifndef _H_GRB_REFERENCE_OMP_BLAS1
				(void)z_coors.assign( index );
#else
						if( ! z_coors.asyncAssign( index, update ) ) {
							(void)++asyncAssigns;
						}
						if( asyncAssigns == maxAsyncAssigns ) {
#ifndef NDEBUG
							const bool was_empty =
#else
							(void)
#endif
								z_coors.joinUpdate( update );
							assert( ! was_empty );
							asyncAssigns = 0;
						}
#endif
				if( x_scalar || y_scalar || chk_coors.assigned( index ) ) {
					rc = apply( z_p[ index ], x_p[ index ], y_p[ index ], op );
				} else if( monoid ) {
					if( swap ) {
						z_p[ index ] = x_scalar ? static_cast< typename OP::D3 >( *x_p ) : static_cast< typename OP::D3 >( x_p[ index ] );
					} else {
						z_p[ index ] = y_scalar ? static_cast< typename OP::D3 >( *y_p ) : static_cast< typename OP::D3 >( y_p[ index ] );
					}
				}
				assert( rc == SUCCESS );
#ifdef NDEBUG
				(void)rc;
#endif
			}
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
		} // end omp single block
#endif

		// cheaper pass #2, only required if we are using monoid semantics
		// AND if both inputs are vectors
		if( monoid && ! x_scalar && ! y_scalar ) {
			start = 0;
			end = chk_coors.nonzeroes() / block_size;
#ifndef _H_GRB_REFERENCE_OMP_BLAS1
			k = 0;
#else
						#pragma omp for schedule( dynamic, config::CACHE_LINE_SIZE::value() ) nowait
#endif
			for( size_t b = start; b < end; ++b ) {
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
				size_t k = b * block_size;
#endif
				// streaming load
				for( size_t i = 0; i < block_size; i++ ) {
					offsets[ i ] = chk_coors.index( k++ );
					assert( offsets[ i ] < n );
				}
				// pure gather
				for( size_t i = 0; i < block_size; i++ ) {
					x_m[ i ] = loop_coors.assigned( offsets[ i ] );
				}
				// gather-like
				for( size_t i = 0; i < block_size; i++ ) {
					if( masked ) {
						mask[ i ] = utils::interpretMask< descr >( mask_coors->assigned( offsets[ i ] ), mask_p, offsets[ i ] );
					}
				}
				// SIMD
				for( size_t i = 0; i < block_size; i++ ) {
					x_m[ i ] = ! x_m[ i ];
				}
				// SIMD
				for( size_t i = 0; i < block_size; i++ ) {
					if( masked ) {
						mask[ i ] = mask[ i ] && x_m[ i ];
					}
				}
				if( ! swap ) {
					// gather
					for( size_t i = 0; i < block_size; ++i ) {
						if( masked ) {
							if( mask[ i ] ) {
								y_b[ i ] = y_p[ offsets[ i ] ];
							}
						} else {
							if( x_m[ i ] ) {
								y_b[ i ] = y_p[ offsets[ i ] ];
							}
						}
					}
					// SIMD
					for( size_t i = 0; i < block_size; i++ ) {
						if( masked ) {
							if( mask[ i ] ) {
								z_b[ i ] = y_b[ i ];
							}
						} else {
							if( x_m[ i ] ) {
								z_b[ i ] = y_b[ i ];
							}
						}
					}
				} else {
					// gather
					for( size_t i = 0; i < block_size; ++i ) {
						if( masked ) {
							if( mask[ i ] ) {
								x_b[ i ] = x_p[ offsets[ i ] ];
							}
						} else {
							if( x_m[ i ] ) {
								x_b[ i ] = x_p[ offsets[ i ] ];
							}
						}
					}
					// SIMD
					for( size_t i = 0; i < block_size; i++ ) {
						if( masked ) {
							if( mask[ i ] ) {
								z_b[ i ] = static_cast< typename OP::D3 >( x_b[ i ] );
							}
						} else {
							if( x_m[ i ] ) {
								z_b[ i ] = static_cast< typename OP::D3 >( x_b[ i ] );
							}
						}
					}
				}
				// SIMD-like
				for( size_t i = 0; i < block_size; i++ ) {
					if( masked ) {
						if( mask[ i ] ) {
#ifndef _H_GRB_REFERENCE_OMP_BLAS1
							(void)z_coors.assign( offsets[ i ] );
#else
										if( ! z_coors.asyncAssign( offsets[ i ], update ) ) {
											(void)++asyncAssigns;
										}
#endif
						}
					} else {
						if( x_m[ i ] ) {
#ifndef _H_GRB_REFERENCE_OMP_BLAS1
							(void)z_coors.assign( offsets[ i ] );
#else
										if( ! z_coors.asyncAssign( offsets[ i ], update ) ) {
											(void)++asyncAssigns;
										}
#endif
						}
					}
				}
				// scatter
				for( size_t i = 0; i < block_size; i++ ) {
					if( masked ) {
						if( mask[ i ] ) {
							z_p[ offsets[ i ] ] = z_b[ i ];
						}
					} else {
						if( x_m[ i ] ) {
#ifdef _DEBUG
							std::cout << "\t\t writing out " << z_b[ i ] << " to index " << offsets[ i ] << "\n";
#endif
							z_p[ offsets[ i ] ] = z_b[ i ];
						}
					}
				}
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
				if( asyncAssigns > maxAsyncAssigns - block_size ) {
#ifdef _DEBUG
					std::cout << "\t\t " << omp_get_thread_num() << ": clearing local update (2) at block " << b << ". It locally holds " << asyncAssigns << " entries. Update is at "
							  << ( (void *)update ) << "\n";
#endif
#ifndef NDEBUG
					const bool was_empty =
#else
					(void)
#endif
						z_coors.joinUpdate( update );
					assert( ! was_empty );
					asyncAssigns = 0;
				}
#endif
			}
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
			#pragma omp single nowait
			{
				size_t k = end * block_size;
#endif
				for( ; k < chk_coors.nonzeroes(); ++k ) {
					const size_t index = chk_coors.index( k );
					assert( index < n );
					if( loop_coors.assigned( index ) ) {
						continue;
					}
					if( masked && mask_coors->template mask< descr >( index, mask_p ) ) {
						continue;
					}
#ifndef _H_GRB_REFERENCE_OMP_BLAS1
					(void)z_coors.assign( index );
#else
							if( ! z_coors.asyncAssign( index, update ) ) {
								(void)++asyncAssigns;
							}
							if( asyncAssigns == maxAsyncAssigns ) {
#ifndef NDEBUG
								const bool was_empty =
#else
								(void)
#endif
									z_coors.joinUpdate( update );
								assert( ! was_empty );
							}
#endif
					z_p[ index ] = swap ? x_p[ index ] : y_p[ index ];
				}
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
			} // end pragma omp single
#endif
		}
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
#ifdef _DEBUG
		std::cout << "\t\t " << omp_get_thread_num()
				  << ": final local update clearing. It locally holds 0 "
					 "entries. Update is at "
				  << ( (void *)update ) << "\n";
#endif
		while( ! z_coors.joinUpdate( update ) ) {}
	} // end pragma omp parallel
#endif
	return SUCCESS;
}

template< bool left_scalar, bool right_scalar, bool left_sparse, bool right_sparse, Descriptor descr, class OP, typename OutputType, typename MaskType, typename InputType1, typename InputType2 >
RC masked_apply_generic( OutputType * const z_p,
	Coordinates< reference > & z_coors,
	const MaskType * const mask_p,
	const Coordinates< reference > & mask_coors,
	const InputType1 * const x_p,
	const InputType2 * const y_p,
	const OP & op,
	const size_t n,
	const Coordinates< reference > * const left_coors = NULL,
	const InputType1 * const left_identity = NULL,
	const Coordinates< reference > * const right_coors = NULL,
	const InputType2 * const right_identity = NULL ) {
#ifdef _DEBUG
	std::cout << "In masked_apply_generic< " << left_scalar << ", " << right_scalar << ", " << left_sparse << ", " << right_sparse << ", " << descr << " > with vector size " << n << "\n";
#endif
	// assertions
	static_assert( ! ( left_scalar && left_sparse ), "left_scalar and left_sparse cannot both be set!" );
	static_assert( ! ( right_scalar && right_sparse ), "right_scalar and right_sparse cannot both be set!" );
	assert( ! left_sparse || left_coors != NULL );
	assert( ! left_sparse || left_identity != NULL );
	assert( ! right_sparse || right_coors != NULL );
	assert( ! right_sparse || right_identity != NULL );

#ifdef _DEBUG
	std::cout << "\tinternal::masked_apply_generic called with nnz(mask)=" << mask_coors.nonzeroes() << " and descriptor " << descr << "\n";
	if( mask_coors.nonzeroes() > 0 ) {
		std::cout << "\t\tNonzero mask indices: " << mask_coors.index( 0 );
		assert( mask_coors.assigned( mask_coors.index( 0 ) ) );
		for( size_t k = 1; k < mask_coors.nonzeroes(); ++k ) {
			std::cout << ", " << mask_coors.index( k );
			assert( mask_coors.assigned( mask_coors.index( k ) ) );
		}
		std::cout << "\n";
	}
	size_t unset = 0;
	for( size_t i = 0; i < mask_coors.size(); ++i ) {
		if( ! mask_coors.assigned( i ) ) {
			(void)++unset;
		}
	}
	assert( unset == mask_coors.size() - mask_coors.nonzeroes() );
#endif
	// whether to use a Theta(n) or a Theta(nnz(mask)) loop
	const bool bigLoop = mask_coors.nonzeroes() == n || ( descr & descriptors::invert_mask );

	// get block size
	constexpr size_t size_t_block_size = config::SIMD_SIZE::value() / sizeof( size_t );
	constexpr size_t op_block_size = OP::blocksize;
	constexpr size_t min_block_size = op_block_size > size_t_block_size ? size_t_block_size : op_block_size;

	// whether we have a dense hint
	constexpr bool dense = descr & descriptors::dense;

	if( bigLoop ) {
#ifdef _DEBUG
		std::cerr << "\t in bigLoop variant\n";
#endif
#ifndef _H_GRB_REFERENCE_OMP_BLAS1
		size_t i = 0;
#else
						#pragma omp parallel
						{
#endif
		constexpr const size_t block_size = op_block_size;
		const size_t num_blocks = n / block_size;
		const size_t start = 0;
		const size_t end = num_blocks;

		// declare buffers that fit in a single SIMD register and initialise if needed
		bool mask_b[ block_size ];
		OutputType z_b[ block_size ];
		InputType1 x_b[ block_size ];
		InputType2 y_b[ block_size ];
		for( size_t k = 0; k < block_size; ++k ) {
			if( left_scalar ) {
				x_b[ k ] = *x_p;
			}
			if( right_scalar ) {
				y_b[ k ] = *y_p;
			}
		}

#ifdef _H_GRB_REFERENCE_OMP_BLAS1
		auto update = z_coors.EMPTY_UPDATE();
		size_t asyncAssigns = 0;
		const size_t maxAsyncAssigns = z_coors.maxAsyncAssigns();
		assert( maxAsyncAssigns >= block_size );
		#pragma omp for schedule( \
		dynamic, config::CACHE_LINE_SIZE::value() / block_size ) nowait
#endif
		// vectorised code
		for( size_t b = start; b < end; ++b ) {
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
			size_t i = start * block_size;
#endif
			for( size_t k = 0; k < block_size; ++k ) {
				const size_t index = i + k;
				assert( index < n );
				mask_b[ k ] = mask_coors.template mask< descr >( index, mask_p );
			}
			// check for no output
			if( left_sparse && right_sparse ) {
				for( size_t k = 0; k < block_size; ++k ) {
					const size_t index = i + k;
					assert( index < n );
					if( mask_b[ k ] ) {
						if( ! left_coors->assigned( index ) && ! right_coors->assigned( index ) ) {
							mask_b[ k ] = false;
						}
					}
				}
			}
			for( size_t k = 0; k < block_size; ++k ) {
				const size_t index = i + k;
				assert( index < n );
				if( mask_b[ k ] ) {
					if( ! left_scalar ) {
						if( left_sparse && ! left_coors->assigned( index ) ) {
							x_b[ k ] = *left_identity;
						} else {
							x_b[ k ] = *( x_p + index );
						}
					}
					if( ! right_scalar ) {
						if( right_sparse && ! right_coors->assigned( i + k ) ) {
							y_b[ k ] = *right_identity;
						} else {
							y_b[ k ] = *( y_p + index );
						}
					}
				}
			}
			for( size_t k = 0; k < block_size; ++k ) {
				if( mask_b[ k ] ) {
					apply( z_b[ k ], x_b[ k ], y_b[ k ], op );
				}
			}
			for( size_t k = 0; k < block_size; ++k ) {
				const size_t index = i + k;
				assert( index < n );
				if( mask_b[ k ] ) {
					if( ! dense ) {
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
						if( ! z_coors.asyncAssign( index, update ) ) {
							(void)++asyncAssigns;
						}
#else
											(void)z_coors.assign( index );
#endif
					}
					*( z_p + index ) = z_b[ k ];
				}
			}
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
			if( asyncAssigns > maxAsyncAssigns - block_size ) {
				(void)z_coors.joinUpdate( update );
				asyncAssigns = 0;
			}
#endif
			i += block_size;
		}
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
		#pragma omp single nowait
#endif
		// scalar coda
		for( size_t i = end * block_size; i < n; ++i ) {
			if( mask_coors.template mask< descr >( i, mask_p ) ) {
				if( ! dense ) {
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
					if( ! z_coors.asyncAssign( i, update ) ) {
						(void)++asyncAssigns;
					}
					if( asyncAssigns == maxAsyncAssigns ) {
						(void)z_coors.joinUpdate( update );
						asyncAssigns = 0;
					}
#else
										(void)z_coors.assign( i );
#endif
				}
				const InputType1 * const x_e = left_scalar ? x_p : ( ( ! left_sparse || left_coors->assigned( i ) ) ? x_p + i : left_identity );
				const InputType2 * const y_e = right_scalar ? y_p : ( ( ! right_sparse || right_coors->assigned( i ) ) ? y_p + i : right_identity );
				OutputType * const z_e = z_p + i;
				apply( *z_e, *x_e, *y_e, op );
			}
		}
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
		while( ! z_coors.joinUpdate( update ) ) {}
	} // end pragma omp parallel
#endif
}
else {
#ifdef _DEBUG
	std::cerr << "\t in smallLoop variant\n";
#endif
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
	#pragma omp parallel
	{
#endif
		// declare buffers that fit in a single SIMD register and initialise if needed
		constexpr const size_t block_size = size_t_block_size > 0 ? min_block_size : op_block_size;
		bool mask_b[ block_size ];
		OutputType z_b[ block_size ];
		InputType1 x_b[ block_size ];
		InputType2 y_b[ block_size ];
		size_t indices[ block_size ];
		for( size_t k = 0; k < block_size; ++k ) {
			if( left_scalar ) {
				x_b[ k ] = *x_p;
			}
			if( right_scalar ) {
				y_b[ k ] = *y_p;
			}
		}
		// loop over mask pattern
		const size_t mask_nnz = mask_coors.nonzeroes();
		const size_t num_blocks = mask_nnz / block_size;
		const size_t start = 0;
		const size_t end = num_blocks;
#ifndef _H_GRB_REFERENCE_OMP_BLAS1
		size_t k = 0;
#else
							auto update = z_coors.EMPTY_UPDATE();
							size_t asyncAssigns = 0;
							const size_t maxAsyncAssigns = z_coors.maxAsyncAssigns();
							assert( maxAsyncAssigns >= block_size );
							#pragma omp for schedule( \
		dynamic, config::CACHE_LINE_SIZE::value() / block_size ) nowait
#endif
		// vectorised code
		for( size_t b = start; b < end; ++b ) {
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
			size_t k = b * block_size;
#ifdef _DEBUG
			std::cout << "\t\t processing range " << k << "--" << ( k + block_size ) << "\n";
#endif
#endif
			for( size_t t = 0; t < block_size; ++t ) {
				indices[ t ] = mask_coors.index( k + t );
			}
			for( size_t t = 0; t < block_size; ++t ) {
				mask_b[ t ] = mask_coors.template mask< descr >( indices[ t ], mask_p );
			}
			for( size_t t = 0; t < block_size; ++t ) {
				if( mask_b[ t ] ) {
					if( ! left_scalar ) {
						if( left_sparse && ! left_coors->assigned( indices[ t ] ) ) {
							x_b[ t ] = *left_identity;
						} else {
							x_b[ t ] = *( x_p + indices[ t ] );
						}
					}
					if( ! right_scalar ) {
						if( right_sparse && ! right_coors->assigned( indices[ t ] ) ) {
							y_b[ t ] = *right_identity;
						} else {
							y_b[ t ] = *( y_p + indices[ t ] );
						}
					}
				}
			}
			// check for no output
			if( left_sparse && right_sparse ) {
				for( size_t t = 0; t < block_size; ++t ) {
					const size_t index = indices[ t ];
					assert( index < n );
					if( mask_b[ t ] ) {
						if( ! left_coors->assigned( index ) && ! right_coors->assigned( index ) ) {
							mask_b[ t ] = false;
						}
					}
				}
			}
			for( size_t t = 0; t < block_size; ++t ) {
				if( mask_b[ t ] ) {
					apply( z_b[ t ], x_b[ t ], y_b[ t ], op );
				}
			}
			for( size_t t = 0; t < block_size; ++t ) {
				if( mask_b[ t ] ) {
					if( ! dense ) {
#ifndef _H_GRB_REFERENCE_OMP_BLAS1
						(void)z_coors.assign( indices[ t ] );
#else
											if( ! z_coors.asyncAssign( indices[ t ], update ) ) {
												(void)++asyncAssigns;
#ifdef _DEBUG
												std::cout << "\t\t now made " << asyncAssigns
														  << " calls to asyncAssign; added "
															 "index "
														  << indices[ t ] << "\n";
#endif
											}
#endif
					}
					*( z_p + indices[ t ] ) = z_b[ t ];
				}
			}
#ifndef _H_GRB_REFERENCE_OMP_BLAS1
			k += block_size;
#else
								if( asyncAssigns > maxAsyncAssigns - block_size ) {
									(void)z_coors.joinUpdate( update );
									asyncAssigns = 0;
								}
#endif
		}
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
		#pragma omp single nowait
#endif
		// scalar coda
		for( size_t k = end * block_size; k < mask_nnz; ++k ) {
			const size_t i = mask_coors.index( k );
			if( mask_coors.template mask< descr >( i, mask_p ) ) {
				if( left_sparse && right_sparse ) {
					if( ! left_coors->assigned( i ) && ! right_coors->assigned( i ) ) {
						continue;
					}
				}
				if( ! dense ) {
#ifndef _H_GRB_REFERENCE_OMP_BLAS1
					(void)z_coors.assign( i );
#else
										if( ! z_coors.asyncAssign( i, update ) ) {
											(void)++asyncAssigns;
										}
										if( asyncAssigns == maxAsyncAssigns ) {
											(void)z_coors.joinUpdate( update );
											asyncAssigns = 0;
										}
#endif
				}
				const InputType1 * const x_e = left_scalar ? x_p : ( ( ! left_sparse || left_coors->assigned( i ) ) ? x_p + i : left_identity );
				const InputType2 * const y_e = right_scalar ? y_p : ( ( ! right_sparse || right_coors->assigned( i ) ) ? y_p + i : right_identity );
				OutputType * const z_e = z_p + i;
				apply( *z_e, *x_e, *y_e, op );
			}
		}
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
		while( ! z_coors.joinUpdate( update ) ) {}
	} // end pragma omp parallel
#endif
}
return SUCCESS;
} // namespace grb

} // end namespace ``grb::internal''

/**
 * Calculates the element-wise operation on one scalar to elements of one
 * vector, \f$ z = x .* \beta \f$, using the given operator. The input and
 * output vectors must be of equal length.
 *
 * The vectors \a x or \a y may not be sparse.
 *
 * For all valid indices \a i of \a z, its element \f$ z_i \f$ after
 * the call to this function completes equals \f$ x_i \odot \beta \f$.
 *
 * \warning Use of sparse vectors is only supported in full generality
 *          when applied via a monoid or semiring; otherwise, there is
 *          no concept for correctly interpreting any missing vector
 *          elements during the requested computation.
 * \note    When applying element-wise operators on sparse vectors
 *          using semirings, there is a difference between interpreting missing
 *          values as an annihilating identity or as a neutral identity--
 *          intuitively, such identities are known as `zero' or `one',
 *          respectively. As a consequence, there are three different variants
 *          for element-wise operations whose names correspond to their
 *          intuitive meanings w.r.t. those identities:
 *            -# eWiseAdd (neutral),
 *            -# eWiseMul (annihilating), and
 *            -# eWiseApply using monoids (neutral).
 *          An eWiseAdd with some semiring and an eWiseApply using its additive
 *          monoid are totally equivalent.
 *
 * @tparam descr      The descriptor to be used. Equal to
 *                    descriptors::no_operation if left unspecified.
 * @tparam OP         The operator to use.
 * @tparam InputType1 The value type of the left-hand vector.
 * @tparam InputType2 The value type of the right-hand scalar.
 * @tparam OutputType The value type of the ouput vector.
 *
 * @param[in]   x   The left-hand input vector.
 * @param[in]  beta The right-hand input scalar.
 * @param[out]  z   The pre-allocated output vector. If the allocation is
 *                  insufficient, the performance guarantees shall not be
 *                  binding.
 * @param[in]   op  The operator to use.
 *
 * @return grb::MISMATCH Whenever the dimensions of \a x and \a z do not
 *                       match. All input data containers are left untouched
 *                       if this exit code is returned; it will be as though
 *                       this call was never made.
 * @return grb::SUCCESS  On successful completion of this call.
 *
 * \parblock
 * \par Performance guarantees
 *      -# This call comprises \f$ \Theta(n) \f$ work, where \f$ n \f$ equals
 *         the size of the vectors \a x and \a z. The constant factor depends
 *         on the cost of evaluating the operator. A good implementation uses
 *         vectorised instructions whenever the input domains, the output
 *         domain, and the operator used allow for this.
 *
 *      -# This call takes \f$ \mathcal{O}(1) \f$ memory beyond the memory
 *         used by the application at the point of a call to this function.
 *
 *      -# This call incurs at most
 *         \f$ n(
 *               \mathit{sizeof}(\mathit{D1}) + \mathit{sizeof}(\mathit{D3})
 *             ) +
 *         \mathcal{O}(1) \f$
 *         bytes of data movement. A good implementation will stream \a y
 *         into \a z to apply the multiplication operator in-place, whenever
 *         the input domains, the output domain, and the operator allow for
 *         this.
 * \endparblock
 */
template< Descriptor descr = descriptors::no_operation, class OP, typename OutputType, typename InputType1, typename InputType2, typename Coords >
RC eWiseApply( Vector< OutputType, reference, Coords > & z,
	const Vector< InputType1, reference, Coords > & x,
	const InputType2 beta,
	const OP & op = OP(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && grb::is_operator< OP >::value,
		void >::type * const = NULL ) {
#ifdef _DEBUG
	std::cout << "In eWiseApply ([T1]<-[T2]<-T3), operator variant\n";
#endif

	// sanity check
	auto & z_coors = internal::getCoordinates( z );
	const size_t n = z_coors.size();
	if( internal::getCoordinates( x ).size() != n ) {
		return MISMATCH;
	}

	const Coords * const no_coordinates = NULL;
	if( nnz( x ) == nnz( z ) && nnz( x ) == n ) {
		// call dense apply
		z_coors.assignAll();
		return internal::dense_apply_generic< false, true, false, false, descr | descriptors::dense >( internal::getRaw( z ), internal::getRaw( x ), no_coordinates, &beta, no_coordinates, op, n );
	} else {
		// since z and x may not perfectly overlap, and since the intersection is
		// unknown a priori, we must iterate over the nonzeroes of x
		z_coors.clear();
		const bool * const null_mask = NULL;
		return internal::sparse_apply_generic< false, false, false, true, descr >(
			internal::getRaw( z ), internal::getCoordinates( z ), null_mask, no_coordinates, internal::getRaw( x ), &( internal::getCoordinates( x ) ), &beta, no_coordinates, op, n );
	}
}

/**
 * Computes \f$ z = x \odot y \f$, out of place.
 *
 * Specialisation for \a x and \a y scalar, operator version.
 */
template< Descriptor descr = descriptors::no_operation, class OP, typename OutputType, typename InputType1, typename InputType2, typename Coords >
RC eWiseApply( Vector< OutputType, reference, Coords > & z,
	const InputType1 alpha,
	const InputType2 beta,
	const OP & op = OP(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && grb::is_operator< OP >::value,
		void >::type * const = NULL ) {
#ifdef _DEBUG
	std::cout << "In eWiseApply ([T1]<-T2<-T3), operator variant\n";
#endif
	typename OP::D3 val;
	RC ret = apply< descr >( val, alpha, beta, op );
	ret = ret ? ret : set< descr >( z, val );
	return ret;
}

/**
 * Computes \f$ z = x \odot y \f$, out of place.
 *
 * Specialisation for \a x and \a y scalar, monoid version.
 */
template< Descriptor descr = descriptors::no_operation, class Monoid, typename OutputType, typename InputType1, typename InputType2, typename Coords >
RC eWiseApply( Vector< OutputType, reference, Coords > & z,
	const InputType1 alpha,
	const InputType2 beta,
	const Monoid & monoid = Monoid(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && grb::is_monoid< Monoid >::value,
		void >::type * const = NULL ) {
#ifdef _DEBUG
	std::cout << "In eWiseApply ([T1]<-T2<-T3), monoid variant\n";
#endif
	// simply delegate to operator variant
	return eWiseApply< descr >( z, alpha, beta, monoid.getOperator() );
}

/**
 * Computes \f$ z = x \odot y \f$, out of place.
 *
 * Specialisation for scalar \a y, masked operator version.
 */
template< Descriptor descr = descriptors::no_operation, class OP, typename OutputType, typename MaskType, typename InputType1, typename InputType2, typename Coords >
RC eWiseApply( Vector< OutputType, reference, Coords > & z,
	const Vector< MaskType, reference, Coords > & mask,
	const Vector< InputType1, reference, Coords > & x,
	const InputType2 beta,
	const OP & op = OP(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< MaskType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value &&
			grb::is_operator< OP >::value,
		void >::type * const = NULL ) {
#ifdef _DEBUG
	std::cout << "In masked eWiseApply ([T1]<-[T2]<-T3, using operator)\n";
#endif
	// check for empty mask
	if( size( mask ) == 0 ) {
		return eWiseApply< descr >( z, x, beta, op );
	}

	// other run-time checks
	const size_t n = internal::getCoordinates( z ).size();
	if( internal::getCoordinates( x ).size() != n ) {
		return MISMATCH;
	}
	if( internal::getCoordinates( mask ).size() != n ) {
		return MISMATCH;
	}

	// check if can delegate to unmasked
	const auto & mask_coors = internal::getCoordinates( mask );
	if( ( descr & descriptors::structural ) && ! ( descr & descriptors::invert_mask ) && mask_coors.nonzeroes() == n ) {
		return eWiseApply< descr >( z, x, beta, op );
	}

	auto & z_coors = internal::getCoordinates( z );
	OutputType * const z_p = internal::getRaw( z );
	const MaskType * const mask_p = internal::getRaw( mask );
	const InputType1 * const x_p = internal::getRaw( x );
	const auto & x_coors = internal::getCoordinates( x );

	// the output sparsity structure is implied by mask and descr
	z_coors.clear();

	if( ( descr & descriptors::dense ) || ( x_coors.nonzeroes() == n ) || ( mask_coors.nonzeroes() <= x_coors.nonzeroes() ) ) {
		return internal::masked_apply_generic< false, true, false, false, descr >( z_p, z_coors, mask_p, mask_coors, x_p, &beta, op, n );
	} else {
		const Coords * const null_coors = NULL;
		return internal::sparse_apply_generic< true, false, false, true, descr >( z_p, z_coors, mask_p, &mask_coors, x_p, &x_coors, &beta, null_coors, op, n );
	}
}

/**
 * Computes \f$ z = x \odot y \f$, out of place.
 *
 * Monoid version.
 */
template< Descriptor descr = descriptors::no_operation, class Monoid, typename OutputType, typename InputType1, typename InputType2, typename Coords >
RC eWiseApply( Vector< OutputType, reference, Coords > & z,
	const Vector< InputType1, reference, Coords > & x,
	const Vector< InputType2, reference, Coords > & y,
	const Monoid & monoid = Monoid(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && grb::is_monoid< Monoid >::value,
		void >::type * const = NULL ) {
#ifdef _DEBUG
	std::cout << "In unmasked eWiseApply ([T1]<-[T2]<-[T3], using monoid)\n";
#endif
	// other run-time checks
	const size_t n = internal::getCoordinates( z ).size();
	if( internal::getCoordinates( x ).size() != n ) {
		return MISMATCH;
	}
	if( internal::getCoordinates( y ).size() != n ) {
		return MISMATCH;
	}

	// check if we can dispatch to dense variant
	if( ( descr & descriptors::dense ) || ( grb::nnz( x ) == n && grb::nnz( y ) == n ) ) {
		return eWiseApply< descr >( z, x, y, monoid.getOperator() );
	}

	// we are in the unmasked sparse variant
	auto & z_coors = internal::getCoordinates( z );
	OutputType * const z_p = internal::getRaw( z );
	const InputType1 * const x_p = internal::getRaw( x );
	const InputType2 * const y_p = internal::getRaw( y );
	const auto & x_coors = internal::getCoordinates( x );
	const auto & y_coors = internal::getCoordinates( y );
	const auto op = monoid.getOperator();

	// z will have an a-priori unknown sparsity structure
	z_coors.clear();

	const bool * const null_mask = NULL;
	const Coords * const null_coors = NULL;
	return internal::sparse_apply_generic< false, true, false, false, descr >( z_p, z_coors, null_mask, null_coors, x_p, &( x_coors ), y_p, &( y_coors ), op, n );
}

/**
 * Computes \f$ z = x \odot y \f$, out of place.
 *
 * Specialisation for scalar \a x. Monoid version.
 */
template< Descriptor descr = descriptors::no_operation, class Monoid, typename OutputType, typename InputType1, typename InputType2, typename Coords >
RC eWiseApply( Vector< OutputType, reference, Coords > & z,
	const InputType1 alpha,
	const Vector< InputType2, reference, Coords > & y,
	const Monoid & monoid = Monoid(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && grb::is_monoid< Monoid >::value,
		void >::type * const = NULL ) {
#ifdef _DEBUG
	std::cout << "In unmasked eWiseApply ([T1]<-T2<-[T3], using monoid)\n";
#endif

	// other run-time checks
	const size_t n = internal::getCoordinates( z ).size();
	if( internal::getCoordinates( y ).size() != n ) {
		return MISMATCH;
	}

	// check if we can dispatch to dense variant
	if( ( descr && descriptors::dense ) || grb::nnz( y ) == n ) {
		return eWiseApply< descr >( z, alpha, y, monoid.getOperator() );
	}

	// we are in the unmasked sparse variant
	auto & z_coors = internal::getCoordinates( z );
	OutputType * const z_p = internal::getRaw( z );
	const InputType2 * const y_p = internal::getRaw( y );
	const auto & y_coors = internal::getCoordinates( y );
	const auto op = monoid.getOperator();

	// the result will always be dense
	if( z_coors.nonzeroes() < n ) {
		z_coors.assignAll();
	}

	// dispatch to generic function
	return internal::dense_apply_generic< true, false, false, true, descr >( z_p, &alpha, NULL, y_p, &y_coors, op, n );
}

/**
 * Computes \f$ z = x \odot y \f$, out of place.
 *
 * Specialisation for scalar \a y. Monoid version.
 */
template< Descriptor descr = descriptors::no_operation, class Monoid, typename OutputType, typename InputType1, typename InputType2, typename Coords >
RC eWiseApply( Vector< OutputType, reference, Coords > & z,
	const Vector< InputType1, reference, Coords > & x,
	const InputType2 beta,
	const Monoid & monoid = Monoid(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && grb::is_monoid< Monoid >::value,
		void >::type * const = NULL ) {
#ifdef _DEBUG
	std::cout << "In unmasked eWiseApply ([T1]<-[T2]<-T3, using monoid)\n";
#endif
	// other run-time checks
	const size_t n = internal::getCoordinates( z ).size();
	if( internal::getCoordinates( x ).size() != n ) {
		return MISMATCH;
	}

	// check if we can dispatch to dense variant
	if( ( descr & descriptors::dense ) || grb::nnz( x ) == n ) {
		return eWiseApply< descr >( z, x, beta, monoid.getOperator() );
	}

	// we are in the unmasked sparse variant
	auto & z_coors = internal::getCoordinates( z );
	OutputType * const z_p = internal::getRaw( z );
	const InputType1 * const x_p = internal::getRaw( x );
	const auto & x_coors = internal::getCoordinates( x );
	const auto op = monoid.getOperator();

	// the result will always be dense
	if( z_coors.nonzeroes() < n ) {
		z_coors.assignAll();
	}

	// dispatch
	return internal::dense_apply_generic< false, true, true, false, descr >( z_p, x_p, &x_coors, &beta, NULL, op, n );
}

/**
 * Computes \f$ z = x \odot y \f$, out of place.
 *
 * Masked monoid version.
 */
template< Descriptor descr = descriptors::no_operation, class Monoid, typename OutputType, typename MaskType, typename InputType1, typename InputType2, typename Coords >
RC eWiseApply( Vector< OutputType, reference, Coords > & z,
	const Vector< MaskType, reference, Coords > & mask,
	const Vector< InputType1, reference, Coords > & x,
	const Vector< InputType2, reference, Coords > & y,
	const Monoid & monoid = Monoid(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< MaskType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value &&
			grb::is_monoid< Monoid >::value,
		void >::type * const = NULL ) {
#ifdef _DEBUG
	std::cout << "In masked eWiseApply ([T1]<-[T2]<-[T3], using monoid)\n";
#endif
	if( size( mask ) == 0 ) {
		return eWiseApply< descr >( z, x, y, monoid );
	}

	// other run-time checks
	const size_t n = internal::getCoordinates( z ).size();
	if( internal::getCoordinates( x ).size() != n ) {
		return MISMATCH;
	}
	if( internal::getCoordinates( y ).size() != n ) {
		return MISMATCH;
	}
	if( internal::getCoordinates( mask ).size() != n ) {
		return MISMATCH;
	}

	// check if we can dispatch to dense variant
	if( ( descr & descriptors::dense ) || ( grb::nnz( x ) == n && grb::nnz( y ) == n ) ) {
		return eWiseApply< descr >( z, mask, x, y, monoid.getOperator() );
	}

	// we are in the masked sparse variant
	auto & z_coors = internal::getCoordinates( z );
	const auto & mask_coors = internal::getCoordinates( mask );
	OutputType * const z_p = internal::getRaw( z );
	const MaskType * const mask_p = internal::getRaw( mask );
	const InputType1 * const x_p = internal::getRaw( x );
	const InputType2 * const y_p = internal::getRaw( y );
	const auto & x_coors = internal::getCoordinates( x );
	const auto & y_coors = internal::getCoordinates( y );
	const InputType1 left_identity = monoid.template getIdentity< InputType1 >();
	const InputType2 right_identity = monoid.template getIdentity< InputType2 >();
	const auto op = monoid.getOperator();

	// z will have an a priori unknown sparsity structure
	z_coors.clear();

	if( grb::nnz( x ) < n && grb::nnz( y ) < n && grb::nnz( x ) + grb::nnz( y ) < grb::nnz( mask ) ) {
		return internal::sparse_apply_generic< true, true, false, false, descr >( z_p, z_coors, mask_p, &mask_coors, x_p, &( x_coors ), y_p, &( y_coors ), op, n );
	} else if( grb::nnz( x ) < n && grb::nnz( y ) == n ) {
		return internal::masked_apply_generic< false, false, true, false, descr, typename Monoid::Operator, OutputType, MaskType, InputType1, InputType2 >(
			z_p, z_coors, mask_p, mask_coors, x_p, y_p, op, n, &x_coors, &left_identity );
	} else if( grb::nnz( y ) < n && grb::nnz( x ) == n ) {
		return internal::masked_apply_generic< false, false, false, true, descr, typename Monoid::Operator, OutputType, MaskType, InputType1, InputType2 >(
			z_p, z_coors, mask_p, mask_coors, x_p, y_p, op, n, NULL, NULL, &y_coors, &right_identity );
	} else {
		return internal::masked_apply_generic< false, false, true, true, descr, typename Monoid::Operator, OutputType, MaskType, InputType1, InputType2 >(
			z_p, z_coors, mask_p, mask_coors, x_p, y_p, op, n, &x_coors, &left_identity, &y_coors, &right_identity );
	}
}

/**
 * Computes \f$ z = x \odot y \f$, out of place.
 *
 * Specialisation for scalar \a x. Masked monoid version.
 */
template< Descriptor descr = descriptors::no_operation, class Monoid, typename OutputType, typename MaskType, typename InputType1, typename InputType2, typename Coords >
RC eWiseApply( Vector< OutputType, reference, Coords > & z,
	const Vector< MaskType, reference, Coords > & mask,
	const InputType1 alpha,
	const Vector< InputType2, reference, Coords > & y,
	const Monoid & monoid = Monoid(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< MaskType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value &&
			grb::is_monoid< Monoid >::value,
		void >::type * const = NULL ) {
#ifdef _DEBUG
	std::cout << "In masked eWiseApply ([T1]<-T2<-[T3], using monoid)\n";
#endif
	if( size( mask ) == 0 ) {
		return eWiseApply< descr >( z, alpha, y, monoid );
	}

	// other run-time checks
	const size_t n = internal::getCoordinates( z ).size();
	if( internal::getCoordinates( y ).size() != n ) {
		return MISMATCH;
	}
	if( internal::getCoordinates( mask ).size() != n ) {
		return MISMATCH;
	}

	// check if we can dispatch to dense variant
	if( ( descr & descriptors::dense ) || grb::nnz( y ) == n ) {
		return eWiseApply< descr >( z, mask, alpha, y, monoid.getOperator() );
	}

	// we are in the masked sparse variant
	auto & z_coors = internal::getCoordinates( z );
	const auto & mask_coors = internal::getCoordinates( mask );
	OutputType * const z_p = internal::getRaw( z );
	const MaskType * const mask_p = internal::getRaw( mask );
	const InputType2 * const y_p = internal::getRaw( y );
	const auto & y_coors = internal::getCoordinates( y );
	const InputType2 right_identity = monoid.template getIdentity< InputType2 >();
	const auto op = monoid.getOperator();

	// the sparsity structure of z will be a result of the given mask and descr
	z_coors.clear();

	return internal::masked_apply_generic< true, false, false, true, descr, typename Monoid::Operator, OutputType, MaskType, InputType1, InputType2 >(
		z_p, z_coors, mask_p, mask_coors, &alpha, y_p, op, n, NULL, NULL, &y_coors, &right_identity );
}

/**
 * Computes \f$ z = x \odot y \f$, out of place.
 *
 * Specialisation for scalar \a y. Masked monoid version.
 */
template< Descriptor descr = descriptors::no_operation, class Monoid, typename OutputType, typename MaskType, typename InputType1, typename InputType2, typename Coords >
RC eWiseApply( Vector< OutputType, reference, Coords > & z,
	const Vector< MaskType, reference, Coords > & mask,
	const Vector< InputType1, reference, Coords > & x,
	const InputType2 beta,
	const Monoid & monoid = Monoid(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< MaskType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value &&
			grb::is_monoid< Monoid >::value,
		void >::type * const = NULL ) {
#ifdef _DEBUG
	std::cout << "In masked eWiseApply ([T1]<-[T2]<-T3, using monoid)\n";
#endif
	if( size( mask ) == 0 ) {
		return eWiseApply< descr >( z, x, beta, monoid );
	}

	// other run-time checks
	const size_t n = internal::getCoordinates( z ).size();
	if( internal::getCoordinates( x ).size() != n ) {
		return MISMATCH;
	}
	if( internal::getCoordinates( mask ).size() != n ) {
		return MISMATCH;
	}

	// check if we can dispatch to dense variant
	if( ( descr & descriptors::dense ) || grb::nnz( x ) == n ) {
		return eWiseApply< descr >( z, mask, x, beta, monoid.getOperator() );
	}

	// we are in the masked sparse variant
	auto & z_coors = internal::getCoordinates( z );
	const auto & mask_coors = internal::getCoordinates( mask );
	OutputType * const z_p = internal::getRaw( z );
	const MaskType * const mask_p = internal::getRaw( mask );
	const InputType1 * const x_p = internal::getRaw( x );
	const auto & x_coors = internal::getCoordinates( x );
	const InputType1 left_identity = monoid.template getIdentity< InputType1 >();
	const auto op = monoid.getOperator();

	// the sparsity structure of z will be the result of the given mask and descr
	z_coors.clear();

	return internal::masked_apply_generic< false, true, true, false, descr >( z_p, z_coors, mask_p, mask_coors, x_p, &beta, op, n, &x_coors, &left_identity );
}

/**
 * Calculates the element-wise operation on one scalar to elements of one
 * vector, \f$ z = \alpha .* y \f$, using the given operator. The input and
 * output vectors must be of equal length.
 *
 * The vectors \a x or \a y may not be sparse.
 *
 * For all valid indices \a i of \a z, its element \f$ z_i \f$ after
 * the call to this function completes equals \f$ \alpha \odot y_i \f$.
 *
 * \warning Use of sparse vectors is only supported in full generality
 *          when applied via a monoid or semiring; otherwise, there is
 *          no concept for correctly interpreting any missing vector
 *          elements during the requested computation.
 * \note    When applying element-wise operators on sparse vectors
 *          using semirings, there is a difference between interpreting missing
 *          values as an annihilating identity or as a neutral identity--
 *          intuitively, identities are known as `zero' or `one',
 *          respectively. As a consequence, there are three different variants
 *          for element-wise operations whose names correspond to their
 *          intuitive meanings w.r.t. those identities:
 *            -# eWiseAdd,
 *            -# eWiseMul, and
 *            -# eWiseMulAdd.
 *
 * @tparam descr The descriptor to be used. Equal to descriptors::no_operation
 *               if left unspecified.
 * @tparam OP    The operator to use.
 * @tparam InputType1 The value type of the left-hand scalar.
 * @tparam InputType2 The value type of the right-hand side vector.
 * @tparam OutputType The value type of the ouput vector.
 *
 * @param[in]  alpha The left-hand scalar.
 * @param[in]   y    The right-hand input vector.
 * @param[out]  z    The pre-allocated output vector. If the allocation is
 *                   insufficient, the performance guarantees shall not be
 *                   binding.
 * @param[in]   op   The operator to use.
 *
 * @return grb::MISMATCH Whenever the dimensions of \a y and \a z do not
 *                       match. All input data containers are left untouched
 *                       if this exit code is returned; it will be as though
 *                       this call was never made.
 * @return grb::SUCCESS  On successful completion of this call.
 *
 * \parblock
 * \par Performance guarantees
 *      -# This call comprises \f$ \Theta(n) \f$ work, where \f$ n \f$ equals
 *         the size of the vectors \a y and \a z. The constant factor depends
 *         on the cost of evaluating the operator. A good implementation uses
 *         vectorised instructions whenever the input domains, the output
 *         domain, and the operator used allow for this.
 *
 *      -# This call takes \f$ \mathcal{O}(1) \f$ memory beyond the memory
 *         used by the application at the point of a call to this function.
 *
 *      -# This call incurs at most
 *         \f$ n(
 *               \mathit{sizeof}(\mathit{D2}) + \mathit{sizeof}(\mathit{D3})
 *             ) +
 *         \mathcal{O}(1) \f$
 *         bytes of data movement. A good implementation will stream \a y
 *         into \a z to apply the multiplication operator in-place, whenever
 *         the input domains, the output domain, and the operator allow for
 *         this.
 * \endparblock
 *
 * \warning The above guarantees are only valid if the output parameter \a z
 *          has sufficient space reserved to store the output.
 */
template< Descriptor descr = descriptors::no_operation, class OP, typename OutputType, typename InputType1, typename InputType2, typename Coords >
RC eWiseApply( Vector< OutputType, reference, Coords > & z,
	const InputType1 alpha,
	const Vector< InputType2, reference, Coords > & y,
	const OP & op = OP(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && grb::is_operator< OP >::value,
		void >::type * const = NULL ) {
#ifdef _DEBUG
	std::cout << "In eWiseApply ([T1]<-T2<-[T3]), operator variant\n";
#endif
	// sanity check
	const size_t n = internal::getCoordinates( z ).size();
	if( internal::getCoordinates( y ).size() != n ) {
		return MISMATCH;
	}

	// check if we can dispatch
	if( static_cast< const void * >( &z ) == static_cast< const void * >( &y ) ) {
		return foldr< descr >( alpha, z, op );
	}

	// check for dense variant
	if( ( descr & descriptors::dense ) || internal::getCoordinates( y ).nonzeroes() == n ) {
		internal::getCoordinates( z ).assignAll();
		const internal::Coordinates< reference > * const no_coordinates = NULL;
		return internal::dense_apply_generic< true, false, false, false, descr >( internal::getRaw( z ), &alpha, no_coordinates, internal::getRaw( y ), no_coordinates, op, n );
	}

	// we are in the sparse variant
	const bool * const null_mask = NULL;
	const Coords * const null_coors = NULL;
	return internal::sparse_apply_generic< false, false, true, false, descr >(
		internal::getRaw( z ), internal::getCoordinates( z ), null_mask, null_coors, &alpha, null_coors, internal::getRaw( y ), &( internal::getCoordinates( y ) ), op, n );
}

/**
 * Computes \f$ z = x \odot y \f$, out of place.
 *
 * Specialisation for scalar \a x. Masked operator version.
 */
template< Descriptor descr = descriptors::no_operation, class OP, typename OutputType, typename MaskType, typename InputType1, typename InputType2, typename Coords >
RC eWiseApply( Vector< OutputType, reference, Coords > & z,
	const Vector< MaskType, reference, Coords > & mask,
	const InputType1 alpha,
	const Vector< InputType2, reference, Coords > & y,
	const OP & op = OP(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< MaskType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value &&
			grb::is_operator< OP >::value,
		void >::type * const = NULL ) {
#ifdef _DEBUG
	std::cout << "In masked eWiseApply ([T1]<-T2<-[T3], operator variant)\n";
#endif
	// check for empty mask
	if( size( mask ) == 0 ) {
		return eWiseApply< descr >( z, alpha, y, op );
	}

	// sanity check
	const size_t n = internal::getCoordinates( z ).size();
	if( internal::getCoordinates( y ).size() != n ) {
		return MISMATCH;
	}
	if( internal::getCoordinates( mask ).size() != n ) {
		return MISMATCH;
	}

	// check delegate to unmasked
	const auto & mask_coors = internal::getCoordinates( mask );
	if( ( descr & descriptors::structural ) && ! ( descr & descriptors::invert_mask ) && mask_coors.nonzeroes() == n ) {
		return eWiseApply< descr >( z, alpha, y, op );
	}

	auto & z_coors = internal::getCoordinates( z );
	OutputType * const z_p = internal::getRaw( z );
	const MaskType * const mask_p = internal::getRaw( mask );
	const InputType2 * const y_p = internal::getRaw( y );
	const auto & y_coors = internal::getCoordinates( y );

	// the output sparsity structure is implied by mask and descr
	z_coors.clear();

	if( ( descr & descriptors::dense ) || ( y_coors.nonzeroes() == n ) || mask_coors.nonzeroes() <= y_coors.nonzeroes() ) {
		return internal::masked_apply_generic< true, false, false, false, descr >( z_p, z_coors, mask_p, mask_coors, &alpha, y_p, op, n );
	} else {
		const Coords * const null_coors = NULL;
		return internal::sparse_apply_generic< true, false, true, false, descr >( z_p, z_coors, mask_p, &mask_coors, &alpha, null_coors, y_p, &y_coors, op, n );
	}
}

/**
 * Calculates the element-wise operation on elements of two vectors,
 * \f$ z = x .* y \f$, using the given operator. The vectors must be
 * of equal length.
 *
 * The vectors \a x or \a y may not be sparse.
 *
 * For all valid indices \a i of \a z, its element \f$ z_i \f$ after
 * the call to this function completes equals \f$ x_i \odot y_i \f$.
 *
 * \warning Use of sparse vectors is only supported in full generality
 *          when applied via a monoid or semiring; otherwise, there is
 *          no concept for correctly interpreting any missing vector
 *          elements during the requested computation.
 * \note    When applying element-wise operators on sparse vectors
 *          using semirings, there is a difference between interpreting missing
 *          values as an annihilating identity or as a neutral identity--
 *          intuitively, identities are known as `zero' or `one',
 *          respectively. As a consequence, there are three different variants
 *          for element-wise operations whose names correspond to their
 *          intuitive meanings w.r.t. those identities:
 *            -# eWiseAdd,
 *            -# eWiseMul, and
 *            -# eWiseMulAdd.
 *
 * @tparam descr The descriptor to be used (descriptors::no_operation if left
 *               unspecified).
 * @tparam OP    The operator to use.
 * @tparam InputType1 The value type of the left-hand side vector.
 * @tparam InputType2 The value type of the right-hand side vector.
 * @tparam OutputType The value type of the ouput vector.
 *
 * @param[in]  x  The left-hand input vector. May not equal \a y.
 * @param[in]  y  The right-hand input vector. May not equal \a x.
 * @param[out] z  The pre-allocated output vector. If the allocation is
 *                insufficient, the performance guarantees shall not be
 *                binding.
 * @param[in]  op The operator to use.
 *
 * @return grb::ILLEGAL  When \a x equals \a y.
 * @return grb::MISMATCH Whenever the dimensions of \a x, \a y, and \a z
 *                       do not match. All input data containers are left
 *                       untouched if this exit code is returned; it will
 *                       be as though this call was never made.
 * @return grb::SUCCESS  On successful completion of this call.
 *
 * \parblock
 * \par Performance guarantees
 *      -# This call comprises \f$ \Theta(n) \f$ work, where \f$ n \f$ equals
 *         the size of the vectors \a x, \a y, and \a z. The constant factor
 *         depends on the cost of evaluating the operator. A good
 *         implementation uses vectorised instructions whenever the input
 *         domains, the output domain, and the operator used allow for this.
 *
 *      -# This call takes \f$ \mathcal{O}(1) \f$ memory beyond the memory
 *         used by the application at the point of a call to this function.
 *
 *      -# This call incurs at most
 *         \f$ n(
 *               \mathit{sizeof}(\mathit{OutputType}) +
 *               \mathit{sizeof}(\mathit{InputType1}) +
 *               \mathit{sizeof}(\mathit{InputType2})
 *             ) +
 *         \mathcal{O}(1) \f$
 *         bytes of data movement. A good implementation will stream \a x or
 *         \a y into \a z to apply the multiplication operator in-place,
 *         whenever the input domains, the output domain, and the operator
 *         used allow for this.
 * \endparblock
 *
 * \warning The above guarantees are only valid if the output parameter \a z
 *          has sufficient space reserved to store the output.
 *
 * \note In case of dense vectors, this is always guaranteed.
 *
 * \warning It shall be illegal to take pointers of this function.
 */
template< Descriptor descr = descriptors::no_operation, class OP, typename OutputType, typename InputType1, typename InputType2, typename Coords >
RC eWiseApply( Vector< OutputType, reference, Coords > & z,
	const Vector< InputType1, reference, Coords > & x,
	const Vector< InputType2, reference, Coords > & y,
	const OP & op = OP(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && grb::is_operator< OP >::value,
		void >::type * const = NULL ) {
#ifdef _DEBUG
	std::cout << "In eWiseApply ([T1]<-[T2]<-[T3]), operator variant\n";
#endif
	// sanity check
	auto & z_coors = internal::getCoordinates( z );
	const size_t n = z_coors.size();
	if( internal::getCoordinates( x ).size() != n || internal::getCoordinates( y ).size() != n ) {
#ifdef _DEBUG
		std::cerr << "\tinput vectors mismatch in dimensions!\n";
#endif
		return MISMATCH;
	}

	// check for possible shortcuts
	if( static_cast< const void * >( &x ) == static_cast< const void * >( &y ) && is_idempotent< OP >::value ) {
		return set< descr >( z, x );
	}
	if( static_cast< const void * >( &x ) == static_cast< void * >( &z ) ) {
		return foldl< descr >( z, y, op );
	}
	if( static_cast< const void * >( &y ) == static_cast< void * >( &z ) ) {
		return foldr< descr >( x, z, op );
	}

	// check for sparsity
	if( ! ( descr & descriptors::dense ) && ( internal::getCoordinates( x ).nonzeroes() < n || internal::getCoordinates( y ).nonzeroes() < n ) ) {
		// sparse case
		z_coors.clear();
		const bool * const null_mask = NULL;
		const Coords * const null_coors = NULL;
		return internal::sparse_apply_generic< false, false, false, false, descr | descriptors::dense >(
			internal::getRaw( z ), z_coors, null_mask, null_coors, internal::getRaw( x ), &( internal::getCoordinates( x ) ), internal::getRaw( y ), &( internal::getCoordinates( y ) ), op, n );
	}

	// dense case
	if( internal::getCoordinates( z ).nonzeroes() < n ) {
		internal::getCoordinates( z ).assignAll();
	}

	const InputType1 * __restrict__ a = internal::getRaw( x );
	const InputType2 * __restrict__ b = internal::getRaw( y );
	OutputType * __restrict__ c = internal::getRaw( z );

	// no, so use eWiseApply
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
	#pragma omp parallel
#endif
	{
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
		size_t start, end;
		config::OMP::localRange( start, end, 0, n, OP::blocksize );
#else
						const size_t start = 0;
						const size_t end = n;
#endif
		if( end > start ) {
			op.eWiseApply( a + start, b + start, c + start, end - start ); // this function is vectorised
		}
	}

	// done
	return SUCCESS;
}

/**
 * Computes \f$ z = x \odot y \f$, out of place.
 *
 * Masked operator version.
 */
template< Descriptor descr = descriptors::no_operation, class OP, typename OutputType, typename MaskType, typename InputType1, typename InputType2, typename Coords >
RC eWiseApply( Vector< OutputType, reference, Coords > & z,
	const Vector< MaskType, reference, Coords > & mask,
	const Vector< InputType1, reference, Coords > & x,
	const Vector< InputType2, reference, Coords > & y,
	const OP & op = OP(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< MaskType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value &&
			grb::is_operator< OP >::value,
		void >::type * const = NULL ) {
#ifdef _DEBUG
	std::cout << "In masked eWiseApply ([T1]<-[T2]<-[T3], using operator)\n";
#endif
	// check for empty mask
	if( size( mask ) == 0 ) {
		return eWiseApply< descr >( z, x, y, op );
	}

	// other run-time checks
	auto & z_coors = internal::getCoordinates( z );
	const auto & mask_coors = internal::getCoordinates( mask );
	const size_t n = z_coors.size();
	if( internal::getCoordinates( x ).size() != n ) {
		return MISMATCH;
	}
	if( internal::getCoordinates( y ).size() != n ) {
		return MISMATCH;
	}
	if( mask_coors.size() != n ) {
		return MISMATCH;
	}

	OutputType * const z_p = internal::getRaw( z );
	const MaskType * const mask_p = internal::getRaw( mask );
	const InputType1 * const x_p = internal::getRaw( x );
	const InputType2 * const y_p = internal::getRaw( y );
	const auto & x_coors = internal::getCoordinates( x );
	const auto & y_coors = internal::getCoordinates( y );
	const auto & m_coors = internal::getCoordinates( mask );
	const size_t sparse_loop = std::min( x_coors.nonzeroes(), y_coors.nonzeroes() );

	// check if can delegate to unmasked variant
	if( m_coors.nonzeroes() == n && ( descr & descriptors::structural ) && ! ( descr & descriptors::invert_mask ) ) {
		return eWiseApply< descr >( z, x, y, op );
	}

	// the output sparsity structure is unknown a priori
	z_coors.clear();

	if( ( descr & descriptors::dense ) || ( x_coors.nonzeroes() == n && y_coors.nonzeroes() == n ) || ( ! ( descr & descriptors::invert_mask ) && sparse_loop >= m_coors.nonzeroes() ) ) {
		// use loop over mask
		return internal::masked_apply_generic< false, false, false, false, descr >( z_p, z_coors, mask_p, mask_coors, x_p, y_p, op, n );
	} else {
		// use loop over sparse inputs
		return internal::sparse_apply_generic< true, false, false, false, descr >( z_p, z_coors, mask_p, &mask_coors, x_p, &x_coors, y_p, &y_coors, op, n );
	}
}

/**
 * Calculates the element-wise addition of two vectors, \f$ z = x .+ y \f$,
 * under this semiring.
 *
 * @tparam descr      The descriptor to be used (descriptors::no_operation
 *                    if left unspecified).
 * @tparam Ring       The semiring type to perform the element-wise addition
 *                    on.
 * @tparam InputType1 The left-hand side input type to the additive operator
 *                    of the \a ring.
 * @tparam InputType2 The right-hand side input type to the additive operator
 *                    of the \a ring.
 * @tparam OutputType The the result type of the additive operator of the
 *                    \a ring.
 *
 * @param[out]  z  The output vector of type \a OutputType. This may be a
 *                 sparse vector.
 * @param[in]   x  The left-hand input vector of type \a InputType1. This may
 *                 be a sparse vector.
 * @param[in]   y  The right-hand input vector of type \a InputType2. This may
 *                 be a sparse vector.
 * @param[in] ring The generalized semiring under which to perform this
 *                 element-wise multiplication.
 *
 * @return grb::MISMATCH Whenever the dimensions of \a x, \a y, and \a z do
 *                       not match. All input data containers are left
 *                       untouched; it will be as though this call was never
 *                       made.
 * @return grb::SUCCESS  On successful completion of this call.
 *
 * \parblock
 * \par Valid descriptors
 * grb::descriptors::no_operation, grb::descriptors::no_casting,
 * grb::descriptors::dense.
 *
 * \note Invalid descriptors will be ignored.
 *
 * If grb::descriptors::no_casting is specified, then 1) the third domain of
 * \a ring must match \a InputType1, 2) the fourth domain of \a ring must match
 * \a InputType2, 3) the fourth domain of \a ring must match \a OutputType. If
 * one of these is not true, the code shall not compile.
 * \endparblock
 *
 * \parblock
 * \par Performance guarantees
 *      -# This call takes \f$ \Theta(n) \f$ work, where \f$ n \f$ equals the
 *         size of the vectors \a x, \a y, and \a z. The constant factor
 *         depends on the cost of evaluating the addition operator. A good
 *         implementation uses vectorised instructions whenever the input
 *         domains, the output domain, and the the additive operator used
 *         allow for this.
 *
 *      -# This call will not result in additional dynamic memory allocations.
 *         No system calls will be made.
 *
 *      -# This call takes \f$ \mathcal{O}(1) \f$ memory beyond the memory
 *         used by the application at the point of a call to this function.
 *
 *      -# This call incurs at most
 *         \f$ n( \mathit{sizeof}(
 *             \mathit{InputType1} +
 *             \mathit{InputType2} +
 *             \mathit{OutputType}
 *           ) + \mathcal{O}(1) \f$
 *         bytes of data movement. A good implementation will stream \a x or
 *         \a y into \a z to apply the additive operator in-place, whenever
 *         the input domains, the output domain, and the operator used allow
 *         for this.
 * \endparblock
 *
 * @see This is a specialised form of eWiseMulAdd.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename OutputType, typename InputType1, typename InputType2, typename Coords >
RC eWiseAdd( Vector< OutputType, reference, Coords > & z,
	const Vector< InputType1, reference, Coords > & x,
	const Vector< InputType2, reference, Coords > & y,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && grb::is_semiring< Ring >::value,
		void >::type * const = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, OutputType >::value ), "grb::eWiseAdd",
		"called with an output vector with element type that does not match the "
		"fourth domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D3, InputType1 >::value ), "grb::eWiseAdd",
		"called with a left-hand side input vector with element type that does not "
		"match the third domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, OutputType >::value ), "grb::eWiseAdd",
		"called with a right-hand side input vector with element type that does "
		"not match the fourth domain of the given semiring" );
#ifdef _DEBUG
	std::cout << "eWiseAdd (reference, vector <- vector + vector) dispatches to eWiseApply( reference, vector <- vector . vector ) using additive monoid\n";
#endif
	return eWiseApply< descr >( z, x, y, ring.getAdditiveMonoid() );
}

/**
 * Calculates the element-wise addition of two vectors, \f$ z = x .+ y \f$,
 * under the given semiring.
 *
 * Specialisation for scalar \a x.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename InputType1, typename InputType2, typename OutputType, typename Coords >
RC eWiseAdd( Vector< OutputType, reference, Coords > & z,
	const InputType1 alpha,
	const Vector< InputType2, reference, Coords > & y,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && grb::is_semiring< Ring >::value,
		void >::type * const = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseAdd",
		"called with a left-hand side input vector with element type that does not "
		"match the first domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseAdd",
		"called with a right-hand side input vector with element type that does "
		"not match the second domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D3, OutputType >::value ), "grb::eWiseAdd",
		"called with an output vector with element type that does not match the "
		"third domain of the given semiring" );
#ifdef _DEBUG
	std::cout << "eWiseAdd (reference, vector <- scalar + vector) dispatches to eWiseApply with additive monoid\n";
#endif
	return eWiseApply< descr >( z, alpha, y, ring.getAdditiveMonoid() );
}

/**
 * Calculates the element-wise addition of two vectors, \f$ z = x .+ y \f$,
 * under the given semiring.
 *
 * Specialisation for scalar \a y.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename InputType1, typename InputType2, typename OutputType, typename Coords >
RC eWiseAdd( Vector< OutputType, reference, Coords > & z,
	const Vector< InputType1, reference, Coords > & x,
	const InputType2 beta,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && grb::is_semiring< Ring >::value,
		void >::type * const = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseAdd",
		"called with a left-hand side input vector with element type that does not "
		"match the first domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseAdd",
		"called with a right-hand side input vector with element type that does "
		"not match the second domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D3, OutputType >::value ), "grb::eWiseAdd",
		"called with an output vector with element type that does not match the "
		"third domain of the given semiring" );
#ifdef _DEBUG
	std::cout << "eWiseAdd (reference, vector <- vector + scalar) dispatches to eWiseApply with additive monoid\n";
#endif
	return eWiseApply< descr >( z, x, beta, ring.getAdditiveMonoid() );
}

/**
 * Calculates the element-wise addition of two vectors, \f$ z = x .+ y \f$,
 * under the given semiring.
 *
 * Specialisation for scalar \a x and \a y.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename InputType1, typename InputType2, typename OutputType, typename Coords >
RC eWiseAdd( Vector< OutputType, reference, Coords > & z,
	const InputType1 alpha,
	const InputType2 beta,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && grb::is_semiring< Ring >::value,
		void >::type * const = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseAdd",
		"called with a left-hand side input vector with element type that does not "
		"match the first domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseAdd",
		"called with a right-hand side input vector with element type that does "
		"not match the second domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D3, OutputType >::value ), "grb::eWiseAdd",
		"called with an output vector with element type that does not match the "
		"third domain of the given semiring" );
#ifdef _DEBUG
	std::cout << "eWiseAdd (reference, vector <- scalar + scalar) dispatches to foldl with precomputed scalar and additive monoid\n";
#endif
	const typename Ring::D4 add;
	(void)apply( add, alpha, beta, ring.getAdditiveOperator() );
	return foldl< descr >( z, add, ring.getAdditiveMonoid() );
}

/**
 * Calculates the element-wise addition of two vectors, \f$ z = x .+ y \f$,
 * under the given semiring.
 *
 * Masked version.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename OutputType, typename MaskType, typename InputType1, typename InputType2, typename Coords >
RC eWiseAdd( Vector< OutputType, reference, Coords > & z,
	const Vector< MaskType, reference, Coords > & m,
	const Vector< InputType1, reference, Coords > & x,
	const Vector< InputType2, reference, Coords > & y,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< MaskType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value &&
			grb::is_semiring< Ring >::value,
		void >::type * const = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, OutputType >::value ), "grb::eWiseAdd",
		"called with an output vector with element type that does not match the "
		"fourth domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D3, InputType1 >::value ), "grb::eWiseAdd",
		"called with a left-hand side input vector with element type that does not "
		"match the third domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, OutputType >::value ), "grb::eWiseAdd",
		"called with a right-hand side input vector with element type that does "
		"not match the fourth domain of the given semiring" );
	NO_CAST_ASSERT(
		( ! ( descr & descriptors::no_casting ) || std::is_same< MaskType, bool >::value ), "grb::eWiseAdd (vector <- vector + vector, masked)", "called with non-bool mask element types" );
#ifdef _DEBUG
	std::cout << "eWiseAdd (reference, vector <- vector + vector, masked) dispatches to eWiseApply( reference, vector <- vector . vector ) using additive monoid\n";
#endif
	return eWiseApply< descr >( z, m, x, y, ring.getAdditiveMonoid() );
}

/**
 * Calculates the element-wise addition of two vectors, \f$ z = x .+ y \f$,
 * under the given semiring.
 *
 * Specialisation for scalar \a x, masked version
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename InputType1, typename InputType2, typename OutputType, typename MaskType, typename Coords >
RC eWiseAdd( Vector< OutputType, reference, Coords > & z,
	const Vector< MaskType, reference, Coords > & m,
	const InputType1 alpha,
	const Vector< InputType2, reference, Coords > & y,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< MaskType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value &&
			grb::is_semiring< Ring >::value,
		void >::type * const = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseAdd",
		"called with a left-hand side input vector with element type that does not "
		"match the first domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseAdd",
		"called with a right-hand side input vector with element type that does "
		"not match the second domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D3, OutputType >::value ), "grb::eWiseAdd",
		"called with an output vector with element type that does not match the "
		"third domain of the given semiring" );
	NO_CAST_ASSERT(
		( ! ( descr & descriptors::no_casting ) || std::is_same< MaskType, bool >::value ), "grb::eWiseAdd (vector <- scalar + vector, masked)", "called with non-bool mask element types" );
#ifdef _DEBUG
	std::cout << "eWiseAdd (reference, vector <- scalar + vector, masked) dispatches to eWiseApply with additive monoid\n";
#endif
	return eWiseApply< descr >( z, m, alpha, y, ring.getAdditiveMonoid() );
}

/**
 * Calculates the element-wise addition of two vectors, \f$ z = x .+ y \f$,
 * under the given semiring.
 *
 * Specialisation for scalar \a y, masked version.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename InputType1, typename InputType2, typename OutputType, typename MaskType, typename Coords >
RC eWiseAdd( Vector< OutputType, reference, Coords > & z,
	const Vector< MaskType, reference, Coords > & m,
	const Vector< InputType1, reference, Coords > & x,
	const InputType2 beta,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< MaskType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value &&
			grb::is_semiring< Ring >::value,
		void >::type * const = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseAdd",
		"called with a left-hand side input vector with element type that does not "
		"match the first domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseAdd",
		"called with a right-hand side input vector with element type that does "
		"not match the second domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D3, OutputType >::value ), "grb::eWiseAdd",
		"called with an output vector with element type that does not match the "
		"third domain of the given semiring" );
	NO_CAST_ASSERT(
		( ! ( descr & descriptors::no_casting ) || std::is_same< MaskType, bool >::value ), "grb::eWiseAdd (vector <- vector + scalar, masked)", "called with non-bool mask element types" );
#ifdef _DEBUG
	std::cout << "eWiseAdd (reference, vector <- vector + scalar, masked) dispatches to eWiseApply with additive monoid\n";
#endif
	return eWiseApply< descr >( z, m, x, beta, ring.getAdditiveMonoid() );
}

/**
 * Calculates the element-wise addition of two vectors, \f$ z = x .+ y \f$,
 * under the given semiring.
 *
 * Specialisation for scalar \a x and \a y, masked version.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename InputType1, typename InputType2, typename OutputType, typename MaskType, typename Coords >
RC eWiseAdd( Vector< OutputType, reference, Coords > & z,
	const Vector< OutputType, reference, Coords > & m,
	const InputType1 alpha,
	const InputType2 beta,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< MaskType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value &&
			grb::is_semiring< Ring >::value,
		void >::type * const = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseAdd",
		"called with a left-hand side input vector with element type that does not "
		"match the first domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseAdd",
		"called with a right-hand side input vector with element type that does "
		"not match the second domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D3, OutputType >::value ), "grb::eWiseAdd",
		"called with an output vector with element type that does not match the "
		"third domain of the given semiring" );
	NO_CAST_ASSERT(
		( ! ( descr & descriptors::no_casting ) || std::is_same< MaskType, bool >::value ), "grb::eWiseAdd (vector <- scalar + scalar, masked)", "called with non-bool mask element types" );
#ifdef _DEBUG
	std::cout << "eWiseAdd (reference, vector <- scalar + scalar, masked) dispatches to foldl with precomputed scalar and additive monoid\n";
#endif
	const typename Ring::D4 add;
	(void)apply( add, alpha, beta, ring.getAdditiveOperator() );
	return foldl< descr >( z, m, add, ring.getAdditiveMonoid() );
}

// declare an internal version of eWiseMulAdd containing the full sparse & dense implementations
namespace internal {

	template< Descriptor descr,
		bool a_scalar,
		bool x_scalar,
		bool y_scalar,
		bool z_assigned,
		typename OutputType,
		typename MaskType,
		typename InputType1,
		typename InputType2,
		typename InputType3,
		typename CoorsType,
		class Ring >
	RC sparse_eWiseMulAdd_maskDriven( Vector< OutputType, reference, CoorsType > & z_vector,
		const MaskType * __restrict__ m,
		const CoorsType * const m_coors,
		const InputType1 * __restrict__ a,
		const CoorsType * const a_coors,
		const InputType2 * __restrict__ x,
		const CoorsType * const x_coors,
		const InputType3 * __restrict__ y,
		const CoorsType * const y_coors,
		const size_t n,
		const Ring & ring ) {
		static_assert( ! ( descr & descriptors::invert_mask ), "Cannot loop over mask nonzeroes if invert_mask is given. Please submit a bug report" );
		static_assert( ! a_scalar || ! x_scalar, "If both a and x are scalars, this is operation is a simple eWiseApply with the additive operator if the semiring." );
		OutputType * __restrict__ z = internal::getRaw( z_vector );
		auto & z_coors = internal::getCoordinates( z_vector );
		assert( z != a );
		assert( z != x );
		assert( z != y );
		assert( a != x );
		assert( a != y );
		assert( x != y );
		assert( m_coors != NULL );
#ifdef NDEBUG
		(void)n;
#endif

#ifdef _H_GRB_REFERENCE_OMP_BLAS1
		#pragma omp parallel
		{
			size_t start, end;
			config::OMP::localRange( start, end, 0, m_coors->nonzeroes() );
#else
						const size_t start = 0;
						const size_t end = m_coors->nonzeroes();
#endif
			size_t k = start;

// vectorised code only for sequential backend
#ifndef _H_GRB_REFERENCE_OMP_BLAS1

			/*
			 * CODE DISABLED pending benchmarks to see whether it is worthwhile--
			 * see internal issue #163

			constexpr size_t size_t_bs = config::CACHE_LINE_SIZE::value() / sizeof(size_t) == 0 ? 1 : config::CACHE_LINE_SIZE::value() / sizeof(size_t);
			constexpr size_t blocksize = Ring::blocksize < size_t_bs ? Ring::blocksize : size_t_bs;

			// vector registers
			bool am[ blocksize ];
			bool xm[ blocksize ];
			bool ym[ blocksize ];
			bool mm[ blocksize ];
			bool zm[ blocksize ];
			size_t indices[ blocksize ];
			typename Ring::D1 aa[ blocksize ];
			typename Ring::D2 xx[ blocksize ];
			typename Ring::D3 tt[ blocksize ];
			typename Ring::D4 bb[ blocksize ];
			typename Ring::D4 yy[ blocksize ];
			typename Ring::D4 zz[ blocksize ];

			if( a_scalar ) {
			    for( size_t b = 0; b < Ring::blocksize; ++b ) {
			        aa[ b ] = static_cast< typename Ring::D1 >(*a);
			    }
			}
			if( x_scalar ) {
			    for( size_t b = 0; b < Ring::blocksize; ++b ) {
			        xx[ b ] = static_cast< typename Ring::D2 >(*x);
			    }
			}
			if( y_scalar ) {
			    for( size_t b = 0; b < Ring::blocksize; ++b ) {
			        yy[ b ] = static_cast< typename Ring::D4 >(*y);
			    }
			}

			// vectorised loop
			while( k + Ring::blocksize < end ) {
			    // set masks
			    for( size_t b = 0; b < blocksize; ++b ) {
			        am[ b ] = xm[ b ] = ym[ b ] = mm[ b ] = zm[ b ] = false;
			    }
			    // gathers
			    for( size_t b = 0; b < blocksize; ++b ) {
			        indices[ b ] = m_coors->index( k + b );
			    }
			    for( size_t b = 0; b < blocksize; ++b ) {
			        mm[ b ] = utils::interpretMask< descr >( true, m, indices[ b ] );
			    }
			    // masked gathers
			    for( size_t b = 0; b < blocksize; ++b ) {
			        if( mm[ b ] ) {
			            zm[ b ] = z_coors.assigned( indices[ b ] );
			        }
			    }
			    if( !a_scalar ) {
			        for( size_t b = 0; b < blocksize; ++b ) {
			            if( mm[ b ] ) {
			                am[ b ] = a_coors->assigned( indices[ b ] );
			            }
			        }
			    }
			    if( !y_scalar ) {
			        for( size_t b = 0; b < blocksize; ++b ) {
			            if( mm[ b ] ) {
			                ym[ b ] = y_coors->assigned( indices[ b ] );
			            }
			        }
			    }
			    if( !a_scalar ) {
			        for( size_t b = 0; b < blocksize; ++b ) {
			            if( am[ b ] ) {
			                aa[ b ] = static_cast< typename Ring::D1 >( a[ indices[ b ] ] );
			            }
			        }
			    }
			    if( !x_scalar ) {
			        for( size_t b = 0; b < blocksize; ++b ) {
			            if( xm[ b ] ) {
			                xx[ b ] = static_cast< typename Ring::D2 >( y[ indices[ b ] ] );
			            }
			        }
			    }
			    if( !y_scalar ) {
			        for( size_t b = 0; b < blocksize; ++b ) {
			            if( ym[ b ] ) {
			                yy[ b ] = static_cast< typename Ring::D4 >( y[ indices[ b ] ] );
			            }
			        }
			    }

			    // do multiplication
			    if( !a_scalar && !x_scalar ) {
			        for( size_t b = 0; b < blocksize; ++b ) {
			            mm[ b ] = am[ b ] && xm[ b ];
			        }
			    } else if( a_scalar ) {
			        for( size_t b = 0; b < blocksize; ++b ) {
			            mm[ b ] = xm[ b ];
			        }
			    } else {
			        for( size_t b = 0; b < blocksize; ++b ) {
			            mm[ b ] = am[ b ];
			        }
			    }
			    for( size_t b = 0; b < blocksize; ++b ) {
			        if( mm[ b ] ) {
			            (void) apply( tt[ b ], aa[ b ], xx[ b ], ring.getMultiplicativeOperator() );
			        }
			    }

			    // at this point am and xm are free to re-use

			    // do addition
			    for( size_t b = 0; b < blocksize; ++b ) {
			        xm[ b ] = mm[ b ] && !ym[ b ];
			    }
			    for( size_t b = 0; b < blocksize; ++b ) {
			        am[ b ] = !mm[ b ] && ym[ b ];
			    }
			    for( size_t b = 0; b < blocksize; ++b ) {
			        mm[ b ] = mm[ b ] && ym[ b ];
			    }
			    for( size_t b = 0; b < blocksize; ++b ) {
			        if( mm[ b ] ) {
			            (void) apply( bb[ b ], tt[ b ], yy[ b ], ring.getAdditiveOperator() );
			        }
			    }
			    for( size_t b = 0; b < blocksize; ++b ) {
			        if( xm[ b ] ) {
			            bb[ b ] = tt[ b ];
			        }
			    }
			    for( size_t b = 0; b < blocksize; ++b ) {
			        if( am[ b ] ) {
			            bb[ b ] = yy[ b ];
			        }
			    }

			    // accumulate into output
			    for( size_t b = 0; b < blocksize; ++b ) {
			        xm[ b ] = xm[ b ] || am[ b ];
			    }
			    for( size_t b = 0; b < blocksize; ++b ) {
			        mm[ b ] = mm[ b ] || xm[ b ];
			    }
			    for( size_t b = 0; b < blocksize; ++b ) {
			        am[ b ] = mm[ b ] && zm[ b ];
			    }
			    for( size_t b = 0; b < blocksize; ++b ) {
			        if( am[ b ] ) {
			            (void) foldr( bb[ b ], zz[ b ], ring.getAdditiveOperator() );
			        }
			    }
			    for( size_t b = 0; b < blocksize; ++b ) {
			        xm[ b ] = mm[ b ] && !zm[ b ];
			    }
			    for( size_t b = 0; b < blocksize; ++b ) {
			        if( xm[ b ] ) {
			            zz[ b ] = bb[ b ];
			        }
			    }

			    // scatter-like
			    for( size_t b = 0; b < blocksize; ++b ) {
			        if( xm[ b ] ) {
			            (void) z_coors.assign( indices[ b ] );
			        }
			    }

			    // scatter
			    for( size_t b = 0; b < blocksize; ++b ) {
			        zm[ b ] = zm[ b ] || mm[ b ];
			    }
			    for( size_t b = 0; b < blocksize; ++b ) {
			        if( zm[ b ] ) {
			            z[ indices[ b ] ] = static_cast< OutputType >( zz[ b ] );
			        }
			    }

			    // move to next block
			    k += blocksize;
			}*/
#endif
			// scalar coda and parallel main body
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
			internal::Coordinates< reference >::Update localUpdate = z_coors.EMPTY_UPDATE();
			const size_t maxAsyncAssigns = z_coors.maxAsyncAssigns();
			size_t asyncAssigns = 0;
#endif
			for( ; k < end; ++k ) {
				const size_t index = m_coors->index( k );
				assert( index < n );
				if( ! ( m_coors->template mask< descr >( index, m ) ) ) {
					continue;
				}
				typename Ring::D3 t = ring.template getZero< typename Ring::D3 >();
				if( ( a_scalar || a_coors->assigned( index ) ) && ( x_scalar || x_coors->assigned( index ) ) ) {
					const InputType1 * const a_p = a + ( a_scalar ? 0 : index );
					const InputType2 * const x_p = x + ( x_scalar ? 0 : index );
					(void)apply( t, *a_p, *x_p, ring.getMultiplicativeOperator() );
					if( y_scalar || y_coors->assigned( index ) ) {
						const InputType3 * const y_p = y + ( y_scalar ? 0 : index );
						typename Ring::D4 b;
						(void)apply( b, t, *y_p, ring.getAdditiveOperator() );
						if( z_coors.assigned( index ) ) {
							typename Ring::D4 out = static_cast< typename Ring::D4 >( z[ index ] );
							(void)foldr( b, out, ring.getAdditiveOperator() );
							z[ index ] = static_cast< OutputType >( out );
						} else {
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
							(void)z_coors.asyncAssign( index, localUpdate );
							(void)++asyncAssigns;
#else
										(void)z_coors.assign( index );
#endif
							z[ index ] = static_cast< OutputType >( b );
						}
					} else if( z_coors.assigned( index ) ) {
						typename Ring::D4 out = static_cast< typename Ring::D4 >( z[ index ] );
						(void)foldr( t, out, ring.getAdditiveOperator() );
						z[ index ] = static_cast< OutputType >( out );
					} else {
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
						(void)z_coors.asyncAssign( index, localUpdate );
						(void)++asyncAssigns;
#else
									(void)z_coors.assign( index );
#endif
						z[ index ] = static_cast< OutputType >( t );
					}
				} else if( y_coors->assigned( index ) ) {
					if( z_coors.assigned( index ) ) {
						typename Ring::D4 out = static_cast< typename Ring::D4 >( z[ index ] );
						(void)foldr( y[ index ], out, ring.getAdditiveOperator() );
						z[ index ] = static_cast< OutputType >( out );
					} else {
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
						(void)z_coors.asyncAssign( index, localUpdate );
						(void)++asyncAssigns;
#else
									(void)z_coors.assign( index );
#endif
						z[ index ] = static_cast< OutputType >( t );
					}
				}
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
				if( asyncAssigns == maxAsyncAssigns ) {
					const bool was_empty = z_coors.joinUpdate( localUpdate );
#ifdef NDEBUG
					(void)was_empty;
#else
					assert( ! was_empty );
#endif
					asyncAssigns = 0;
				}
#endif
			}
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
			while( ! z_coors.joinUpdate( localUpdate ) ) {}
		} // end pragma omp parallel
#endif
		return SUCCESS;
	}

	/**
	 * Call this version if consuming the multiplication first and performing the
	 * addition separately is cheaper than fusing the computations.
	 */
	template< Descriptor descr,
		bool masked,
		bool a_scalar,
		bool x_scalar,
		bool y_scalar,
		bool z_assigned,
		typename OutputType,
		typename MaskType,
		typename InputType1,
		typename InputType2,
		typename InputType3,
		typename CoorsType,
		class Ring >
	RC twoPhase_sparse_eWiseMulAdd_mulDriven( Vector< OutputType, reference, CoorsType > & z_vector,
		const Vector< MaskType, reference, CoorsType > * const m_vector,
		const InputType1 * __restrict__ a,
		const CoorsType * const a_coors,
		const InputType2 * __restrict__ x,
		const CoorsType * const x_coors,
		const Vector< InputType3, reference, CoorsType > * const y_vector,
		const InputType3 * __restrict__ y,
		const size_t n,
		const Ring & ring = Ring() ) {
		static_assert( ! a_scalar || ! x_scalar, "If both a and x are scalars, this is operation is a simple eWiseApply with the additive operator of the semiring." );
		InputType3 * __restrict__ z = internal::getRaw( z_vector );
		auto & z_coors = internal::getCoordinates( z_vector );
		assert( z != a );
		assert( z != x );
		assert( a != x );

		const size_t a_loop_size = a_scalar ? n : a_coors->nonzeroes();
		const size_t x_loop_size = x_scalar ? n : x_coors->nonzeroes();
		assert( a_loop_size < n || x_loop_size < n );
		const auto & it_coors = a_loop_size < x_loop_size ? *a_coors : *x_coors;
		const auto & ck_coors = a_loop_size < x_loop_size ? *x_coors : *a_coors;

#ifdef _H_GRB_REFERENCE_OMP_BLAS1
		#pragma omp parallel
		{
			auto localUpdate = z_coors.EMPTY_UPDATE();
			const size_t maxAsyncAssigns = z_coors.maxAsyncAssigns();
			size_t asyncAssigns = 0;
			#pragma omp for schedule( dynamic, config::CACHE_LINE_SIZE::value() ) nowait
#endif
			for( size_t i = 0; i < it_coors.nonzeroes(); ++i ) {
				const size_t index = it_coors.index( i );
				if( masked ) {
					const MaskType * __restrict__ const m = internal::getRaw( *m_vector );
					const CoorsType * const m_coors = &( internal::getCoordinates( *m_vector ) );
					if( ! m_coors->template mask< descr >( index, m ) ) {
						continue;
					}
				}
				if( ck_coors.assigned( index ) ) {
					typename Ring::D3 t;
					const InputType1 * const a_p = a_scalar ? a : a + index;
					const InputType2 * const x_p = x_scalar ? x : x + index;
					(void)apply( t, *a_p, *x_p, ring.getMultiplicativeOperator() );
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
					if( z_coors.asyncAssign( index, localUpdate ) ) {
#else
								if( z_coors.assign( index ) ) {
#endif
						typename Ring::D4 b = static_cast< typename Ring::D4 >( z[ index ] );
						(void)foldr( t, b, ring.getAdditiveOperator() );
						z[ index ] = static_cast< OutputType >( b );
					} else {
						z[ index ] = static_cast< OutputType >( static_cast< typename Ring::D4 >( t ) );
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
						(void)++asyncAssigns;
						if( asyncAssigns == maxAsyncAssigns ) {
							(void)z_coors.joinUpdate( localUpdate );
							asyncAssigns = 0;
						}
#endif
					}
				}
			}
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
			while( ! z_coors.joinUpdate( localUpdate ) ) {}
		}
#endif

		// now handle addition
		if( masked ) {
			if( y_scalar ) {
				return foldl< descr >( z_vector, *m_vector, *y, ring.getAdditiveMonoid() );
			} else {
				return foldl< descr >( z_vector, *m_vector, *y_vector, ring.getAdditiveMonoid() );
			}
		} else {
			if( y_scalar ) {
				return foldl< descr >( z_vector, *y, ring.getAdditiveMonoid() );
			} else {
				return foldl< descr >( z_vector, *y_vector, ring.getAdditiveMonoid() );
			}
		}
	}

	/** In this variant, all vector inputs (and output) is dense. */
	template< Descriptor descr,
		bool a_scalar,
		bool x_scalar,
		bool y_scalar,
		bool z_assigned,
		typename OutputType,
		typename InputType1,
		typename InputType2,
		typename InputType3,
		typename CoorsType,
		class Ring >
	RC dense_eWiseMulAdd( Vector< OutputType, reference, CoorsType > & z_vector,
		const InputType1 * __restrict__ const a_in,
		const InputType2 * __restrict__ const x_in,
		const InputType3 * __restrict__ const y_in,
		const size_t n,
		const Ring & ring = Ring() ) {
#ifdef _DEBUG
		std::cout << "\tdense_eWiseMulAdd: loop size will be " << n << "\n";
#endif
		OutputType * __restrict__ const z_in = internal::getRaw( z_vector );
		assert( z_in != a_in );
		assert( z_in != x_in );
		assert( z_in != y_in );
		assert( a_in != x_in );
		assert( a_in != y_in );
		assert( x_in != y_in );

#ifdef _H_GRB_REFERENCE_OMP_BLAS1
		#pragma omp parallel
		{
			size_t start, end;
			config::OMP::localRange( start, end, 0, n );
#else
						const size_t start = 0;
						const size_t end = n;
#endif
			// create local copies of the input const pointers
			const InputType1 * __restrict__ a = a_in;
			const InputType2 * __restrict__ x = x_in;
			const InputType3 * __restrict__ y = y_in;
			OutputType * __restrict__ z = z_in;

			// vector registers
			typename Ring::D1 aa[ Ring::blocksize ];
			typename Ring::D2 xx[ Ring::blocksize ];
			typename Ring::D3 tt[ Ring::blocksize ];
			typename Ring::D4 bb[ Ring::blocksize ];
			typename Ring::D4 yy[ Ring::blocksize ];
			typename Ring::D4 zz[ Ring::blocksize ];

			if( a_scalar ) {
				for( size_t b = 0; b < Ring::blocksize; ++b ) {
					aa[ b ] = *a;
				}
			}
			if( x_scalar ) {
				for( size_t b = 0; b < Ring::blocksize; ++b ) {
					xx[ b ] = *x;
				}
			}
			if( y_scalar ) {
				for( size_t b = 0; b < Ring::blocksize; ++b ) {
					yy[ b ] = *y;
				}
			}

			// do vectorised out-of-place operations. Allows for aligned overlap.
			// Non-aligned ovelap is not possible due to GraphBLAS semantics.
			size_t i = start;
			// note: read the tail code (under this while loop) comments first for greater understanding
			while( i + Ring::blocksize <= end ) {
#ifdef _DEBUG
				std::cout << "\tdense_eWiseMulAdd: handling block of size " << Ring::blocksize << " starting at index " << i << "\n";
#endif
				// read-in
				if( ! a_scalar ) {
					for( size_t b = 0; b < Ring::blocksize; ++b ) {
						aa[ b ] = static_cast< typename Ring::D2 >( a[ i + b ] );
					}
				}
				if( ! x_scalar ) {
					for( size_t b = 0; b < Ring::blocksize; ++b ) {
						xx[ b ] = static_cast< typename Ring::D2 >( x[ i + b ] );
					}
				}
				if( ! y_scalar ) {
					for( size_t b = 0; b < Ring::blocksize; ++b ) {
						yy[ b ] = static_cast< typename Ring::D4 >( y[ i + b ] );
					}
				}
				if( ! z_assigned ) {
					for( size_t b = 0; b < Ring::blocksize; ++b ) {
						zz[ b ] = static_cast< typename Ring::D4 >( z[ i + b ] );
					}
				}

				// operate
				for( size_t b = 0; b < Ring::blocksize; ++b ) {
					apply( tt[ b ], aa[ b ], xx[ b ], ring.getMultiplicativeOperator() );
					apply( bb[ b ], tt[ b ], yy[ b ], ring.getAdditiveOperator() );
				}
				if( ! z_assigned ) {
					for( size_t b = 0; b < Ring::blocksize; ++b ) {
						foldr( bb[ b ], zz[ b ], ring.getAdditiveOperator() );
					}
				}

				// write-out
				if( z_assigned ) {
					for( size_t b = 0; b < Ring::blocksize; ++b, ++i ) {
						z[ i ] = static_cast< OutputType >( bb[ b ] );
					}
				} else {
					for( size_t b = 0; b < Ring::blocksize; ++b, ++i ) {
						z[ i ] = static_cast< OutputType >( zz[ b ] );
					}
				}
			}

			// perform tail
			if( ! a_scalar ) {
				a += i;
			}
			if( ! x_scalar ) {
				x += i;
			}
			if( ! y_scalar ) {
				y += i;
			}
			z += i;
			for( ; i < end; ++i ) {
				// do multiply
				const typename Ring::D1 & as = static_cast< typename Ring::D1 >( *a );
				const typename Ring::D2 & xs = static_cast< typename Ring::D2 >( *x );
				typename Ring::D4 ys = static_cast< typename Ring::D4 >( *y );
				typename Ring::D3 ts;
#ifndef NDEBUG
				const RC always_succeeds =
#else
							(void)
#endif
					apply( ts, as, xs, ring.getMultiplicativeOperator() );
				assert( always_succeeds == SUCCESS );

				// do add
				foldr( ts, ys, ring.getAdditiveOperator() );

				// write out
				if( z_assigned ) {
					*z = static_cast< OutputType >( ys );
				} else {
					foldr( ys, *z, ring.getAdditiveOperator() );
				}

				// move pointers
				if( ! a_scalar ) {
					(void)a++;
				}
				if( ! x_scalar ) {
					(void)x++;
				}
				if( ! y_scalar ) {
					(void)y++;
				}
				(void)z++;
			}
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
		} // end OpenMP parallel section
#endif

		// done
		return SUCCESS;
	}

	/**
	 * Depending on sparsity patterns, there are many variants of eWiseMulAdd.
	 * This function identifies and calls the most opportune variants.
	 */
	template< Descriptor descr,
		bool masked,
		bool a_scalar,
		bool x_scalar,
		bool y_scalar,
		typename MaskType,
		class Ring,
		typename InputType1,
		typename InputType2,
		typename InputType3,
		typename OutputType,
		typename CoorsType >
	RC eWiseMulAdd_dispatch( Vector< OutputType, reference, CoorsType > & z_vector,
		const Vector< MaskType, reference, CoorsType > * const m_vector,
		const InputType1 * __restrict__ a,
		const CoorsType * const a_coors,
		const InputType2 * __restrict__ x,
		const CoorsType * const x_coors,
		const Vector< InputType3, reference, CoorsType > * const y_vector,
		const InputType3 * __restrict__ y,
		const CoorsType * const y_coors,
		const size_t n,
		const Ring & ring ) {
		const MaskType * __restrict__ m = NULL;
		const CoorsType * m_coors = NULL;
		assert( ! masked || ( m_vector != NULL ) );
		if( masked ) {
			m = internal::getRaw( *m_vector );
			m_coors = &( internal::getCoordinates( *m_vector ) );
		}
		assert( ! masked || ( m_coors != NULL ) );
		assert( ! a_scalar || ( a_coors == NULL ) );
		assert( ! x_scalar || ( x_coors == NULL ) );
		assert( ! y_scalar || ( y_coors == NULL ) );

		// check whether we are in the sparse or dense case
		constexpr bool dense = ( descr & descriptors::dense );
		const bool mask_is_dense = ! masked || ( ( descr & descriptors::structural ) && ! ( descr & descriptors::invert_mask ) && m_coors->nonzeroes() == n );
		const size_t z_nns = nnz( z_vector );
		const bool sparse = ( a_scalar ? false : ( a_coors->nonzeroes() < n ) ) || ( x_scalar ? false : ( x_coors->nonzeroes() < n ) ) || ( y_scalar ? false : ( y_coors->nonzeroes() < n ) ) ||
			( z_nns > 0 && z_nns < n ) || ( masked && ! mask_is_dense );
		if( dense && sparse ) {
			std::cerr << "Warning: call to grb::eWiseMulAdd (reference) with a "
						 "dense descriptor but sparse input arguments. "
						 "Returning ILLEGAL\n";
			return ILLEGAL;
		}

		// pre-assign coors if output is dense but previously empty
		const bool z_assigned = z_nns == 0 && ( y_scalar || y_coors->nonzeroes() == n ) && ( ! masked || mask_is_dense );
		if( z_assigned ) {
#ifdef _DEBUG
			std::cout << "\teWiseMulAdd_dispatch: detected output will be dense, pre-assigning all output coordinates\n";
#endif
			internal::getCoordinates( z_vector ).assignAll();
		}

		if( ! dense && sparse ) {
			// the below computes loop sizes multiplied with the number of vectors that
			// each loop needs to touch. Possible vectors are: z, m, a, x, and y.
			const size_t mask_factor = masked ? 1 : 0;
			const size_t mul_loop_size = ( 3 + mask_factor ) * std::min( ( a_scalar ? n : a_coors->nonzeroes() ), ( x_scalar ? n : x_coors->nonzeroes() ) ) +
				( 2 + mask_factor ) * ( y_scalar ? n : y_coors->nonzeroes() );
			/** See internal issue #42 (closed): this variant, in a worst-case analysis
			 * is never preferred:
			const size_t add_loop_size = (4 + mask_factor) *
			        (y_scalar ? n : y_coors->nonzeroes()) +
			    (4 + mask_factor) * std::min(
			        (a_scalar ? n : a_coors->nonzeroes()),
			        (x_scalar ? n : x_coors->nonzeroes())
			    ) // min is worst-case, best case is 0, realistic is some a priori unknown
			      // problem-dependent overlap
			;*/
#ifdef _DEBUG
			std::cout << "\t\teWiseMulAdd_dispatch: mul_loop_size = " << mul_loop_size << "\n";
			// internal issue #42, (closed):
			// std::cout << "\t\teWiseMulAdd_dispatch: add_loop_size = " << add_loop_size << "\n";
#endif
			if( masked ) {
				const size_t mask_loop_size = 5 * m_coors->nonzeroes();
#ifdef _DEBUG
				std::cout << "\t\teWiseMulAdd_dispatch: mask_loop_size= " << mask_loop_size << "\n";
#endif
				// internal issue #42 (closed):
				// if( mask_loop_size < mul_loop_size && mask_loop_size < add_loop_size ) {
				if( mask_loop_size < mul_loop_size ) {
#ifdef _DEBUG
					std::cout << "\teWiseMulAdd_dispatch: will be driven by output mask\n";
#endif
					if( z_assigned ) {
						return sparse_eWiseMulAdd_maskDriven< descr, a_scalar, x_scalar, y_scalar, true >( z_vector, m, m_coors, a, a_coors, x, x_coors, y, y_coors, n, ring );
					} else {
						return sparse_eWiseMulAdd_maskDriven< descr, a_scalar, x_scalar, y_scalar, false >( z_vector, m, m_coors, a, a_coors, x, x_coors, y, y_coors, n, ring );
					}
				}
			}
			// internal issue #42 (closed):
			// if( mul_loop_size < add_loop_size ) {
#ifdef _DEBUG
			std::cout << "\teWiseMulAdd_dispatch: will be driven by the multiplication a*x\n";
#endif
			if( z_assigned ) {
				return twoPhase_sparse_eWiseMulAdd_mulDriven< descr, masked, a_scalar, x_scalar, y_scalar, true >( z_vector, m_vector, a, a_coors, x, x_coors, y_vector, y, n, ring );
			} else {
				return twoPhase_sparse_eWiseMulAdd_mulDriven< descr, masked, a_scalar, x_scalar, y_scalar, false >( z_vector, m_vector, a, a_coors, x, x_coors, y_vector, y, n, ring );
			}
			/* internal issue #42 (closed):
			} else {
#ifdef _DEBUG
			    std::cout << "\teWiseMulAdd_dispatch: will be driven by the addition with y\n";
#endif
			    if( z_assigned ) {
			        return twoPhase_sparse_eWiseMulAdd_addPhase1< descr, masked, a_scalar, x_scalar, y_scalar, true >(
			            z_vector,
			            m, m_coors,
			            a, a_coors,
			            x, x_coors,
			            y, y_coors,
			            n, ring
			        );
			    } else {
			        return twoPhase_sparse_eWiseMulAdd_addPhase1< descr, masked, a_scalar, x_scalar, y_scalar, false >(
			            z_vector,
			            m, m_coors,
			            a, a_coors,
			            x, x_coors,
			            y, y_coors,
			            n, ring
			        );
			    }
			}*/
		}

		// all that remains is the dense case
		assert( a_scalar || a_coors->nonzeroes() == n );
		assert( x_scalar || x_coors->nonzeroes() == n );
		assert( y_scalar || y_coors->nonzeroes() == n );
		assert( ! masked || mask_is_dense );
		assert( internal::getCoordinates( z_vector ).nonzeroes() == n );
#ifdef _DEBUG
		std::cout << "\teWiseMulAdd_dispatch: will perform a dense eWiseMulAdd\n";
#endif
		if( z_assigned ) {
			return dense_eWiseMulAdd< descr, a_scalar, x_scalar, y_scalar, true >( z_vector, a, x, y, n, ring );
		} else {
			return dense_eWiseMulAdd< descr, a_scalar, x_scalar, y_scalar, false >( z_vector, a, x, y, n, ring );
		}
	}

} // namespace internal

/**
 * Calculates the axpy, \f$ z = a * x .+ y \f$, under this semiring.
 *
 * Specialisation for when \a a is a scalar.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename InputType1, typename InputType2, typename InputType3, typename OutputType, typename Coords >
RC eWiseMulAdd( Vector< OutputType, reference, Coords > & _z,
	const InputType1 alpha,
	const Vector< InputType2, reference, Coords > & _x,
	const Vector< InputType3, reference, Coords > & _y,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && ! grb::is_object< InputType3 >::value &&
			grb::is_semiring< Ring >::value,
		void >::type * const = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseMulAdd",
		"called with a left-hand scalar alpha of an element type that does not "
		"match the first domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseMulAdd",
		"called with a right-hand vector _x with an element type that does not "
		"match the second domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, InputType3 >::value ), "grb::eWiseMulAdd",
		"called with an additive vector _y with an element type that does not "
		"match the fourth domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, OutputType >::value ), "grb::eWiseMulAdd",
		"called with a result vector _z with an element type that does not match "
		"the fourth domain of the given semiring" );

	// dynamic sanity checks
	const size_t n = internal::getCoordinates( _z ).size();
	if( internal::getCoordinates( _x ).size() != n || internal::getCoordinates( _y ).size() != n ) {
		return MISMATCH;
	}

	// catch trivial cases
	const InputType1 zeroIT1 = ring.template getZero< InputType1 >();
	if( alpha == zeroIT1 || internal::getCoordinates( _x ).nonzeroes() == 0 ) {
		return foldl< descr >( _z, _y, ring.getAdditiveMonoid() );
	}
	if( internal::getCoordinates( _y ).nonzeroes() == 0 ) {
		return eWiseMulAdd< descr >( _z, alpha, _x, ring.template getZero< typename Ring::D4 >(), ring );
	}

	// check for density
	const Vector< bool, reference, Coords > * const null_mask = NULL;
	auto null_coors = &( internal::getCoordinates( _x ) );
	null_coors = NULL;
	constexpr bool maybe_sparse = ! ( descr & descriptors::dense );
	if( maybe_sparse ) {
		// check whether all inputs are actually dense
		if( internal::getCoordinates( _x ).nonzeroes() == n && internal::getCoordinates( _y ).nonzeroes() == n ) {
			// yes, dispatch to version with dense descriptor set
			return internal::eWiseMulAdd_dispatch< descr | descriptors::dense, false, true, false, false >(
				_z, null_mask, &alpha, null_coors, internal::getRaw( _x ), &( internal::getCoordinates( _x ) ), &_y, internal::getRaw( _y ), &( internal::getCoordinates( _y ) ), n, ring );
		}
	}

	// sparse or dense case
	return internal::eWiseMulAdd_dispatch< descr, false, true, false, false >(
		_z, null_mask, &alpha, null_coors, internal::getRaw( _x ), &( internal::getCoordinates( _x ) ), &_y, internal::getRaw( _y ), &( internal::getCoordinates( _y ) ), n, ring );
}

/**
 * Calculates the elementwise multiply-add, \f$ z = a .* x .+ y \f$, under
 * this semiring.
 *
 * Specialisation for when \a x is a scalar.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename InputType1, typename InputType2, typename InputType3, typename OutputType, typename Coords >
RC eWiseMulAdd( Vector< OutputType, reference, Coords > & _z,
	const Vector< InputType1, reference, Coords > & _a,
	const InputType2 chi,
	const Vector< InputType3, reference, Coords > & _y,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && ! grb::is_object< InputType3 >::value &&
			grb::is_semiring< Ring >::value,
		void >::type * const = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseMulAdd",
		"called with a left-hand scalar alpha of an element type that does not "
		"match the first domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseMulAdd",
		"called with a right-hand vector _x with an element type that does not "
		"match the second domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, InputType3 >::value ), "grb::eWiseMulAdd",
		"called with an additive vector _y with an element type that does not "
		"match the fourth domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, OutputType >::value ), "grb::eWiseMulAdd",
		"called with a result vector _z with an element type that does not match "
		"the fourth domain of the given semiring" );

	// dynamic sanity checks
	const size_t n = internal::getCoordinates( _z ).size();
	if( internal::getCoordinates( _a ).size() != n || internal::getCoordinates( _y ).size() != n ) {
		return MISMATCH;
	}

	// catch trivial cases
	const InputType1 zeroIT2 = ring.template getZero< InputType2 >();
	if( chi == zeroIT2 || internal::getCoordinates( _a ).nonzeroes() == 0 ) {
		return foldl< descr >( _z, _y, ring.getAdditiveMonoid() );
	}
	if( internal::getCoordinates( _y ).nonzeroes() == 0 ) {
		return eWiseMulAdd< descr >( _z, _a, chi, ring.template getZero< typename Ring::D4 >(), ring );
	}

	// check for density
	const Vector< bool, reference, Coords > * const null_mask = NULL;
	auto null_coors = &( internal::getCoordinates( _a ) );
	null_coors = NULL;
	constexpr bool maybe_sparse = ! ( descr & descriptors::dense );
	if( maybe_sparse ) {
		// check whether all inputs are actually dense
		if( internal::getCoordinates( _a ).nonzeroes() == n && internal::getCoordinates( _y ).nonzeroes() == n ) {
			// yes, dispatch to version with dense descriptor set
			return internal::eWiseMulAdd_dispatch< descr | descriptors::dense, false, false, true, false >(
				_z, null_mask, internal::getRaw( _a ), &( internal::getCoordinates( _a ) ), &chi, null_coors, &_y, internal::getRaw( _y ), &( internal::getCoordinates( _y ) ), n, ring );
		}
	}

	// sparse or dense case
	return internal::eWiseMulAdd_dispatch< descr, false, false, true, false >(
		_z, null_mask, internal::getRaw( _a ), &( internal::getCoordinates( _a ) ), &chi, null_coors, &_y, internal::getRaw( _y ), &( internal::getCoordinates( _y ) ), n, ring );
}

/**
 * Calculates the axpy, \f$ z = a * x .+ y \f$, under this semiring.
 *
 * Specialisation for when \a y is a scalar.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename InputType1, typename InputType2, typename InputType3, typename OutputType, typename Coords >
RC eWiseMulAdd( Vector< OutputType, reference, Coords > & _z,
	const Vector< InputType1, reference, Coords > & _a,
	const Vector< InputType2, reference, Coords > & _x,
	const InputType3 gamma,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && ! grb::is_object< InputType3 >::value &&
			grb::is_semiring< Ring >::value,
		void >::type * const = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseMulAdd",
		"called with a left-hand scalar alpha of an element type that does not "
		"match the first domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseMulAdd",
		"called with a right-hand vector _x with an element type that does not "
		"match the second domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, InputType3 >::value ), "grb::eWiseMulAdd",
		"called with an additive vector _y with an element type that does not "
		"match the fourth domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, OutputType >::value ), "grb::eWiseMulAdd",
		"called with a result vector _z with an element type that does not match "
		"the fourth domain of the given semiring" );

	// dynamic sanity checks
	const size_t n = internal::getCoordinates( _z ).size();
	if( internal::getCoordinates( _a ).size() != n || internal::getCoordinates( _x ).size() != n ) {
		return MISMATCH;
	}

	// catch trivial cases
	const InputType3 zeroIT3 = ring.template getZero< InputType3 >();
	if( internal::getCoordinates( _a ).nonzeroes() == 0 || internal::getCoordinates( _x ).nonzeroes() == 0 ) {
		return foldl< descr >( _z, gamma, ring.getAdditiveMonoid() );
	}
	if( gamma == zeroIT3 ) {
		return eWiseMulAdd< descr >( _z, _a, _x, ring.template getZero< typename Ring::D4 >(), ring );
	}

	// check for density
	const Vector< bool, reference, Coords > * const null_mask = NULL;
	const Vector< InputType3, reference, Coords > * const null_y = NULL;
	auto null_coors = &( internal::getCoordinates( _a ) );
	null_coors = NULL;
	constexpr bool maybe_sparse = ! ( descr & descriptors::dense );
	if( maybe_sparse ) {
		// check whether all inputs are actually dense
		if( internal::getCoordinates( _a ).nonzeroes() == n && internal::getCoordinates( _x ).nonzeroes() == n ) {
			// yes, dispatch to version with dense descriptor set
			return internal::eWiseMulAdd_dispatch< descr | descriptors::dense, false, false, false, true >(
				_z, null_mask, internal::getRaw( _a ), &( internal::getCoordinates( _a ) ), internal::getRaw( _x ), &( internal::getCoordinates( _x ) ), null_y, &gamma, null_coors, n, ring );
		}
	}

	// sparse or dense case
	return internal::eWiseMulAdd_dispatch< descr, false, false, false, true >(
		_z, null_mask, internal::getRaw( _a ), &( internal::getCoordinates( _a ) ), internal::getRaw( _x ), &( internal::getCoordinates( _x ) ), null_y, &gamma, null_coors, n, ring );
}

/**
 * Calculates the axpy, \f$ z = a * x .+ y \f$, under this semiring.
 *
 * Specialisation for when \a x and \a y are scalar.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename InputType1, typename InputType2, typename InputType3, typename OutputType, typename Coords >
RC eWiseMulAdd( Vector< OutputType, reference, Coords > & _z,
	const Vector< InputType1, reference, Coords > & _a,
	const InputType2 beta,
	const InputType3 gamma,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && ! grb::is_object< InputType3 >::value &&
			grb::is_semiring< Ring >::value,
		void >::type * const = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseMulAdd",
		"called with a left-hand scalar alpha of an element type that does not "
		"match the first domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseMulAdd",
		"called with a right-hand vector _x with an element type that does not "
		"match the second domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, InputType3 >::value ), "grb::eWiseMulAdd",
		"called with an additive vector _y with an element type that does not "
		"match the fourth domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, OutputType >::value ), "grb::eWiseMulAdd",
		"called with a result vector _z with an element type that does not match "
		"the fourth domain of the given semiring" );

	// dynamic sanity checks
	const size_t n = internal::getCoordinates( _z ).size();
	if( internal::getCoordinates( _a ).size() != n ) {
		return MISMATCH;
	}

	// catch trivial cases
	const InputType2 zeroIT2 = ring.template getZero< InputType2 >();
	const InputType3 zeroIT3 = ring.template getZero< InputType3 >();
	if( internal::getCoordinates( _a ).nonzeroes() == 0 || zeroIT2 ) {
		return foldl< descr >( _z, gamma, ring.getAdditiveMonoid() );
	}
	if( gamma == zeroIT3 ) {
		return eWiseMulAdd< descr >( _z, _a, beta, ring.template getZero< typename Ring::D4 >(), ring );
	}

	// check for density
	Vector< bool, reference, Coords > * const null_mask = NULL;
	Vector< InputType3, reference, Coords > * const null_y = NULL;
	auto null_coors = &( internal::getCoordinates( _a ) );
	null_coors = NULL;
	constexpr bool maybe_sparse = ! ( descr & descriptors::dense );
	if( maybe_sparse ) {
		// check whether all inputs are actually dense
		if( internal::getCoordinates( _a ).nonzeroes() == n ) {
			// yes, dispatch to version with dense descriptor set
			return internal::eWiseMulAdd_dispatch< descr | descriptors::dense, false, false, true, true >(
				_z, null_mask, internal::getRaw( _a ), &( internal::getCoordinates( _a ) ), &beta, null_coors, null_y, &gamma, null_coors, n, ring );
		}
	}

	// sparse or dense case
	return internal::eWiseMulAdd_dispatch< descr, false, false, true, true >(
		_z, null_mask, internal::getRaw( _a ), &( internal::getCoordinates( _a ) ), &beta, null_coors, null_y, &gamma, null_coors, n, ring );
}

/**
 * Calculates the axpy, \f$ z = a * x .+ y \f$, under this semiring.
 *
 * Specialisation for when \a a and \a y are scalar.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename InputType1, typename InputType2, typename InputType3, typename OutputType, typename Coords >
RC eWiseMulAdd( Vector< OutputType, reference, Coords > & _z,
	const InputType1 alpha,
	const Vector< InputType2, reference, Coords > & _x,
	const InputType3 gamma,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && ! grb::is_object< InputType3 >::value &&
			grb::is_semiring< Ring >::value,
		void >::type * const = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseMulAdd",
		"called with a left-hand scalar alpha of an element type that does not "
		"match the first domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseMulAdd",
		"called with a right-hand vector _x with an element type that does not "
		"match the second domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, InputType3 >::value ), "grb::eWiseMulAdd",
		"called with an additive vector _y with an element type that does not "
		"match the fourth domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, OutputType >::value ), "grb::eWiseMulAdd",
		"called with a result vector _z with an element type that does not match "
		"the fourth domain of the given semiring" );

	// dynamic sanity checks
	const size_t n = internal::getCoordinates( _z ).size();
	if( internal::getCoordinates( _x ).size() != n ) {
		return MISMATCH;
	}

	// catch trivial cases
	const InputType1 zeroIT1 = ring.template getZero< InputType1 >();
	if( internal::getCoordinates( _x ).nonzeroes() == 0 || alpha == zeroIT1 ) {
		return foldl< descr >( _z, gamma, ring.getAdditiveMonoid() );
	}

	// check for density
	const Vector< bool, reference, Coords > * null_mask = NULL;
	const Vector< InputType3, reference, Coords > * null_y = NULL;
	auto null_coors = &( internal::getCoordinates( _x ) );
	null_coors = NULL;
	constexpr bool maybe_sparse = ! ( descr & descriptors::dense );
	if( maybe_sparse ) {
		// check whether all inputs are actually dense
		if( internal::getCoordinates( _x ).nonzeroes() == n ) {
			// yes, dispatch to version with dense descriptor set
			return internal::eWiseMulAdd_dispatch< descr | descriptors::dense, false, true, false, true >(
				_z, null_mask, &alpha, null_coors, internal::getRaw( _x ), &( internal::getCoordinates( _x ) ), null_y, &gamma, null_coors, n, ring );
		}
	}

	// sparse or dense case
	return internal::eWiseMulAdd_dispatch< descr, false, true, false, true >(
		_z, null_mask, &alpha, null_coors, internal::getRaw( _x ), &( internal::getCoordinates( _x ) ), null_y, &gamma, null_coors, n, ring );
}

/**
 * Calculates the axpy, \f$ z = a * x .+ y \f$, under this semiring.
 *
 * Specialisation for when \a a and \a x are scalar.
 *
 * \internal Dispatches to eWiseAdd.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename OutputType, typename InputType1, typename InputType2, typename InputType3, typename Coords >
RC eWiseMulAdd( Vector< OutputType, reference, Coords > & z,
	const InputType1 alpha,
	const InputType2 beta,
	const Vector< InputType3, reference, Coords > & y,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && ! grb::is_object< InputType3 >::value &&
			grb::is_semiring< Ring >::value,
		void >::type * const = NULL ) {
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseMulAdd(vector,scalar,scalar,scalar)",
		"First domain of semiring does not match first input type" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseMulAdd(vector,scalar,scalar,scalar)",
		"Second domain of semiring does not match second input type" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, InputType3 >::value ), "grb::eWiseMulAdd(vector,scalar,scalar,scalar)",
		"Fourth domain of semiring does not match third input type" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, OutputType >::value ), "grb::eWiseMulAdd(vector,scalar,scalar,scalar)",
		"Fourth domain of semiring does not match output type" );
#ifdef _DEBUG
	std::cout << "eWiseMulAdd (reference, vector <- scalar x scalar + vector) precomputes scalar multiply and dispatches to eWiseAdd (reference, vector <- scalar + vector)\n";
#endif
	typename Ring::D3 mul_result;
	RC rc = grb::apply( mul_result, alpha, beta, ring.getMultiplicativeOperator() );
#ifdef NDEBUG
	(void)rc;
#else
					assert( rc == SUCCESS );
#endif
	return grb::eWiseAdd( z, mul_result, y, ring );
}

/**
 * Calculates the axpy, \f$ z = a * x .+ y \f$, under this semiring.
 *
 * Specialisation for when \a a, \a x, and \a y are scalar.
 *
 * \internal Dispatches to set.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename OutputType, typename InputType1, typename InputType2, typename InputType3, typename Coords >
RC eWiseMulAdd( Vector< OutputType, reference, Coords > & z,
	const InputType1 alpha,
	const InputType2 beta,
	const InputType3 gamma,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && ! grb::is_object< InputType3 >::value &&
			grb::is_semiring< Ring >::value,
		void >::type * const = NULL ) {
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseMulAdd(vector,scalar,scalar,scalar)",
		"First domain of semiring does not match first input type" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseMulAdd(vector,scalar,scalar,scalar)",
		"Second domain of semiring does not match second input type" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, InputType3 >::value ), "grb::eWiseMulAdd(vector,scalar,scalar,scalar)",
		"Fourth domain of semiring does not match third input type" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, OutputType >::value ), "grb::eWiseMulAdd(vector,scalar,scalar,scalar)",
		"Fourth domain of semiring does not match output type" );
#ifdef _DEBUG
	std::cout << "eWiseMulAdd (reference, vector <- scalar x scalar + scalar) precomputes scalar operations and dispatches to set (reference)\n";
#endif
	typename Ring::D3 mul_result;
	RC rc = grb::apply( mul_result, alpha, beta, ring.getMultiplicativeOperator() );
#ifdef NDEBUG
	(void)rc;
#endif
	assert( rc == SUCCESS );
	typename Ring::D4 add_result;
	rc = grb::apply( add_result, mul_result, gamma, ring.getAdditiveOperator() );
#ifdef NDEBUG
	(void)rc;
#endif
	assert( rc == SUCCESS );
	return grb::set( z, add_result );
}

/**
 * Calculates the elementwise multiply-add, \f$ z = a .* x .+ y \f$, under
 * this semiring.
 *
 * Any combination of \a a, \a x, and \a y may be a scalar. Any scalars equal
 * to the given semiring's zero will be detected and automatically be
 * transformed into calls to eWiseMul, eWiseAdd, and so on.
 *
 * @tparam descr      The descriptor to be used (descriptors::no_operation
 *                    if left unspecified).
 * @tparam Ring       The semiring type to perform the element-wise
 *                    multiply-add on.
 * @tparam InputType1 The left-hand side input type to the multiplicative
 *                    operator of the \a ring.
 * @tparam InputType2 The right-hand side input type to the multiplicative
 *                    operator of the \a ring.
 * @tparam InputType3 The output type to the multiplicative operator of the
 *                    \a ring \em and the left-hand side input type to the
 *                    additive operator of the \a ring.
 * @tparam OutputType The right-hand side input type to the additive operator
 *                    of the \a ring \em and the result type of the same
 *                    operator.
 *
 * @param[out] _z  The pre-allocated output vector.
 * @param[in]  _a  The elements for left-hand side multiplication.
 * @param[in]  _x  The elements for right-hand side multiplication.
 * @param[in]  _y  The elements for right-hand size addition.
 * @param[in] ring The ring to perform the eWiseMulAdd under.
 *
 * @return grb::MISMATCH Whenever the dimensions of \a _a, \a _x, \a _y, and
 *                       \a z do not match. In this case, all input data
 *                       containers are left untouched and it will simply be
 *                       as though this call was never made.
 * @return grb::SUCCESS  On successful completion of this call.
 *
 * \warning An implementation is not obligated to detect overlap whenever
 *          it occurs. If part of \a z overlaps with \a x, \a y, or \a a,
 *          undefined behaviour will occur \em unless this function returns
 *          grb::OVERLAP. In other words: an implementation which returns
 *          erroneous results when vectors overlap and still returns
 *          grb::SUCCESS thus is also a valid GraphBLAS implementation!
 *
 * \parblock
 * \par Valid descriptors
 * grb::descriptors::no_operation, grb::descriptors::no_casting.
 *
 * \note Invalid descriptors will be ignored.
 *
 * If grb::descriptors::no_casting is specified, then 1) the first domain of
 * \a ring must match \a InputType1, 2) the second domain of \a ring must match
 * \a InputType2, 3) the third domain of \a ring must match \a InputType3,
 * 4) the fourth domain of \a ring must match \a OutputType. If one of these is
 * not true, the code shall not compile.
 * \endparblock
 *
 * \parblock
 * \par Performance guarantees
 *      -# This call takes \f$ \Theta(n) \f$ work, where \f$ n \f$ equals the
 *         size of the vectors \a _a, \a _x, \a _y, and \a _z. The constant
 *         factor depends on the cost of evaluating the addition and
 *         multiplication operators. A good implementation uses vectorised
 *         instructions whenever the input domains, the output domain, and
 *         the operators used allow for this.
 *
 *      -# This call takes \f$ \mathcal{O}(1) \f$ memory beyond the memory
 *         already used by the application when this function is called.
 *
 *      -# This call incurs at most \f$ n( \mathit{sizeof}(
 *           \mathit{InputType1} + \mathit{bool}
 *           \mathit{InputType2} + \mathit{bool}
 *           \mathit{InputType3} + \mathit{bool}
 *           \mathit{OutputType} + \mathit{bool}
 *         ) + \mathcal{O}(1) \f$
 *         bytes of data movement. A good implementation will stream \a _a,
 *         \a _x or \a _y into \a _z to apply the additive and multiplicative
 *         operators in-place, whenever the input domains, the output domain,
 *         and the operators used allow for this.
 * \endparblock
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename InputType1, typename InputType2, typename InputType3, typename OutputType, typename Coords >
RC eWiseMulAdd( Vector< OutputType, reference, Coords > & _z,
	const Vector< InputType1, reference, Coords > & _a,
	const Vector< InputType2, reference, Coords > & _x,
	const Vector< InputType3, reference, Coords > & _y,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && ! grb::is_object< InputType3 >::value &&
			grb::is_semiring< Ring >::value,
		void >::type * const = NULL ) {
	(void)ring;
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseMulAdd",
		"called with a left-hand vector _a with an element type that does not "
		"match the first domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseMulAdd",
		"called with a right-hand vector _x with an element type that does not "
		"match the second domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, InputType3 >::value ), "grb::eWiseMulAdd",
		"called with an additive vector _y with an element type that does not "
		"match the fourth domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, OutputType >::value ), "grb::eWiseMulAdd",
		"called with a result vector _z with an element type that does not match "
		"the fourth domain of the given semiring" );

	// catch trivial cases
	if( internal::getCoordinates( _a ).nonzeroes() == 0 ) {
		return foldr< descr >( _y, _z, ring.getAdditiveMonoid() );
	}
	if( internal::getCoordinates( _x ).nonzeroes() == 0 ) {
		return foldr< descr >( _y, _z, ring.getAdditiveMonoid() );
	}
	if( internal::getCoordinates( _y ).nonzeroes() == 0 ) {
		return eWiseMulAdd< descr >( _z, _a, _x, ring.template getZero< typename Ring::D4 >(), ring );
	}

	// dynamic sanity checks
	const size_t n = internal::getCoordinates( _z ).size();
	if( internal::getCoordinates( _x ).size() != n || internal::getCoordinates( _y ).size() != n || internal::getCoordinates( _a ).size() != n ) {
		return MISMATCH;
	}

	const Vector< bool, reference, Coords > * const null_mask = NULL;
	constexpr bool maybe_sparse = ! ( descr & descriptors::dense );
	if( maybe_sparse ) {
		// check for dense variant
		if( internal::getCoordinates( _x ).nonzeroes() == n && internal::getCoordinates( _y ).nonzeroes() == n && internal::getCoordinates( _a ).nonzeroes() == n ) {
			// yes, dispatch to version with dense descriptor set
			return internal::eWiseMulAdd_dispatch< descr | descriptors::dense, false, false, false, false >( _z, null_mask, internal::getRaw( _a ), &( internal::getCoordinates( _a ) ),
				internal::getRaw( _x ), &( internal::getCoordinates( _x ) ), &_y, internal::getRaw( _y ), &( internal::getCoordinates( _y ) ), n, ring );
		}
	}
	return internal::eWiseMulAdd_dispatch< descr, false, false, false, false >( _z, null_mask, internal::getRaw( _a ), &( internal::getCoordinates( _a ) ), internal::getRaw( _x ),
		&( internal::getCoordinates( _x ) ), &_y, internal::getRaw( _y ), &( internal::getCoordinates( _y ) ), n, ring );
}

/**
 * Calculates the element-wise multiplication of two vectors,
 *     \f$ z = z + x .* y \f$,
 * under a given semiring.
 *
 * @tparam descr      The descriptor to be used (descriptors::no_operation
 *                    if left unspecified).
 * @tparam Ring       The semiring type to perform the element-wise multiply
 *                    on.
 * @tparam InputType1 The left-hand side input type to the multiplicative
 *                    operator of the \a ring.
 * @tparam InputType2 The right-hand side input type to the multiplicative
 *                    operator of the \a ring.
 * @tparam OutputType The the result type of the multiplicative operator of
 *                    the \a ring.
 *
 * @param[out]  z  The output vector of type \a OutputType.
 * @param[in]   x  The left-hand input vector of type \a InputType1.
 * @param[in]   y  The right-hand input vector of type \a InputType2.
 * @param[in] ring The generalized semiring under which to perform this
 *                 element-wise multiplication.
 *
 * @return grb::MISMATCH Whenever the dimensions of \a x, \a y, and \a z do
 *                       not match. All input data containers are left
 *                       untouched if this exit code is returned; it will be
 *                       as though this call was never made.
 * @return grb::SUCCESS  On successful completion of this call.
 *
 * \parblock
 * \par Valid descriptors
 * grb::descriptors::no_operation, grb::descriptors::no_casting.
 *
 * \note Invalid descriptors will be ignored.
 *
 * If grb::descriptors::no_casting is specified, then 1) the first domain of
 * \a ring must match \a InputType1, 2) the second domain of \a ring must match
 * \a InputType2, 3) the third domain of \a ring must match \a OutputType. If
 * one of these is not true, the code shall not compile.
 *
 * \endparblock
 *
 * \parblock
 * \par Performance guarantees
 *      -# This call takes \f$ \Theta(n) \f$ work, where \f$ n \f$ equals the
 *         size of the vectors \a x, \a y, and \a z. The constant factor
 *         depends on the cost of evaluating the multiplication operator. A
 *         good implementation uses vectorised instructions whenever the input
 *         domains, the output domain, and the multiplicative operator used
 *         allow for this.
 *
 *      -# This call will not result in additional dynamic memory allocations.
 *
 *      -# This call takes \f$ \mathcal{O}(1) \f$ memory beyond the memory
 *         used by the application at the point of a call to this function.
 *
 *      -# This call incurs at most \f$ n( \mathit{sizeof}(\mathit{D1}) +
 *         \mathit{sizeof}(\mathit{D2}) + \mathit{sizeof}(\mathit{D3})) +
 *         \mathcal{O}(1) \f$ bytes of data movement. A good implementation
 *         will stream \a x or \a y into \a z to apply the multiplication
 *         operator in-place, whenever the input domains, the output domain,
 *         and the operator used allow for this.
 * \endparblock
 *
 * \warning When given sparse vectors, the zero now annihilates instead of
 *       acting as an identity. Thus the eWiseMul cannot simply map to an
 *       eWiseApply of the multiplicative operator.
 *
 * @see This is a specialised form of eWiseMulAdd.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename InputType1, typename InputType2, typename OutputType, typename Coords >
RC eWiseMul( Vector< OutputType, reference, Coords > & z,
	const Vector< InputType1, reference, Coords > & x,
	const Vector< InputType2, reference, Coords > & y,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && grb::is_semiring< Ring >::value,
		void >::type * const = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseMul",
		"called with a left-hand side input vector with element type that does not "
		"match the first domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseMul",
		"called with a right-hand side input vector with element type that does "
		"not match the second domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D3, OutputType >::value ), "grb::eWiseMul",
		"called with an output vector with element type that does not match the "
		"third domain of the given semiring" );
#ifdef _DEBUG
	std::cout << "eWiseMul (reference, vector <- vector x vector) dispatches to eWiseMulAdd (vector <- vector x vector + 0)\n";
#endif
	return eWiseMulAdd< descr >( z, x, y, ring.template getZero< Ring::D4 >(), ring );
}

/**
 * Computes \f$ z = z + x * y \f$.
 *
 * Specialisation for scalar \a x.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename InputType1, typename InputType2, typename OutputType, typename Coords >
RC eWiseMul( Vector< OutputType, reference, Coords > & z,
	const InputType1 alpha,
	const Vector< InputType2, reference, Coords > & y,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && grb::is_semiring< Ring >::value,
		void >::type * const = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseMul",
		"called with a left-hand side input vector with element type that does not "
		"match the first domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseMul",
		"called with a right-hand side input vector with element type that does "
		"not match the second domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D3, OutputType >::value ), "grb::eWiseMul",
		"called with an output vector with element type that does not match the "
		"third domain of the given semiring" );
#ifdef _DEBUG
	std::cout << "eWiseMul (reference, vector <- scalar x vector) dispatches to eWiseMulAdd (vector <- scalar x vector + 0)\n";
#endif
	return eWiseMulAdd< descr >( z, alpha, y, ring.template getZero< typename Ring::D4 >(), ring );
}

/**
 * Computes \f$ z = z + x * y \f$.
 *
 * Specialisation for scalar \a y.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename InputType1, typename InputType2, typename OutputType, typename Coords >
RC eWiseMul( Vector< OutputType, reference, Coords > & z,
	const Vector< InputType1, reference, Coords > & x,
	const InputType2 beta,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && grb::is_semiring< Ring >::value,
		void >::type * const = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseMul",
		"called with a left-hand side input vector with element type that does not "
		"match the first domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseMul",
		"called with a right-hand side input vector with element type that does "
		"not match the second domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D3, OutputType >::value ), "grb::eWiseMul",
		"called with an output vector with element type that does not match the "
		"third domain of the given semiring" );
#ifdef _DEBUG
	std::cout << "eWiseMul (reference) dispatches to eWiseMulAdd with 0.0 as additive scalar\n";
#endif
	return eWiseMulAdd< descr >( z, x, beta, ring.template getZero< typename Ring::D4 >(), ring.getMultiplicativeOperator() );
}

/**
 * Calculates the axpy, \f$ z = a * x .+ y \f$, under this semiring.
 *
 * Specialisation for when \a a is a scalar, masked version.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename InputType1, typename InputType2, typename InputType3, typename OutputType, typename MaskType, typename Coords >
RC eWiseMulAdd( Vector< OutputType, reference, Coords > & _z,
	const Vector< MaskType, reference, Coords > & _m,
	const InputType1 alpha,
	const Vector< InputType2, reference, Coords > & _x,
	const Vector< InputType3, reference, Coords > & _y,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && ! grb::is_object< InputType3 >::value &&
			grb::is_semiring< Ring >::value && ! grb::is_object< MaskType >::value,
		void >::type * const = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseMulAdd",
		"called with a left-hand scalar alpha of an element type that does not "
		"match the first domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseMulAdd",
		"called with a right-hand vector _x with an element type that does not "
		"match the second domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, InputType3 >::value ), "grb::eWiseMulAdd",
		"called with an additive vector _y with an element type that does not "
		"match the fourth domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, OutputType >::value ), "grb::eWiseMulAdd",
		"called with a result vector _z with an element type that does not match "
		"the fourth domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< bool, MaskType >::value ), "grb::eWiseMulAdd", "called with a mask vector _m with a non-bool element type" );

	// catch empty mask
	const size_t m = internal::getCoordinates( _m ).size();
	if( m == 0 ) {
		return eWiseMulAdd< descr >( _z, alpha, _x, _y, ring );
	}

	// dynamic sanity checks
	const size_t n = internal::getCoordinates( _z ).size();
	if( internal::getCoordinates( _x ).size() != n || internal::getCoordinates( _y ).size() != n || m != n ) {
		return MISMATCH;
	}

	// catch trivial cases
	const InputType1 zeroIT1 = ring.template getZero< InputType1 >();
	if( alpha == zeroIT1 || internal::getCoordinates( _x ).nonzeroes() == 0 ) {
		return foldl< descr >( _z, _m, _y, ring.getAdditiveMonoid() );
	}
	if( internal::getCoordinates( _y ).nonzeroes() == 0 ) {
		return eWiseMulAdd< descr >( _z, _m, alpha, _x, ring.template getZero< typename Ring::D4 >(), ring );
	}

	// check for density
	const Vector< bool, reference, Coords > * const null_mask = NULL;
	auto null_coors = &( internal::getCoordinates( _x ) );
	null_coors = NULL;
	constexpr bool maybe_sparse = ! ( descr & descriptors::dense );
	if( maybe_sparse ) {
		// check whether all inputs are actually dense
		if( internal::getCoordinates( _x ).nonzeroes() == n && internal::getCoordinates( _y ).nonzeroes() == n &&
			( internal::getCoordinates( _m ).nonzeroes() == n && ( descr & descriptors::structural ) && ! ( descr & descriptors::invert_mask ) ) ) {
			// yes, dispatch to version with dense descriptor set
			return internal::eWiseMulAdd_dispatch< descr | descriptors::dense, false, true, false, false >(
				_z, null_mask, &alpha, null_coors, internal::getRaw( _x ), &( internal::getCoordinates( _x ) ), &_y, internal::getRaw( _y ), &( internal::getCoordinates( _y ) ), n, ring );
		}
	}

	// sparse case
	return internal::eWiseMulAdd_dispatch< descr, true, true, false, false >(
		_z, &_m, &alpha, null_coors, internal::getRaw( _x ), &( internal::getCoordinates( _x ) ), &_y, internal::getRaw( _y ), &( internal::getCoordinates( _y ) ), n, ring );
}

/**
 * Calculates the elementwise multiply-add, \f$ z = a .* x .+ y \f$, under
 * this semiring.
 *
 * Specialisation for when \a x is a scalar, masked version.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename InputType1, typename InputType2, typename InputType3, typename OutputType, typename MaskType, typename Coords >
RC eWiseMulAdd( Vector< OutputType, reference, Coords > & _z,
	const Vector< MaskType, reference, Coords > & _m,
	const Vector< InputType1, reference, Coords > & _a,
	const InputType2 chi,
	const Vector< InputType3, reference, Coords > & _y,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && ! grb::is_object< InputType3 >::value &&
			grb::is_semiring< Ring >::value && ! grb::is_object< MaskType >::value,
		void >::type * const = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseMulAdd",
		"called with a left-hand scalar alpha of an element type that does not "
		"match the first domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseMulAdd",
		"called with a right-hand vector _x with an element type that does not "
		"match the second domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, InputType3 >::value ), "grb::eWiseMulAdd",
		"called with an additive vector _y with an element type that does not "
		"match the fourth domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, OutputType >::value ), "grb::eWiseMulAdd",
		"called with a result vector _z with an element type that does not match "
		"the fourth domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< bool, MaskType >::value ), "grb::eWiseMulAdd", "called with a mask vector _m with a non-bool element type" );

	// catch empty mask
	const size_t m = internal::getCoordinates( _m ).size();
	if( m == 0 ) {
		return eWiseMulAdd< descr >( _z, _a, chi, _y, ring );
	}

	// dynamic sanity checks
	const size_t n = internal::getCoordinates( _z ).size();
	if( internal::getCoordinates( _a ).size() != n || internal::getCoordinates( _y ).size() != n || m != n ) {
		return MISMATCH;
	}

	// catch trivial cases
	const InputType1 zeroIT2 = ring.template getZero< InputType2 >();
	if( chi == zeroIT2 || internal::getCoordinates( _a ).nonzeroes() == 0 ) {
		return foldl< descr >( _z, _m, _y, ring.getAdditiveMonoid() );
	}
	if( internal::getCoordinates( _y ).nonzeroes() == 0 ) {
		return eWiseMulAdd< descr >( _z, _m, _a, chi, ring.template getZero< typename Ring::D4 >(), ring );
	}

	// check for density
	const Vector< bool, reference, Coords > * const null_mask = NULL;
	auto null_coors = &( internal::getCoordinates( _a ) );
	null_coors = NULL;
	constexpr bool maybe_sparse = ! ( descr & descriptors::dense );
	if( maybe_sparse ) {
		// check whether all inputs are actually dense
		if( internal::getCoordinates( _a ).nonzeroes() == n && internal::getCoordinates( _y ).nonzeroes() == n &&
			( internal::getCoordinates( _m ).nonzeroes() == n && ( descr & descriptors::structural ) && ! ( descr & descriptors::invert_mask ) ) ) {
			// yes, dispatch to version with dense descriptor set
			return internal::eWiseMulAdd_dispatch< descr | descriptors::dense, false, false, true, false >(
				_z, null_mask, internal::getRaw( _a ), &( internal::getCoordinates( _a ) ), &chi, null_coors, &_y, internal::getRaw( _y ), &( internal::getCoordinates( _y ) ), n, ring );
		}
	}

	// sparse case
	return internal::eWiseMulAdd_dispatch< descr, true, false, true, false >(
		_z, &_m, internal::getRaw( _a ), &( internal::getCoordinates( _a ) ), &chi, null_coors, &_y, internal::getRaw( _y ), &( internal::getCoordinates( _y ) ), n, ring );
}

/**
 * Calculates the axpy, \f$ z = a * x .+ y \f$, under this semiring.
 *
 * Specialisation for when \a y is a scalar, masked version.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename InputType1, typename InputType2, typename InputType3, typename OutputType, typename MaskType, typename Coords >
RC eWiseMulAdd( Vector< OutputType, reference, Coords > & _z,
	const Vector< MaskType, reference, Coords > & _m,
	const Vector< InputType1, reference, Coords > & _a,
	const Vector< InputType2, reference, Coords > & _x,
	const InputType3 gamma,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && ! grb::is_object< InputType3 >::value &&
			grb::is_semiring< Ring >::value && ! grb::is_object< MaskType >::value,
		void >::type * const = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseMulAdd",
		"called with a left-hand scalar alpha of an element type that does not "
		"match the first domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseMulAdd",
		"called with a right-hand vector _x with an element type that does not "
		"match the second domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, InputType3 >::value ), "grb::eWiseMulAdd",
		"called with an additive vector _y with an element type that does not "
		"match the fourth domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, OutputType >::value ), "grb::eWiseMulAdd",
		"called with a result vector _z with an element type that does not match "
		"the fourth domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< bool, MaskType >::value ), "grb::eWiseMulAdd", "called with a mask vector _m with a non-bool element type" );

	// catch empty mask
	const size_t m = internal::getCoordinates( _m ).size();
	if( m == 0 ) {
		return eWiseMulAdd< descr >( _z, _a, _x, gamma, ring );
	}

	// dynamic sanity checks
	const size_t n = internal::getCoordinates( _z ).size();
	if( internal::getCoordinates( _a ).size() != n || internal::getCoordinates( _x ).size() != n || m != n ) {
		return MISMATCH;
	}

	// catch trivial cases
	const InputType3 zeroIT3 = ring.template getZero< InputType3 >();
	if( internal::getCoordinates( _a ).nonzeroes() == 0 || internal::getCoordinates( _x ).nonzeroes() == 0 ) {
		return foldl< descr >( _z, _m, gamma, ring.getAdditiveMonoid() );
	}
	if( gamma == zeroIT3 ) {
		return eWiseMulAdd< descr >( _z, _m, _a, _x, ring.template getZero< typename Ring::D4 >(), ring );
	}

	// check for density
	const Vector< bool, reference, Coords > * const null_mask = NULL;
	const Vector< InputType3, reference, Coords > * const null_y = NULL;
	auto null_coors = &( internal::getCoordinates( _a ) );
	null_coors = NULL;
	constexpr bool maybe_sparse = ! ( descr & descriptors::dense );
	if( maybe_sparse ) {
		// check whether all inputs are actually dense
		if( internal::getCoordinates( _a ).nonzeroes() == n && internal::getCoordinates( _x ).nonzeroes() == n &&
			( internal::getCoordinates( _m ).nonzeroes() == n && ( descr & descriptors::structural ) && ! ( descr & descriptors::invert_mask ) ) ) {
			// yes, dispatch to version with dense descriptor set
			return internal::eWiseMulAdd_dispatch< descr | descriptors::dense, false, false, false, true >(
				_z, null_mask, internal::getRaw( _a ), &( internal::getCoordinates( _a ) ), internal::getRaw( _x ), &( internal::getCoordinates( _x ) ), null_y, &gamma, null_coors, n, ring );
		}
	}

	// sparse case
	return internal::eWiseMulAdd_dispatch< descr, true, false, false, true >(
		_z, &_m, internal::getRaw( _a ), &( internal::getCoordinates( _a ) ), internal::getRaw( _x ), &( internal::getCoordinates( _x ) ), null_y, &gamma, null_coors, n, ring );
}

/**
 * Calculates the axpy, \f$ z = a * x .+ y \f$, under this semiring.
 *
 * Specialisation for when \a x and \a y are scalar, masked version.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename InputType1, typename InputType2, typename InputType3, typename OutputType, typename MaskType, typename Coords >
RC eWiseMulAdd( Vector< OutputType, reference, Coords > & _z,
	const Vector< MaskType, reference, Coords > & _m,
	const Vector< InputType1, reference, Coords > & _a,
	const InputType2 beta,
	const InputType3 gamma,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && ! grb::is_object< InputType3 >::value &&
			grb::is_semiring< Ring >::value && ! grb::is_object< MaskType >::value,
		void >::type * const = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseMulAdd",
		"called with a left-hand scalar alpha of an element type that does not "
		"match the first domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseMulAdd",
		"called with a right-hand vector _x with an element type that does not "
		"match the second domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, InputType3 >::value ), "grb::eWiseMulAdd",
		"called with an additive vector _y with an element type that does not "
		"match the fourth domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, OutputType >::value ), "grb::eWiseMulAdd",
		"called with a result vector _z with an element type that does not match "
		"the fourth domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< bool, MaskType >::value ), "grb::eWiseMulAdd", "called with a mask vector _m with a non-bool element type" );

	// catch empty mask
	const size_t m = internal::getCoordinates( _m ).size();
	if( m == 0 ) {
		return eWiseMulAdd< descr >( _z, _a, beta, gamma, ring );
	}

	// dynamic sanity checks
	const size_t n = internal::getCoordinates( _z ).size();
	if( internal::getCoordinates( _a ).size() != n || m != n ) {
		return MISMATCH;
	}

	// catch trivial cases
	const InputType2 zeroIT2 = ring.template getZero< InputType2 >();
	const InputType3 zeroIT3 = ring.template getZero< InputType3 >();
	if( internal::getCoordinates( _a ).nonzeroes() == 0 || zeroIT2 ) {
		return foldl< descr >( _z, _m, gamma, ring.getAdditiveMonoid() );
	}
	if( gamma == zeroIT3 ) {
		return eWiseMulAdd< descr >( _z, _m, _a, beta, ring.template getZero< typename Ring::D4 >(), ring );
	}

	// check for density
	const Vector< bool, reference, Coords > * null_mask = NULL;
	const Vector< InputType3, reference, Coords > * null_y = NULL;
	auto null_coors = &( internal::getCoordinates( _a ) );
	null_coors = NULL;
	constexpr bool maybe_sparse = ! ( descr & descriptors::dense );
	if( maybe_sparse ) {
		// check whether all inputs are actually dense
		if( internal::getCoordinates( _a ).nonzeroes() == n && ( internal::getCoordinates( _m ).nonzeroes() == n && ( descr & descriptors::structural ) && ! ( descr & descriptors::invert_mask ) ) ) {
			// yes, dispatch to version with dense descriptor set
			return internal::eWiseMulAdd_dispatch< descr | descriptors::dense, false, false, true, true >(
				_z, null_mask, internal::getRaw( _a ), &( internal::getCoordinates( _a ) ), &beta, null_coors, null_y, &gamma, null_coors, n, ring );
		}
	}

	// sparse case
	return internal::eWiseMulAdd_dispatch< descr, true, false, true, true >(
		_z, &_m, internal::getRaw( _a ), &( internal::getCoordinates( _a ) ), &beta, null_coors, null_y, &gamma, null_coors, n, ring );
}

/**
 * Calculates the axpy, \f$ z = a * x .+ y \f$, under this semiring.
 *
 * Specialisation for when \a a and \a y are scalar, masked version.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename InputType1, typename InputType2, typename InputType3, typename OutputType, typename MaskType, typename Coords >
RC eWiseMulAdd( Vector< OutputType, reference, Coords > & _z,
	const Vector< MaskType, reference, Coords > & _m,
	const InputType1 alpha,
	const Vector< InputType2, reference, Coords > & _x,
	const InputType3 gamma,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && ! grb::is_object< InputType3 >::value &&
			grb::is_semiring< Ring >::value && ! grb::is_object< MaskType >::value,
		void >::type * const = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseMulAdd",
		"called with a left-hand scalar alpha of an element type that does not "
		"match the first domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseMulAdd",
		"called with a right-hand vector _x with an element type that does not "
		"match the second domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, InputType3 >::value ), "grb::eWiseMulAdd",
		"called with an additive vector _y with an element type that does not "
		"match the fourth domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, OutputType >::value ), "grb::eWiseMulAdd",
		"called with a result vector _z with an element type that does not match "
		"the fourth domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< bool, MaskType >::value ), "grb::eWiseMulAdd", "called with a mask vector _m with a non-bool element type" );

	// catch empty mask
	const size_t m = internal::getCoordinates( _m ).size();
	if( m == 0 ) {
		return eWiseMulAdd< descr >( _z, alpha, _x, gamma, ring );
	}

	// dynamic sanity checks
	const size_t n = internal::getCoordinates( _z ).size();
	if( internal::getCoordinates( _x ).size() != n || m != n ) {
		return MISMATCH;
	}

	// catch trivial cases
	const InputType1 zeroIT1 = ring.template getZero< InputType1 >();
	if( internal::getCoordinates( _x ).nonzeroes() == 0 || alpha == zeroIT1 ) {
		return foldl< descr >( _z, _m, gamma, ring.getAdditiveMonoid() );
	}

	// check for density
	const Vector< bool, reference, Coords > * const null_mask = NULL;
	const Vector< InputType3, reference, Coords > * const null_y = NULL;
	auto null_coors = &( internal::getCoordinates( _x ) );
	null_coors = NULL;
	constexpr bool maybe_sparse = ! ( descr & descriptors::dense );
	if( maybe_sparse ) {
		// check whether all inputs are actually dense
		if( internal::getCoordinates( _x ).nonzeroes() == n && ( internal::getCoordinates( _m ).nonzeroes() == n && ( descr & descriptors::structural ) && ! ( descr & descriptors::invert_mask ) ) ) {
			// yes, dispatch to version with dense descriptor set
			return internal::eWiseMulAdd_dispatch< descr | descriptors::dense, false, true, false, true >(
				_z, null_mask, &alpha, null_coors, internal::getRaw( _x ), &( internal::getCoordinates( _x ) ), null_y, &gamma, null_coors, n, ring );
		}
	}

	// sparse case
	return internal::eWiseMulAdd_dispatch< descr, true, true, false, true >(
		_z, &_m, &alpha, null_coors, internal::getRaw( _x ), &( internal::getCoordinates( _x ) ), null_y, &gamma, null_coors, n, ring );
}

/**
 * Calculates the axpy, \f$ z = a * x .+ y \f$, under this semiring.
 *
 * Masked version.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename InputType1, typename InputType2, typename InputType3, typename OutputType, typename MaskType, typename Coords >
RC eWiseMulAdd( Vector< OutputType, reference, Coords > & _z,
	const Vector< MaskType, reference, Coords > & _m,
	const Vector< InputType1, reference, Coords > & _a,
	const Vector< InputType2, reference, Coords > & _x,
	const Vector< InputType3, reference, Coords > & _y,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && ! grb::is_object< InputType3 >::value &&
			grb::is_semiring< Ring >::value && ! grb::is_object< MaskType >::value,
		void >::type * const = NULL ) {
	(void)ring;
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseMulAdd",
		"called with a left-hand vector _a with an element type that does not "
		"match the first domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseMulAdd",
		"called with a right-hand vector _x with an element type that does not "
		"match the second domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, InputType3 >::value ), "grb::eWiseMulAdd",
		"called with an additive vector _y with an element type that does not "
		"match the fourth domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, OutputType >::value ), "grb::eWiseMulAdd",
		"called with a result vector _z with an element type that does not match "
		"the fourth domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< bool, MaskType >::value ), "grb::eWiseMulAdd", "called with a mask vector _m with a non-bool element type" );

	// catch empty mask
	const size_t m = internal::getCoordinates( _m ).size();
	if( m == 0 ) {
		return eWiseMulAdd< descr >( _z, _a, _x, _y, ring );
	}

	// catch trivial cases
	if( internal::getCoordinates( _a ).nonzeroes() == 0 ) {
		return foldr< descr >( _y, _m, _z, ring.getAdditiveMonoid() );
	}
	if( internal::getCoordinates( _x ).nonzeroes() == 0 ) {
		return foldr< descr >( _y, _m, _z, ring.getAdditiveMonoid() );
	}
	if( internal::getCoordinates( _y ).nonzeroes() == 0 ) {
		return eWiseMulAdd< descr >( _z, _m, _a, _x, ring.template getZero< typename Ring::D4 >(), ring );
	}

	// dynamic sanity checks
	const size_t n = internal::getCoordinates( _z ).size();
	if( internal::getCoordinates( _x ).size() != n || internal::getCoordinates( _y ).size() != n || internal::getCoordinates( _a ).size() != n || m != n ) {
		return MISMATCH;
	}

	const Vector< bool, reference, Coords > * const null_mask = NULL;
	constexpr bool maybe_sparse = ! ( descr & descriptors::dense );
	if( maybe_sparse ) {
		// check for dense variant
		if( internal::getCoordinates( _x ).nonzeroes() == n && internal::getCoordinates( _y ).nonzeroes() == n && internal::getCoordinates( _a ).nonzeroes() == n &&
			( internal::getCoordinates( _m ).nonzeroes() == n && ( descr & descriptors::structural ) && ! ( descr & descriptors::invert_mask ) ) ) {
			// yes, dispatch to version with dense descriptor set
			return internal::eWiseMulAdd_dispatch< descr | descriptors::dense, false, false, false, false >( _z, null_mask, internal::getRaw( _a ), &( internal::getCoordinates( _a ) ),
				internal::getRaw( _x ), &( internal::getCoordinates( _x ) ), &_y, internal::getRaw( _y ), &( internal::getCoordinates( _y ) ), n, ring );
		}
	}
	return internal::eWiseMulAdd_dispatch< descr, true, false, false, false >( _z, &_m, internal::getRaw( _a ), &( internal::getCoordinates( _a ) ), internal::getRaw( _x ),
		&( internal::getCoordinates( _x ) ), &_y, internal::getRaw( _y ), &( internal::getCoordinates( _y ) ), n, ring );
}

/**
 * Calculates the element-wise multiplication of two vectors,
 *     \f$ z = z + x .* y \f$,
 * under a given semiring.
 *
 * Masked verison.
 *
 * \internal Dispatches to eWiseMulAdd with zero additive scalar.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename InputType1, typename InputType2, typename OutputType, typename MaskType, typename Coords >
RC eWiseMul( Vector< OutputType, reference, Coords > & z,
	const Vector< MaskType, reference, Coords > & m,
	const Vector< InputType1, reference, Coords > & x,
	const Vector< InputType2, reference, Coords > & y,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && ! grb::is_object< MaskType >::value &&
			grb::is_semiring< Ring >::value,
		void >::type * const = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseMul",
		"called with a left-hand side input vector with element type that does not "
		"match the first domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseMul",
		"called with a right-hand side input vector with element type that does "
		"not match the second domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D3, OutputType >::value ), "grb::eWiseMul",
		"called with an output vector with element type that does not match the "
		"third domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< bool, MaskType >::value ), "grb::eWiseMulAdd", "called with a mask vector with a non-bool element type" );
#ifdef _DEBUG
	std::cout << "eWiseMul (reference, vector <- vector x vector, masked) dispatches to eWiseMulAdd (vector <- vector x vector + 0, masked)\n";
#endif
	return eWiseMulAdd< descr >( z, m, x, y, ring.template getZero< Ring::D4 >(), ring );
}

/**
 * Computes \f$ z = z + x * y \f$.
 *
 * Specialisation for scalar \a x, masked version.
 *
 * \internal Dispatches to eWiseMulAdd with zero additive scalar.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename InputType1, typename InputType2, typename OutputType, typename MaskType, typename Coords >
RC eWiseMul( Vector< OutputType, reference, Coords > & z,
	const Vector< MaskType, reference, Coords > & m,
	const InputType1 alpha,
	const Vector< InputType2, reference, Coords > & y,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && ! grb::is_object< MaskType >::value &&
			grb::is_semiring< Ring >::value,
		void >::type * const = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseMul",
		"called with a left-hand side input vector with element type that does not "
		"match the first domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseMul",
		"called with a right-hand side input vector with element type that does "
		"not match the second domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D3, OutputType >::value ), "grb::eWiseMul",
		"called with an output vector with element type that does not match the "
		"third domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< bool, MaskType >::value ), "grb::eWiseMulAdd", "called with a mask vector _m with a non-bool element type" );
#ifdef _DEBUG
	std::cout << "eWiseMul (reference, vector <- scalar x vector, masked) dispatches to eWiseMulAdd (vector <- scalar x vector + 0, masked)\n";
#endif
	return eWiseMulAdd< descr >( z, m, alpha, y, ring.template getZero< typename Ring::D4 >(), ring );
}

/**
 * Computes \f$ z = z + x * y \f$.
 *
 * Specialisation for scalar \a y, masked version.
 *
 * \internal Dispatches to eWiseMulAdd with zero additive scalar.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename InputType1, typename InputType2, typename OutputType, typename MaskType, typename Coords >
RC eWiseMul( Vector< OutputType, reference, Coords > & z,
	const Vector< MaskType, reference, Coords > & m,
	const Vector< InputType1, reference, Coords > & x,
	const InputType2 beta,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && ! grb::is_object< MaskType >::value &&
			grb::is_semiring< Ring >::value,
		void >::type * const = NULL ) {
	// static sanity checks
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseMul",
		"called with a left-hand side input vector with element type that does not "
		"match the first domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseMul",
		"called with a right-hand side input vector with element type that does "
		"not match the second domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D3, OutputType >::value ), "grb::eWiseMul",
		"called with an output vector with element type that does not match the "
		"third domain of the given semiring" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< bool, MaskType >::value ), "grb::eWiseMulAdd", "called with a mask vector _m with a non-bool element type" );
#ifdef _DEBUG
	std::cout << "eWiseMul (reference, masked) dispatches to masked eWiseMulAdd with 0.0 as additive scalar\n";
#endif
	return eWiseMulAdd< descr >( z, m, x, beta, ring.template getZero< typename Ring::D4 >(), ring.getMultiplicativeOperator() );
}

/**
 * Computes \f$ z = z + a * x + y \f$.
 *
 * Specialisation for scalar \a a and \a x, masked version.
 *
 * \internal Dispatches to masked eWiseAdd.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename OutputType, typename MaskType, typename InputType1, typename InputType2, typename InputType3, typename Coords >
RC eWiseMulAdd( Vector< OutputType, reference, Coords > & z,
	const Vector< MaskType, reference, Coords > & m,
	const InputType1 alpha,
	const InputType2 beta,
	const Vector< InputType3, reference, Coords > & y,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && ! grb::is_object< InputType3 >::value &&
			grb::is_semiring< Ring >::value && ! grb::is_object< MaskType >::value,
		void >::type * const = NULL ) {
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseMulAdd(vector,scalar,scalar,scalar)",
		"First domain of semiring does not match first input type" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseMulAdd(vector,scalar,scalar,scalar)",
		"Second domain of semiring does not match second input type" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, InputType3 >::value ), "grb::eWiseMulAdd(vector,scalar,scalar,scalar)",
		"Fourth domain of semiring does not match third input type" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, OutputType >::value ), "grb::eWiseMulAdd(vector,scalar,scalar,scalar)",
		"Fourth domain of semiring does not match output type" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< bool, MaskType >::value ), "grb::eWiseMulAdd", "called with a mask vector with a non-bool element type" );
#ifdef _DEBUG
	std::cout << "eWiseMulAdd (reference, vector <- scalar x scalar + vector, masked) precomputes scalar multiply and dispatches to eWiseAdd (reference, vector <- scalar + vector, masked)\n";
#endif
	typename Ring::D3 mul_result;
	RC rc = grb::apply( mul_result, alpha, beta, ring.getMultiplicativeOperator() );
#ifdef NDEBUG
	(void)rc;
#else
					assert( rc == SUCCESS );
#endif
	return grb::eWiseAdd( z, m, mul_result, y, ring );
}

/**
 * Computes \f$ z = z + a * x + y \f$.
 *
 * Specialisation for scalar \a a, \a x, and \a y, masked version.
 *
 * \internal Dispatches to masked set.
 */
template< Descriptor descr = descriptors::no_operation, class Ring, typename OutputType, typename MaskType, typename InputType1, typename InputType2, typename InputType3, typename Coords >
RC eWiseMulAdd( Vector< OutputType, reference, Coords > & z,
	const Vector< MaskType, reference, Coords > & m,
	const InputType1 alpha,
	const InputType2 beta,
	const InputType3 gamma,
	const Ring & ring = Ring(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && ! grb::is_object< InputType3 >::value &&
			grb::is_semiring< Ring >::value,
		void >::type * const = NULL ) {
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D1, InputType1 >::value ), "grb::eWiseMulAdd(vector,scalar,scalar,scalar)",
		"First domain of semiring does not match first input type" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D2, InputType2 >::value ), "grb::eWiseMulAdd(vector,scalar,scalar,scalar)",
		"Second domain of semiring does not match second input type" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, InputType3 >::value ), "grb::eWiseMulAdd(vector,scalar,scalar,scalar)",
		"Fourth domain of semiring does not match third input type" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename Ring::D4, OutputType >::value ), "grb::eWiseMulAdd(vector,scalar,scalar,scalar)",
		"Fourth domain of semiring does not match output type" );
	NO_CAST_OP_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< bool, MaskType >::value ), "grb::eWiseMulAdd", "called with a mask vector with a non-bool element type" );
#ifdef _DEBUG
	std::cout << "eWiseMulAdd (reference, vector <- scalar x scalar + scalar, masked) precomputes scalar operations and dispatches to set (reference, masked)\n";
#endif
	typename Ring::D3 mul_result;
	RC rc = grb::apply( mul_result, alpha, beta, ring.getMultiplicativeOperator() );
#ifdef NDEBUG
	(void)rc;
#endif
	assert( rc == SUCCESS );
	typename Ring::D4 add_result;
	rc = grb::apply( add_result, mul_result, gamma, ring.getAdditiveOperator() );
#ifdef NDEBUG
	(void)rc;
#endif
	assert( rc == SUCCESS );
	return grb::set( z, m, add_result );
}

// internal namespace for implementation of grb::dot
namespace internal {

	/** @see grb::dot */
	template< Descriptor descr = descriptors::no_operation, class AddMonoid, class AnyOp, typename OutputType, typename InputType1, typename InputType2, typename Coords >
	RC dot_generic( OutputType & z,
		const Vector< InputType1, reference, Coords > & x,
		const Vector< InputType2, reference, Coords > & y,
		const AddMonoid & addMonoid = AddMonoid(),
		const AnyOp & anyOp = AnyOp() ) {
		const size_t n = internal::getCoordinates( x ).size();
		if( n != internal::getCoordinates( y ).size() ) {
			return MISMATCH;
		}

		// check if dense flag is set correctly
		constexpr bool dense = descr & descriptors::dense;
		const size_t nzx = internal::getCoordinates( x ).nonzeroes();
		const size_t nzy = internal::getCoordinates( y ).nonzeroes();
		if( dense ) {
			if( n != nzx || n != nzy ) {
				return PANIC;
			}
		} else {
			if( n == nzx && n == nzy ) {
				return PANIC;
			}
		}

		size_t loopsize = n;
		auto * coors_r_p = &( internal::getCoordinates( x ) );
		auto * coors_q_p = &( internal::getCoordinates( y ) );
		if( ! dense ) {
			if( nzx < nzy ) {
				loopsize = nzx;
			} else {
				loopsize = nzy;
				std::swap( coors_r_p, coors_q_p );
			}
		}
		auto & coors_r = *coors_r_p;
		auto & coors_q = *coors_q_p;

#ifdef _DEBUG
		std::cout << "\t In dot_generic with loopsize " << loopsize << "\n";
#endif
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
		z = addMonoid.template getIdentity< OutputType >();
		#pragma omp parallel
		{
			size_t start, end;
			config::OMP::localRange( start, end, 0, loopsize, AnyOp::blocksize );
#else
						const size_t start = 0;
						const size_t end = loopsize;
#endif
			if( end > start ) {
				// get raw alias
				const InputType1 * __restrict__ a = internal::getRaw( x );
				const InputType2 * __restrict__ b = internal::getRaw( y );

				// overwrite z with first multiplicant
				typename AddMonoid::D3 reduced;
				if( dense ) {
					apply( reduced, a[ end - 1 ], b[ end - 1 ], anyOp );
				} else {
					const size_t index = coors_r.index( end - 1 );
					if( coors_q.assigned( index ) ) {
						apply( reduced, a[ index ], b[ index ], anyOp );
					} else {
						reduced = addMonoid.template getIdentity< typename AddMonoid::D3 >();
					}
				}

				// enter vectorised loop
				size_t i = start;
				if( dense ) {
					while( i + AnyOp::blocksize < end - 1 ) {
						// declare buffers
						static_assert( AnyOp::blocksize > 0,
							"Configuration error: vectorisation blocksize set to "
							"0!" );
						typename AnyOp::D1 xx[ AnyOp::blocksize ];
						typename AnyOp::D2 yy[ AnyOp::blocksize ];
						typename AnyOp::D3 zz[ AnyOp::blocksize ];

						// prepare registers
						for( size_t k = 0; k < AnyOp::blocksize; ++k ) {
							xx[ k ] = static_cast< typename AnyOp::D1 >( a[ i ] );
							yy[ k ] = static_cast< typename AnyOp::D2 >( b[ i++ ] );
						}

						// perform element-wise multiplication
						for( size_t k = 0; k < AnyOp::blocksize; ++k ) {
							apply( zz[ k ], xx[ k ], yy[ k ], anyOp );
						}

						// perform reduction into output element
						addMonoid.getOperator().foldlArray( reduced, zz, AnyOp::blocksize );
						//^--> note that this foldl operates on raw arrays,
						//     and thus should not be mistaken with a foldl
						//     on a grb::Vector.
					}
				} else {
#ifdef _DEBUG
					std::cout << "\t\t in sparse variant, nonzero range " << start << "--" << end << ", blocksize " << AnyOp::blocksize << "\n";
#endif
					while( i + AnyOp::blocksize < end - 1 ) {
						// declare buffers
						static_assert( AnyOp::blocksize > 0,
							"Configuration error: vectorisation blocksize set to "
							"0!" );
						typename AnyOp::D1 xx[ AnyOp::blocksize ];
						typename AnyOp::D2 yy[ AnyOp::blocksize ];
						typename AnyOp::D3 zz[ AnyOp::blocksize ];
						bool mask[ AnyOp::blocksize ];

						// prepare registers
						for( size_t k = 0; k < AnyOp::blocksize; ++k, ++i ) {
							mask[ k ] = coors_q.assigned( coors_r.index( i ) );
						}

						// rewind
						i -= AnyOp::blocksize;

						// do masked load
						for( size_t k = 0; k < AnyOp::blocksize; ++k, ++i ) {
							if( mask[ k ] ) {
								xx[ k ] = static_cast< typename AnyOp::D1 >( a[ coors_r.index( i ) ] );
								yy[ k ] = static_cast< typename AnyOp::D2 >( b[ coors_r.index( i ) ] );
							}
						}

						// rewind
						i -= AnyOp::blocksize;

						// perform element-wise multiplication
						for( size_t k = 0; k < AnyOp::blocksize; ++k, ++i ) {
							if( mask[ k ] ) {
								apply( zz[ k ], xx[ k ], yy[ k ], anyOp );
							} else {
								zz[ k ] = addMonoid.template getIdentity< typename AnyOp::D3 >();
							}
						}

						// perform reduction into output element
						addMonoid.getOperator().foldlArray( reduced, zz, AnyOp::blocksize );
						//^--> note that this foldl operates on raw arrays,
						//     and thus should not be mistaken with a foldl
						//     on a grb::Vector.
					}
				}

				// perform element-by-element updates for remainder (if any)
				for( ; i < end - 1; ++i ) {
					OutputType temp;
					const size_t index = coors_r.index( i );
					if( dense || coors_q.assigned( index ) ) {
						apply( temp, a[ index ], b[ index ], anyOp );
						foldr( temp, reduced, addMonoid.getOperator() );
					}
				}

				// write back result
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
				#pragma omp critical
				{ foldr( reduced, z, addMonoid.getOperator() ); }
#else
							z = static_cast< OutputType >( reduced );
#endif
			}
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
		} // end parallel section
#endif
#ifdef _DEBUG
		std::cout << "\t returning " << z << "\n";
#endif

		// done!
		return SUCCESS;
	}

} // namespace internal

/**
 * Calculates the dot product, \f$ z = (x,y) \f$, under the given semiring.
 *
 * @tparam descr      The descriptor to be used (descriptors::no_operation
 *                    if left unspecified).
 * @tparam Ring       The semiring type to use.
 * @tparam OutputType The output type.
 * @tparam InputType1 The input element type of the left-hand input vector.
 * @tparam InputType2 The input element type of the right-hand input vector.
 *
 * @param[out]  z  The output element \f$ \alpha \f$.
 * @param[in]   x  The left-hand input vector.
 * @param[in]   y  The right-hand input vector.
 * @param[in] ring The semiring to perform the dot-product under. If left
 *                 undefined, the default constructor of \a Ring will be used.
 *
 * @return grb::MISMATCH When the dimensions of \a x and \a y do not match. All
 *                       input data containers are left untouched if this exit
 *                       code is returned; it will be as though this call was
 *                       never made.
 * @return grb::SUCCESS  On successful completion of this call.
 *
 * \parblock
 * \par Performance guarantees
 *      -# This call takes \f$ \Theta(n/p) \f$ work at each user process, where
 *         \f$ n \f$ equals the size of the vectors \a x and \a y, and
 *         \f$ p \f$ is the number of user processes. The constant factor
 *         depends on the cost of evaluating the addition and multiplication
 *         operators. A good implementation uses vectorised instructions
 *         whenever the input domains, output domain, and the operators used
 *         allow for this.
 *
 *      -# This call takes \f$ \mathcal{O}(1) \f$ memory beyond the memory used
 *         by the application at the point of a call to this function.
 *
 *      -# This call incurs at most
 *         \f$ n( \mathit{sizeof}(\mathit{D1}) + \mathit{sizeof}(\mathit{D2}) ) + \mathcal{O}(p) \f$
 *         bytes of data movement.
 *
 *      -# This call incurs at most \f$ \Theta(\log p) \f$ synchronisations
 *         between two or more user processes.
 *
 *      -# A call to this function does result in any system calls.
 * \endparblock
 *
 * \note This requires an implementation to pre-allocate \f$ \Theta(p) \f$
 *       memory for inter-process reduction, if the underlying communication
 *       layer indeed requires such a buffer. This buffer may not be allocated
 *       (nor freed) during a call to this function.
 *
 * \parblock
 * \par Valid descriptors
 *   -# grb::descriptors::no_operation
 *   -# grb::descriptors::no_casting
 * \endparblock
 */
template< Descriptor descr = descriptors::no_operation, class AddMonoid, class AnyOp, typename OutputType, typename InputType1, typename InputType2, typename Coords >
RC dot( OutputType & z,
	const Vector< InputType1, reference, Coords > & x,
	const Vector< InputType2, reference, Coords > & y,
	const AddMonoid & addMonoid = AddMonoid(),
	const AnyOp & anyOp = AnyOp(),
	const typename std::enable_if< ! grb::is_object< OutputType >::value && ! grb::is_object< InputType1 >::value && ! grb::is_object< InputType2 >::value && grb::is_monoid< AddMonoid >::value &&
			grb::is_operator< AnyOp >::value,
		void >::type * const = NULL ) {
	// static sanity checks
	NO_CAST_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< InputType1, typename AnyOp::D1 >::value ), "grb::dot",
		"called with a left-hand vector value type that does not match the first "
		"domain of the given multiplicative operator" );
	NO_CAST_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< InputType2, typename AnyOp::D2 >::value ), "grb::dot",
		"called with a right-hand vector value type that does not match the second "
		"domain of the given multiplicative operator" );
	NO_CAST_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename AddMonoid::D3, typename AnyOp::D1 >::value ), "grb::dot",
		"called with a multiplicative operator output domain that does not match "
		"the first domain of the given additive operator" );
	NO_CAST_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< OutputType, typename AddMonoid::D2 >::value ), "grb::dot",
		"called with an output vector value type that does not match the second "
		"domain of the given additive operator" );
	NO_CAST_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< typename AddMonoid::D3, typename AddMonoid::D2 >::value ), "grb::dot",
		"called with an additive operator whose output domain does not match its "
		"second input domain" );
	NO_CAST_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< OutputType, typename AddMonoid::D3 >::value ), "grb::dot",
		"called with an output vector value type that does not match the third "
		"domain of the given additive operator" );

	// dynamic sanity check
	const size_t n = internal::getCoordinates( y ).size();
	if( internal::getCoordinates( x ).size() != n ) {
		return MISMATCH;
	}

	// cache nnzs
	const size_t nnzx = internal::getCoordinates( x ).nonzeroes();
	const size_t nnzy = internal::getCoordinates( y ).nonzeroes();

	// catch trivial case
	if( nnzx == 0 && nnzy == 0 ) {
		z = addMonoid.template getIdentity< OutputType >();
		return SUCCESS;
	}

	// if descriptor says nothing about being dense...
	if( ! ( descr & descriptors::dense ) ) {
		// check if inputs are actually dense...
		if( nnzx == n && nnzy == n ) {
			// call implementation with the right descriptor
			return internal::dot_generic< descr | descriptors::dense >( z, x, y, addMonoid, anyOp );
		}
	} else {
		// descriptor says dense, but if any of the vectors are actually sparse...
		if( internal::getCoordinates( x ).nonzeroes() < n || internal::getCoordinates( y ).nonzeroes() < n ) {
			// call implementation with corrected descriptor
			return internal::dot_generic< descr & ~( descriptors::dense ) >( z, x, y, addMonoid, anyOp );
		}
	}

	// all OK, pass to implementation
	return internal::dot_generic< descr >( z, x, y, addMonoid, anyOp );
}

/** No implementation notes. */
template< typename Func, typename DataType, typename Coords >
RC eWiseMap( const Func f, Vector< DataType, reference, Coords > & x ) {
	const auto & coors = internal::getCoordinates( x );
	if( coors.isDense() ) {
		// vector is distributed sequentially, so just loop over it
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
		#pragma omp parallel for schedule( \
		dynamic, config::CACHE_LINE_SIZE::value() )
#endif
		for( size_t i = 0; i < coors.size(); ++i ) {
			// apply the lambda
			DataType & xval = internal::getRaw( x )[ i ];
			xval = f( xval );
		}
	} else {
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
		#pragma omp parallel for schedule( \
		dynamic, config::CACHE_LINE_SIZE::value() )
#endif
		for( size_t k = 0; k < coors.nonzeroes(); ++k ) {
			DataType & xval = internal::getRaw( x )[ coors.index( k ) ];
			xval = f( xval );
		}
	}
	// and done!
	return SUCCESS;
}

/**
 * This is the eWiseLambda that performs length checking by recursion.
 *
 * in the reference implementation all vectors are distributed equally, so no
 * need to synchronise any data structures. We do need to do error checking
 * though, to see when to return grb::MISMATCH. That's this function.
 *
 * @see Vector::operator[]()
 * @see Vector::lambda_reference
 */
template< typename Func, typename DataType1, typename DataType2, typename Coords, typename... Args >
RC eWiseLambda( const Func f, const Vector< DataType1, reference, Coords > & x, const Vector< DataType2, reference, Coords > & y, Args const &... args ) {
	// catch mismatch
	if( size( x ) != size( y ) ) {
		return MISMATCH;
	}
	// continue
	return eWiseLambda( f, x, args... );
}

/**
 * No implementation notes. This is the `real' implementation on reference
 * vectors.
 *
 * @see Vector::operator[]()
 * @see Vector::lambda_reference
 */
template< typename Func, typename DataType, typename Coords >
RC eWiseLambda( const Func f, const Vector< DataType, reference, Coords > & x ) {
#ifdef _DEBUG
	std::cout << "Info: entering eWiseLambda function on vectors.\n";
#endif
	const auto & coors = internal::getCoordinates( x );
	if( coors.isDense() ) {
		// vector is distributed sequentially, so just loop over it
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
		#pragma omp parallel for schedule( \
		dynamic, config::CACHE_LINE_SIZE::value() )
#endif
		for( size_t i = 0; i < coors.size(); ++i ) {
			// apply the lambda
#ifdef _DEBUG
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
			#pragma omp critical
#endif
#endif
			f( i );
		}
	} else {
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
		#pragma omp parallel for schedule( \
		dynamic, config::CACHE_LINE_SIZE::value() )
#endif
		for( size_t k = 0; k < coors.nonzeroes(); ++k ) {
			const size_t i = coors.index( k );
#ifdef _DEBUG
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
			#pragma omp critical
#endif
			std::cout << "\tprocessing coordinate " << k << " which has index " << i << "\n";
#endif
#ifdef _DEBUG
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
			#pragma omp critical
#endif
#endif
			f( i );
		}
	}
	// and done!
	return SUCCESS;
}

/**
 * Reduces a vector into a scalar. Reduction takes place according a monoid
 * \f$ (\oplus,1) \f$, where \f$ \oplus:\ D_1 \times D_2 \to D_3 \f$ with an
 * associated identity \f$ 1 \in \{D_1,D_2,D_3\} \f$. Elements from the given
 * vector \f$ y \in \{D_1,D_2\} \f$ will be applied at the left-hand or right-
 * hand side of \f$ \oplus \f$; which, exactly, is implementation-dependent
 * but should not matter since \f$ \oplus \f$ should be associative.
 *
 * Let \f$ x_0 = 1 \f$ and let
 * \f$ x_{i+1} = \begin{cases}
 *   x_i \oplus y_i\text{ if }y_i\text{ is nonzero}
 *   x_i\text{ otherwise}
 * \end{cases},\f$
 * for all \f$ i \in \{ 0, 1, \ldots, n-1 \} \f$. On function exit \a x will be
 * set to \f$ x_n \f$.
 *
 * This function assumes that \f$ \odot \f$ under the given domains consitutes
 * a valid monoid, which for standard associative operators it usually means
 * that \f$ D_3 \subseteq D_2 \subseteq D_1 \f$. If not, or if the operator is
 * non-standard, the monoid axioms are to be enforced in some other way-- the
 * user is responsible for checking this is indeed the case or undefined
 * behaviour will occur.
 *
 * \note While the monoid identity may be used to easily provide parallel
 *       implementations of this function, having a notion of an identity is
 *       mandatory to be able to interpret sparse vectors; this is why we do
 *       not allow a plain operator to be passed to this function.
 *
 * @tparam descr     The descriptor to be used (descriptors::no_operation if
 *                   left unspecified).
 * @tparam Monoid    The monoid to use for reduction. A monoid is required
 *                   because the output value \a y needs to be initialised
 *                   with an identity first.
 * @tparam InputType The type of the elements in the supplied GraphBLAS
 *                   vector \a y.
 * @tparam IOType    The type of the output value \a x.
 *
 * @param[out]   x   The result of reduction.
 * @param[in]    y   A valid GraphBLAS vector. This vector may be sparse.
 * @param[in] monoid The monoid under which to perform this reduction.
 *
 * @return grb::SUCCESS When the call completed successfully.
 * @return grb::ILLEGAL If the provided input vector \a y was not dense.
 * @return grb::ILLEGAL If the provided input vector \a y was empty.
 *
 * \parblock
 * \par Valid descriptors
 * grb::descriptors::no_operation, grb::descriptors::no_casting,
 * grb::descriptors::dense
 *
 * \note Invalid descriptors will be ignored.
 *
 * If grb::descriptors::no_casting is specified, then 1) the first domain of
 * \a monoid must match \a InputType, 2) the second domain of \a op must match
 * \a IOType, and 3) the third domain must match \a IOType. If one of
 * these is not true, the code shall not compile.
 * \endparblock
 *
 * \parblock
 * \par Performance guarantees
 *      -# This call comprises \f$ \Theta(n) \f$ work, where \f$ n \f$ equals
 *         the size of the vector \a x. The constant factor depends on the
 *         cost of evaluating the underlying binary operator. A good
 *         implementation uses vectorised instructions whenever the input
 *         domains, the output domain, and the operator used allow for this.
 *
 *      -# This call will not result in additional dynamic memory allocations.
 *         No system calls will be made.
 *
 *      -# This call takes \f$ \mathcal{O}(1) \f$ memory beyond the memory
 *         used by the application at the point of a call to this function.
 *
 *      -# This call incurs at most
 *         \f$ n \mathit{sizeof}(\mathit{InputType}) + \mathcal{O}(1) \f$
 *         bytes of data movement. If \a y is sparse, a call to this function
 *         incurs at most \f$ n \mathit{sizeof}( \mathit{bool} ) \f$ extra
 *         bytes of data movement.
 * \endparblock
 *
 * @see grb::foldl provides similar functionality.
 */
template< Descriptor descr = descriptors::no_operation, class Monoid, typename InputType, typename IOType, typename MaskType, typename Coords >
RC foldl( IOType & x,
	const Vector< InputType, reference, Coords > & y,
	const Vector< MaskType, reference, Coords > & mask,
	const Monoid & monoid = Monoid(),
	const typename std::enable_if< ! grb::is_object< IOType >::value && ! grb::is_object< InputType >::value && ! grb::is_object< MaskType >::value && grb::is_monoid< Monoid >::value,
		void >::type * const = NULL ) {
#ifdef _DEBUG
	std::cout << "foldl: IOType <- [InputType] with a monoid called. Array has size " << size( y ) << " with " << nnz( y ) << " nonzeroes. It has a mask of size " << size( mask ) << " with "
			  << nnz( mask ) << " nonzeroes.\n";
#endif

	// static sanity checks
	NO_CAST_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< IOType, InputType >::value ), "grb::reduce", "called with a scalar IO type that does not match the input vector type" );
	NO_CAST_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< InputType, typename Monoid::D1 >::value ), "grb::reduce",
		"called with an input vector value type that does not match the first "
		"domain of the given monoid" );
	NO_CAST_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< InputType, typename Monoid::D2 >::value ), "grb::reduce",
		"called with an input vector type that does not match the second domain of "
		"the given monoid" );
	NO_CAST_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< InputType, typename Monoid::D3 >::value ), "grb::reduce",
		"called with an input vector type that does not match the third domain of "
		"the given monoid" );
	NO_CAST_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< bool, MaskType >::value ), "grb::reduce", "called with a vector mask type that is not boolean" );

	// dynamic sanity checks
	if( size( mask ) > 0 && size( mask ) != size( y ) ) {
		return MISMATCH;
	}

	// do minimal work
	RC ret = SUCCESS;
	IOType global_reduced = monoid.template getIdentity< IOType >();

	// check if we have a mask
	const bool masked = internal::getCoordinates( mask ).size() > 0;

	// TODO internal issue #194
#ifdef _H_GRB_REFERENCE_OMP_BLAS1
	#pragma omp parallel
	{
		IOType local_x = monoid.template getIdentity< IOType >();
		//#pragma omp for schedule(dynamic,config::CACHE_LINE_SIZE::value())
		#pragma omp for //note: using the above dynamic schedule wreaks havoc on performance on this kernel (tested 25/08/2017)
		for( size_t i = 0; i < internal::getCoordinates( y ).size(); ++i ) {
			// if mask OK and there is a nonzero
			if( ( ! masked || internal::getCoordinates( mask ).template mask< descr >( i, internal::getRaw( mask ) ) ) && internal::getCoordinates( y ).assigned( i ) ) {
				const RC rc = foldl( local_x, internal::getRaw( y )[ i ], monoid.getOperator() );
				assert( rc == SUCCESS );
				if( rc != SUCCESS ) {
					ret = rc;
				}
			}
		}
		#pragma omp critical
		{
			const RC rc = foldl< descr >( global_reduced, local_x, monoid.getOperator() );
			assert( rc == SUCCESS );
			if( rc != SUCCESS ) {
				ret = rc;
			}
		}
	}
#else
					// if masked or sparse
					if( masked || internal::getCoordinates( y ).nonzeroes() < internal::getCoordinates( y ).size() ) {
						// sparse case
						for( size_t i = 0; i < internal::getCoordinates( y ).size(); ++i ) {
							// if mask OK and there is a nonzero
							if( ( ! masked || internal::getCoordinates( mask ).template mask< descr >( i, internal::getRaw( mask ) ) ) && internal::getCoordinates( y ).assigned( i ) ) {
								// fold it into y
								RC rc = foldl( global_reduced, internal::getRaw( y )[ i ], monoid.getOperator() );
								assert( rc == SUCCESS );
								if( rc != SUCCESS ) {
									ret = rc;
								}
							}
						}
					} else {
						// dense case relies on foldlArray
						monoid.getOperator().foldlArray( global_reduced, internal::getRaw( y ), internal::getCoordinates( y ).nonzeroes() );
					}
#endif

	// do accumulation
	if( ret == SUCCESS ) {
#ifdef _DEBUG
		std::cout << "Accumulating " << global_reduced << " into " << x << " using foldl\n";
#endif
		ret = foldl( x, global_reduced, monoid.getOperator() );
	}

	// done
	return ret;
}

/**
 * TODO internal issue #195 
 */
template< Descriptor descr = descriptors::no_operation, typename T, typename U, typename Coords >
RC zip( Vector< std::pair< T, U >, reference, Coords > & z,
	const Vector< T, reference, Coords > & x,
	const Vector< U, reference, Coords > & y,
	const typename std::enable_if< ! grb::is_object< T >::value && ! grb::is_object< U >::value, void >::type * const = NULL ) {
	const size_t n = size( z );
	if( n != size( x ) ) {
		return MISMATCH;
	}
	if( n != size( y ) ) {
		return MISMATCH;
	}
	if( nnz( x ) < n ) {
		return ILLEGAL;
	}
	if( nnz( y ) < n ) {
		return ILLEGAL;
	}
	auto & z_coors = internal::getCoordinates( z );
	const T * const x_raw = internal::getRaw( x );
	const U * const y_raw = internal::getRaw( y );
	std::pair< T, U > * z_raw = internal::getRaw( z );
	z_coors.assignAll();
	for( size_t i = 0; i < n; ++i ) {
		z_raw[ i ].first = x_raw[ i ];
		z_raw[ i ].second = y_raw[ i ];
	}
	return SUCCESS;
}

/**
 * TODO internal issue #195 
 */
template< Descriptor descr = descriptors::no_operation, typename T, typename U, typename Coords >
RC unzip( Vector< T, reference, Coords > & x,
	Vector< U, reference, Coords > & y,
	const Vector< std::pair< T, U >, reference, Coords > & in,
	const typename std::enable_if< ! grb::is_object< T >::value && ! grb::is_object< U >::value, void >::type * const = NULL ) {
	const size_t n = size( in );
	if( n != size( x ) ) {
		return MISMATCH;
	}
	if( n != size( y ) ) {
		return MISMATCH;
	}
	if( nnz( in ) < n ) {
		return ILLEGAL;
	}
	auto & x_coors = internal::getCoordinates( x );
	auto & y_coors = internal::getCoordinates( y );
	T * const x_raw = internal::getRaw( x );
	U * const y_raw = internal::getRaw( y );
	const std::pair< T, U > * in_raw = internal::getRaw( in );
	x_coors.assignAll();
	y_coors.assignAll();
	for( size_t i = 0; i < n; ++i ) {
		x_raw[ i ] = in_raw[ i ].first;
		y_raw[ i ] = in_raw[ i ].second;
	}
	return SUCCESS;
}

/** @} */
//   ^-- ends BLAS-1 module

} // end namespace ``grb''

#undef NO_CAST_ASSERT
#undef NO_CAST_OP_ASSERT

// parse this unit again for OpenMP support
#ifdef _GRB_WITH_OMP
#ifndef _H_GRB_REFERENCE_OMP_BLAS1
#define _H_GRB_REFERENCE_OMP_BLAS1
#define reference reference_omp
#include "graphblas/reference/blas1.hpp"
#undef reference
#undef _H_GRB_REFERENCE_OMP_BLAS1
#endif
#endif

#endif // end `_H_GRB_REFERENCE_BLAS1'