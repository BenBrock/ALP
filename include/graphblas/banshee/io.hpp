
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
 */

#if ! defined _H_GRB_BANSHEE_IO
#define _H_GRB_BANSHEE_IO

#include "graphblas/banshee/coordinates.hpp"
#include "graphblas/banshee/vector.hpp"
#include "graphblas/io.hpp"
#include "graphblas/utils/SynchronizedNonzeroIterator.hpp"

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
		"* Possible fix 2 | Provide a value input iterator with element "      \
		"types that match the output vector element type.\n"                   \
		"* Possible fix 3 | If applicable, provide an index input iterator "   \
		"with element types that are integral.\n"                              \
		"********************************************************************" \
		"********************************************************************" \
		"******************************\n" );

namespace grb {

	/**
	 * \defgroup IO Data Ingestion
	 * @{
	 */

	/**
	 * Ingests raw data into a GraphBLAS vector.
	 *
	 * This is the direct variant without iterator output position updates.
	 *
	 * The input is given by iterators. The \a start position will be assumed to
	 * contain a value to be added to this vector at index \a 0. The \a start
	 * position will be incremented up to \a n times.
	 *
	 * An element found at a position that has been incremented \a i times will
	 * be added to this vector at index \a i.
	 *
	 * If, when adding a value \a x to index \a i, an existing value at the same
	 * index position was found, then the given \a Dup will be used to
	 * combine the two values. \a Dup must be a binary operator; the old
	 * value will be used as the left-hand side input, the new value from the
	 * current iterator position as its right-hand side input. The result of
	 * applying the operator defines the new value at position \a i.
	 *
	 * \warning If there is no \a Dup type nor \a dup instance provided then
	 * grb::operators::right_assign will be assumed-- this means new values will
	 * simply overwrite old values.
	 *
	 * \warning If, on input, \a x is not empty, new values will be combined with
	 * old ones by use of \a Dup.
	 *
	 * \note To ensure all old values of \a x are deleted, simply preface a call
	 * to this function by one to grb::clear(x).
	 *
	 * If, after \a n increments of the \a start position, that incremented
	 * position is not found to equal the given \a end position, this function
	 * will return grb::MISMATCH. The \a n elements that were found, however,
	 * will have been added to the vector; the remaining items in the iterator
	 * range will simply be ignored.
	 * If \a start was incremented \a i times with \f$ i < n \f$ and is found to
	 * be equal to \a end, grb::MISMATCH will be returned as well. The \a i
	 * values that were extracted from \a start on will still have been added to
	 * the output vector \a x.
	 *
	 * Since this function lacks explicit input for the index of each vector
	 * element, IOMode::parallel is <em>not supported</em>.
	 *
	 * \warning If \a P is larger than one and \a IOMode is parallel, this
	 *          function will return grb::RC::ILLEGAL and will have no other
	 *          effect.
	 *
	 * @tparam descr        The descriptors passed to this function call.
	 * @tparam InputType    The type of the vector elements.
	 * @tparam Dup          The class of the operator used to resolve
	 *                      duplicated entries.
	 * @tparam fwd_iterator The type of the input forward iterator.
	 *
	 * \parblock
	 * \par Valid descriptors
	 *   -# grb::descriptors::no_operation for default behaviour;
	 *   -# grb::descriptors::no_casting which will cause compilation to fail
	 *      whenever \a InputType does not match \a fwd_iterator::value_type.
	 * \endparblock
	 *
	 * @param[in,out] x     The vector to update with new input values.
	 * @param[in,out] start On input:  the start position of the input forward
	 *                                 iterator.
	 *                      On output: the position after the last increment
	 *                                 performed while calling this function.
	 * @param[in]     end   The end position of the input forward iterator.
	 * @param[in]     dup   The operator to use for resolving write conflicts.
	 *
	 * \warning Use of this function, which is grb::IOMode::sequential, leads to
	 *          unscalable performance and should thus be used with care!
	 *
	 * @return grb::SUCCESS  Whenever \a n new elements from \a start to \a end
	 *                       were successfully added to \a x, where \a n is the
	 *                       size of this vector.
	 * @return grb::MISMATCH Whenever the number of elements between \a start to
	 *                       \a end does not equal \a n. When this is returned,
	 *                       the output vector \a x is still updated with
	 *                       whatever values that were successfully extracted
	 *                       from \a start. If this is not exected behaviour, the
	 *                       user could, for example, catch this error code and
	 *                       call grb::clear.
	 * @return grb::OUTOFMEM Whenever not enough capacity could be allocated to
	 *                       store the input from \a start to \a end. The output
	 *                       vector \a x is guaranteed to contain all values up to
	 *                       the returned position \a start.
	 * @return grb::ILLEGAL  Whenever \a mode is parallel while the number of user
	 *                       processes is larger than one.
	 * @return grb::PANIC    Whenever an un-mitigable error occurs. The state of
	 *                       the GraphBLAS library and all associated containers
	 *                       becomes undefined.
	 *
	 * \parblock
	 * \par Performance guarantees (IOMode::sequential).
	 * A call to this function
	 *   -# comprises \f$ \mathcal{O}( n ) \f$ work <em>per user process</em>,
	 *      where \a n is the vector size.
	 *   -# results in at most \f$ n \mathit{sizeof}( T ) \f$ bytes of data
	 *      movement, where \a T is the underlying data type of the input
	 *      iterator, <em>per user process</em>.
	 *   -# Results in at most \f$   n \mathit{sizeof}( \mathit{InputType} ) +
	 *                             2 n \mathit{sizeof}( \mathit{bool} ) \f$
	 *      bytes of data movement that may be distributed over multiple user
	 *      processes.
	 *   -# if the capacity of this vector is not large enough to hold \a n
	 *      elements, a call to this function may allocate \f$ \mathcal{O}( n ) \f$
	 *      new bytes of memory which may be distributed over multiple user
	 *      processes.
	 *   -# if the capacity of this vector is not large enough to hold \a n
	 *      elements, this function may make system calls at any of the user
	 *      processes.
	 * \endparblock
	 *
	 * \parblock
	 * \par Performance guarantees (IOMode::parallel).
	 * Not supported.
	 * \endparblock
	 */
	template< Descriptor descr = descriptors::no_operation, typename InputType, typename fwd_iterator, class Dup = operators::right_assign< InputType > >
	RC buildVector( Vector< InputType, banshee > & x, fwd_iterator start, const fwd_iterator end, const IOMode mode, const Dup & dup = Dup() ) {
		// static sanity check
		NO_CAST_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< InputType, decltype( *std::declval< fwd_iterator >() ) >::value ), "grb::buildVector (banshee implementation)",
			"Input iterator does not match output vector type while no_casting "
			"descriptor was set" );

		// in the sequential banshee implementation, the number of user processes always equals 1
		// therefore the sequential and parallel modes are equivalent
#ifndef NDEBUG
		assert( mode == SEQUENTIAL || mode == PARALLEL );
#else
		(void)mode;
#endif

		// declare temporary to meet delegate signature
		const fwd_iterator start_pos = start;

		// do delegate
		return x.template build< descr >( dup, start_pos, end, start );
	}

	/**
	 * Ingests raw data into a GraphBLAS vector. This is the coordinate-wise
	 * version.
	 *
	 * The input is given by iterators. The \a val_start position will be assumed
	 * to contain a value to be added to this vector at index pointed to by
	 * \a ind_start. The same remains true if both \a val_start and \a ind_start
	 * positions are incremented.
	 *
	 * When multiple iterator position pairs correspond to a new nonzero value
	 * at the same position \a i, then those values are combined using the given
	 * \a duplicate operator. \a Merger must be an \em associative binary
	 * operator.
	 *
	 * If, when adding a value \a x to index \a i an existing value at the same
	 * index position was found, then the given \a dup will be used to combine
	 * the two values. \a Dup must be a binary operator; the old value will be
	 * used as the left-hand side input, the new value from the current iterator
	 * position as its right-hand side input. The result of applying the operator
	 * defines the new value at position \a i.
	 *
	 * \warning If there is no \a Dup type nor \a dup instance provided then
	 * grb::operators::right_assign will be assumed-- this means new values will
	 * simply overwrite old values.
	 *
	 * \warning If, on input, \a x is not empty, new values will be combined with
	 * old ones by use of \a Dup.
	 *
	 * \note To ensure all old values of \a x are deleted, simply preface a call
	 * to this function by one to grb::clear(x).
	 *
	 * If, after \a n increments of the \a start position, that incremented
	 * position is not found to equal the given \a end position, this function
	 * will return grb::MISMATCH. The \a n elements that were found, however,
	 * will have been added to the vector; the remaining items in the iterator
	 * range will simply be ignored.
	 * If \a start was incremented \a i times with \f$ i < n \f$ and is found to
	 * be equal to \a end, grb::MISMATCH will be returned as well. The \a i
	 * values that were extracted from \a start on will still have been added to
	 * the output vector \a x.
	 *
	 * This function, like with all GraphBLAS I/O, has two modes as detailed in
	 * \a IOMode. In case of IOMode::sequential, all \a P user processes are
	 * expected to provide iterators with exactly the same context across all
	 * processes.
	 * In case of IOMode::parallel, the \a P user processes are expected to be
	 * provided disjoint parts of the input that make up the entire vector. The
	 * following two vectors \a x and \a y thus are equal:
	 * \code
	 * size_t s = ...; //let s be 0 or 1, the ID of this user process;
	 *                 //assume P=2 user processes total.
	 * double raw[8] = {0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0};
	 * size_t ind[8] = {0  , 1,   2,   3,   4,   5,   6,   7,   8,   9,   10};
	 *
	 * const double * const r = &(raw[0]); //get a standard C pointer to the data.
	 * const size_t * const i = &(ind[0]); //get a C pointer to the indices.
	 *
	 * grb::init( s, 2 );
	 * grb::Vector<double> x( 8 ), y( 8 );
	 * x.buildVector( x, i,       i + 8      , r,       r + 8,       sequential );
	 * y.buildVector( y, i + s*4, i + s*4 + 4, r + s*4, r + s*4 + 4, parallel );
	 * ...
	 *
	 * grb::finalize();
	 * \endcode
	 *
	 * \warning While the above is semantically equivalent, their performance
	 *          characteristics are not. Please see the below for details.
	 *
	 * @tparam descr         The descriptor to be used (descriptors::no_operation
	 * 			 if left unspecified).
	 * @tparam Dup           The type of the operator used to resolve inputs to
	 *                       pre-existing vector contents. The default Dup simply
	 *                       overwrites pre-existing content.
	 * @tparam InputType     The type of values stored by the vector.
	 * @tparam fwd_iterator1 The type of the iterator to be used for index value
	 *                       input.
	 * @tparam fwd_iterator2 The type of the iterator to be used for nonzero
	 *                       value input.
	 *
	 * \parblock
	 * \par Valid descriptors
	 *   -# grb::descriptors::no_operation for default behaviour;
	 *   -# grb::descriptors::no_casting which will cause compilation to fail
	 *      whenever 1) \a InputType does not match \a fwd_iterator2::value_type,
	 *      or whenever 2) \a fwd_iterator1::value_type is not an integral type.
	 * \endparblock
	 *
	 * @param[out]    x         Where the ingested data is to be added.
	 * @param[in,out] ind_start On input:  the start position of the indices
	 *                                     to be inserted. This iterator reports,
	 *                                     for every nonzero to be inserted, its
	 *                                     index value.
	 *                          On output: the position after the last increment
	 *                                     performed while calling this function.
	 * @param[in]  ind_end      The end iterator corresponding to \a ind_start.
	 * @param[in]  val_start    On input:  the start iterator of the auxiliary
	 *                                     data to be inserted. This iterator
	 *                                     reports, for every nonzero to be
	 *                                     inserted, its nonzero value.
	 *                          On output: the position after the last increment
	 *                                     performed while calling this function.
	 * @param[in]  val_end     The end iterator corresponding to \a val_start.
	 * @param[in]  mode        The IOMode of this call. By default this is set to
	 *                         IOMode::parallel.
	 * @param[in]  dup         The operator that resolves input to pre-existing
	 *                         vector entries.
	 *
	 * \warning Use of IOMode::sequential leads to unscalable performance and
	 *          should be used with care.
	 *
	 * @return grb::SUCCESS  Whenever \a n new elements from \a start to \a end
	 *                       were successfully added to \a x, where \a n is the
	 *                       size of this vector.
	 * @return grb::MISMATCH Whenever an element from \a ind_start is larger or
	 *                       equal to \a n. When this is returned, the output
	 *                       vector \a x is still updated with whatever values
	 *                       that were successfully extracted from \a ind_start
	 *                       and \a val_start.
	 *                       If this is not exected behaviour, the user could,
	 *                       for example, catch this error code and followed by
	 *                       a call to grb::clear.
	 * @return grb::OUTOFMEM Whenever not enough capacity could be allocated to
	 *                       store the input from \a start to \a end. The output
	 *                       vector \a x is guaranteed to contain all values up to
	 *                       the returned position \a start.
	 * @return grb::PANIC    Whenever an un-mitigable error occurs. The state of
	 *                       the GraphBLAS library and all associated containers
	 *                       becomes undefined.
	 *
	 * \parblock
	 * \par Performance guarantees (IOMode::sequential).
	 * A call to this function
	 *   -# comprises \f$ \mathcal{O}( n ) \f$ work <em>per user process</em>,
	 *      where \a n is the vector size.
	 *   -# results in at most
	 *         \f$ n ( \mathit{sizeof}( T ) \mathit{sizeof}( U ) \f$
	 *      bytes of data movement, where \a T and \a U are the underlying data
	 *      types of each of the input iterators, <em>per user process</em>.
	 *   -# Results in at most \f$   n \mathit{sizeof}( \mathit{InputType} ) +
	 *                             2 n \mathit{sizeof}( \mathit{bool} ) \f$
	 *      bytes of data movement that may be distributed over multiple user
	 *      processes.
	 *   -# if the capacity of this vector is not large enough to hold \a n
	 *      elements, a call to this function may allocate
	 *         \f$ \mathcal{O}( n ) \f$
	 *      new bytes of memory which \em may be distributed over multiple user
	 *      processes.
	 *   -# if the capacity of this vector is not large enough to hold \a n
	 *      elements, this function may make system calls at any of the user
	 *      processes.
	 * \endparblock
	 *
	 * \parblock
	 * \par Performance guarantees (IOMode::parallel).
	 * A call to this function
	 *   -# comprises \f$ \mathcal{O}( n ) \f$ work where \a n is the vector size.
	 *      Unlike with IOMode::sequential, this work may be distributed over the
	 *      multiple user processes.
	 *   -# results in at most \f$   n \mathit{sizeof}( T ) +
	 *                               n \mathit{sizeof}( U ) +
	 *                               n \mathit{sizeof}( \mathit{InputType} ) +
	 *                             2 n \mathit{sizeof}( \mathit{bool} ) \f$
	 *      bytes of data movement, where \a T and \a U are the underlying data
	 *      types of the input iterators. All of these may be distributed over
	 *      multiple user processes.
	 *   -# if the capacity of this vector is not large enough to hold \a n
	 *      elements, a call to this function may allocate
	 *         \f$ \mathcal{O}( n ) \f$
	 *      new bytes of memory which \em may be distributed over multiple user
	 *      processes.
	 *   -# if the capacity of this vector is not large enough to hold \a n
	 *      elements, a call to this function may result in system calls at any of
	 *      the user processes.
	 * \endparblock
	 */
	template< Descriptor descr = descriptors::no_operation, typename InputType, typename fwd_iterator1, typename fwd_iterator2, class Dup = operators::right_assign< InputType > >
	RC buildVector( Vector< InputType, banshee > & x,
		fwd_iterator1 ind_start,
		const fwd_iterator1 ind_end,
		fwd_iterator2 val_start,
		const fwd_iterator2 val_end,
		const IOMode mode,
		const Dup & dup = Dup() ) {
		// static sanity check
		NO_CAST_ASSERT( ( ! ( descr & descriptors::no_casting ) || std::is_same< InputType, decltype( *std::declval< fwd_iterator2 >() ) >::value ||
							std::is_integral< decltype( *std::declval< fwd_iterator1 >() ) >::value ),
			"grb::buildVector (banshee implementation)",
			"At least one input iterator has incompatible value types while "
			"no_casting descriptor was set" );

		// in the sequential banshee implementation, the number of user processes always equals 1
		// therefore the sequential and parallel modes are equivalent
#ifndef NDEBUG
		assert( mode == SEQUENTIAL || mode == PARALLEL );
#else
		(void)mode;
#endif

		// call the private member function that provides this functionality
		return x.template build< descr >( dup, ind_start, ind_end, val_start, val_end );
	}

	/*
	 * @see grb::buildMatrix.
	 *
	 * @see grb::buildMatrixUnique calls this function when
	 *                             grb::descriptors::no_duplicates is passed.
	 *
	template<
	    Descriptor descr = descriptors::no_operation,
	    template< typename, typename, typename > class accum = operators::right_assign,
	    template< typename, typename, typename > class dup   = operators::add,
	    typename InputType,
	    typename fwd_iterator1 = const size_t *__restrict__,
	    typename fwd_iterator2 = const size_t *__restrict__,
	    typename fwd_iterator3 = const InputType  *__restrict__,
	    typename length_type = size_t
	>
	RC buildMatrix(
	    Matrix< InputType, banshee > &A,
	    const fwd_iterator1 I,
	    const fwd_iterator2 J,
	    const fwd_iterator3 V,
	    const length_type nz,
	    const IOMode mode
	) {
	    //delegate in case of no duplicats
	    if( descr & descriptors::no_duplicates ) {
	        return buildMatrixUnique( A, I, J, V, nz, mode );
	    }
	    assert( false );
	    return PANIC;
	}*/

	/**
	 * Calls the other #buildMatrixUnique variant.
	 * @see grb::buildMatrixUnique for the user-level specification.
	 */
	template< Descriptor descr = descriptors::no_operation, typename InputType, typename fwd_iterator >
	RC buildMatrixUnique( Matrix< InputType, banshee > & A, fwd_iterator start, const fwd_iterator end, const IOMode mode ) {
		// parallel or sequential mode are equivalent for banshee implementation
		assert( mode == PARALLEL || mode == SEQUENTIAL );
#ifdef NDEBUG
		(void)mode;
#endif
		return A.template buildMatrixUnique< descr >( start, end );
	}

	/** @} */

} // namespace grb

#undef NO_CAST_ASSERT

#endif // end ``_H_GRB_BANSHEE_IO