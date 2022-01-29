
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

/**
 * @file ndim_matrix_builders.hpp
 * @author Alberto Scolari (alberto.scolari@huawei.com)
 * @brief Utilities to build matrices for an HPCG simulation in a generic number of dimensions
 *
 * In particular, the main matrices are:
 * - a system matrix, generated from an N-dimenional space of coordinates by iterating along
 *   each dimension in priority order, where the first dimension has highest priority and the last
 *   dimension least priority; for each point (row), all its N-dimensional neighbours within
 *   a given distance are generated for the column
 * - a coarsening matrix, generated by iterating on a coarser system of N dimensions (row) and projecting
 *   each point to a corresponding system of finer sizes
 *
 * @date 2021-04-30
 */

#ifndef _H_NDIM_MATRIX_BUILDERS
#define _H_NDIM_MATRIX_BUILDERS

#include <algorithm>
#include <array>
#include <cstddef>
#include <initializer_list>
#include <numeric>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace grb {
	namespace utils {

		/**
		 * @brief Base class that iterates on DIMS dimensions starting from the first one.
		 *
		 * The coordinates are assumed to generate the row number in a matrix whose number of rows is
		 * the product of all sizes. This class generates row numbers for physical problems described as
		 * systems of linear equations in an n-dimensional space.
		 *
		 * Example of iterations in a 3D (x, y, z) system of size (4,3,2), with generated row numbers
		 * reported as '=> ROW':
		 * - z[0]
		 * - y[0]
		 * - x[0] => 0, x[1] => 1, x[2] => 2, x[3] => 3
		 * - y[1]
		 * - x[0] => 4, x[1] => 5, x[2] => 6, x[3] => 7
		 * - y[2]
		 * - x[0] => 8, x[1] => 9, x[2] => 10, x[3] => 11
		 * - z[1]
		 * - y[0]
		 * - x[0] => 12, x[1] => 13, x[2] => 14, x[3] => 15
		 * - y[1]
		 * - x[0] => 16, x[1] => 17, x[2] => 18, x[3] => 19
		 * - y[2]
		 * - x[0] => 20, x[1] => 21, x[2] => 22, x[3] => 23
		 *
		 * The main goal of this class is to be derived by other classes to generate matrices in an
		 * STL-iterator-fashion; hence, this class contains all the code for basic coordinate-to-row-column
		 * conversion in \p DIM dimensions and the basic logic to increment the row number.
		 *
		 * @tparam DIMS number os dimensions of the system
		 */
		template< std::size_t DIMS >
		struct row_generator {

			using row_coordinate_type = std::size_t; ///< numeric type of rows
			using array_t = std::array< row_coordinate_type,
				DIMS >; ///< type for the array storing the coordinates.

			const array_t physical_sizes; ///< size of each dimension, starting from the one to be explored first

			/**
			 * @brief Construct a new row generator object
			 * @param[in] _sizes array of sizes of each dimension; no dimension should be 0, otherwise an exception
			 *                   is thrown
			 * @param[in] first_row first row to iterate from; it is allowed to be beyond the matrix size, e.g. to create
			 *                      an end iterator (no check occurs)
			 */
			row_generator( const array_t & _sizes, row_coordinate_type first_row ) : physical_sizes( _sizes ) {
				static_assert( DIMS > 0, "DIMS should be higher than 0" );
				for( const auto i : _sizes ) {
					if( i == static_cast< row_coordinate_type >( 0U ) ) {
						throw std::invalid_argument( "All dimension sizes must "
													 "be > 0" );
					}
				}
				row_to_coords( first_row );
			}

			row_generator( const row_generator & o ) = default;

			row_generator( row_generator && o ) = default;

		protected:
			// x: row_coords[0], y: row_coords[1], z: row_coords[2], ...
			array_t row_coords; ///< n-D coordinates from which to compute the row

			/**
			 * @brief converts a row number into a n-D coordinates according to the sizes in #physical_sizes
			 *
			 * In case the input is higher than the nunber of rows, the last coordinate is allowed to
			 * go beyond its physical size. E.g., if the system has size (4,3,2) and \p rowcol is 24,
			 * the coordinates are (0,0,3).
			 *
			 * @param[in] rowcol row number to convert; it can be any number
			 */
			void row_to_coords( row_coordinate_type rowcol ) {
				std::size_t s = 1;
				for( std::size_t i { 0 }; i < row_coords.size() - 1; i++ )
					s *= physical_sizes[ i ];

				for( typename array_t::size_type i { row_coords.size() - 1 }; i > 0; i-- ) {
					row_coords[ i ] = rowcol / s;
					rowcol -= row_coords[ i ] * s;
					s /= physical_sizes[ i ];
				}
				row_coords[ 0 ] = rowcol % physical_sizes[ 0 ];
			}

			/**
			 * @brief Pure function converting an array of coordinates into a row number, based on #physical_sizes.
			 * @param a the #array_t array of coordinates to convert
			 * @return #row_coordinate_type the row corresponding to the coordinates in \p a
			 */
			row_coordinate_type coords_to_rowcol( const array_t & a ) const {
				row_coordinate_type row { 0 };
				row_coordinate_type s { 1 };
				for( typename array_t::size_type i { 0 }; i < a.size(); i++ ) {
					row += s * a[ i ];
					s *= physical_sizes[ i ];
				}
				return row;
			}

			/**
			 * @brief Increment #row_coords in order to move to the next coordinate (according to the
			 * n-dimensional iteration order) and update #current_row accordingly.
			 *
			 * To be used by derived classes in order to generate the matrix, e.g. via the \c operator()++
			 * operator prescribed for STL-like iterators.
			 */
			void increment_row() {
				bool rewind;
				typename array_t::size_type i { 0 };
				do {
					typename array_t::value_type & coord = row_coords[ i ];
					// must rewind dimension if we wrap-around
					typename array_t::value_type new_coord = ( coord + 1 ) % physical_sizes[ i ];
					rewind = new_coord < coord;
					coord = new_coord;
					++i;
				} while( rewind && i < row_coords.size() - 1 ); // rewind only the first N-1 coordinates

				// if we still have to rewind, increment the last coordinate, which is unbounded
				if( rewind ) {
					row_coords.back()++;
				}
			}
		};

		// ===============================================================

		/**
		 * @brief STL-like iterable class to generate the values for a matrix by iterating in an n-dimensional
		 * space along the coordinates.
		 *
		 * For each \f$ X=(x0, x1, ...,xn) \f$ point of the underlying (n+1)-dimensional space,
		 * this class iterates through the points of the n-dimensional halo of radius \p halo around \f$ X \f$,
		 * generating the row number corresponding to \f$ X \f$ and the column number corresponding to
		 * each halo point. At each coordinate \code (row, col) \endcode generated this way, the corresponding matrix value
		 * being generated depends on whether \code row == col \endcode.
		 *
		 * @tparam DIMS number of dimensions of the system
		 * @tparam HALO halo size, determining the number of points to iterate around and thus the column coordinates
		 * @tparam T type of matrix values
		 */
		template< std::size_t DIMS, typename T = double >
		struct matrix_generator_iterator : public row_generator< DIMS > {

			using row_coordinate_type = typename row_generator< DIMS >::row_coordinate_type;
			using column_coordinate_type = typename row_generator< DIMS >::row_coordinate_type;
			using nonzero_value_type = T;
			using array_t = typename row_generator< DIMS >::array_t;
			using value_type = std::pair< std::pair< row_coordinate_type, column_coordinate_type >, T >;

			// halo may in future become a DIM-size array to iterate in arbitrary shapes
			const row_coordinate_type halo;              ///< number of points per dimension to iterate around
			const nonzero_value_type diagonal_value;     ///< value to be emitted when the object has moved to the diagonal
			const nonzero_value_type non_diagonal_value; ///< value to emit outside of the diagonal

			/**
			 * @brief Construct a new \c matrix_generator_iterator object, setting the current row as \p row
			 * and emitting \p diag if the iterator has moved on the diagonal, \p non_diag otherwise.
			 *
			 * @param sizes array with the sizes along the dimensions
			 * @param row current row to initialize the matrix on
			 * @param _halo halo of points to iterate around; must be > 0
			 * @param diag value to emit when on the diagonal
			 * @param non_diag value to emit outside the diagonal
			 */
			matrix_generator_iterator( const array_t & sizes, row_coordinate_type row, row_coordinate_type _halo, nonzero_value_type diag, nonzero_value_type non_diag ) :
				row_generator< DIMS >( sizes, row ), halo( _halo ), diagonal_value( diag ), non_diagonal_value( non_diag ) {
				if( halo <= 0 ) {
					throw std::invalid_argument( "halo should be higher than "
												 "0" );
				}
				for( const auto i : sizes ) {
					if( i < static_cast< row_coordinate_type >( 2 * halo + 1 ) ) {
						throw std::invalid_argument( "Iteration halo goes "
													 "beyond system sizes" );
					}
				}
				current_values.first.first = row;
				update_column_max_values();
				reset_all_columns();
				current_values.first.second = this->coords_to_rowcol( col_coords );
				current_values.second = v();
			}

			matrix_generator_iterator( const matrix_generator_iterator & o ) = default;

			matrix_generator_iterator( matrix_generator_iterator && o ) = default;

			/**
			 * @brief Increments the iterator by moving coordinates to the next (row, column) to iterate on.
			 *
			 * This operator internally increments the columns coordinates until wrap-around, when it increments
			 * the row coordinates and resets the column coordinates to the first possible columns; this column coordinate
			 * depends on the row coordinates according to the dimensions iteration order and on the parameter \p halo.
			 *
			 * @return matrix_generator_iterator<DIMS, T>& \c this object, with the updated state
			 */
			matrix_generator_iterator< DIMS, T > & operator++() {
				bool must_rewind = increment_column();
				if( must_rewind ) {
					this->increment_row();
					// after changing row, we must find the first non-zero column
					reset_all_columns();
					current_values.first.first = this->coords_to_rowcol( this->row_coords );
					update_column_max_values();
				}
				// trigger column update after row update, as a row update
				// triggers a column update
				current_values.first.second = this->coords_to_rowcol( col_coords );
				current_values.second = this->v();
				return *this;
			}

			/**
			 * @brief Operator to compare \c this against \p o  and return whether they differ.
			 *
			 * @param o object to compare \c this against
			 * @return true of the row or the column is different between \p o and \c this
			 * @return false if both row and column of \p o and \c this are equal
			 */
			bool operator!=( const matrix_generator_iterator< DIMS, T > & o ) const {
				if( o.i() != this->i() ) {
					return true;
				}
				return o.j() != this->j();
			}

			/**
			 * @brief Operator to compare \c this against \p o  and return whether they are equal.
			 *
			 * @param o object to compare \c this against
			 * @return true of the row or the column is different between \p o and \c this
			 * @return false if both row and column of \p o and \c this are equal
			 */
			bool operator==( const matrix_generator_iterator< DIMS, T > & o ) const {
				return o.i() == this->i() && o.j() == this->j();
			}

			/**
			 * @brief Operator returning the triple to directly access row, column and element values.
			 *
			 * Useful when building the matrix by copying the triple of coordinates and value,
			 * like for the BSP1D backend.
			 */
			const value_type & operator*() const {
				return current_values;
			}

			/**
			 * @brief Returns current row.
			 */
			inline row_coordinate_type i() const {
				return current_values.first.first;
			}

			/**
			 * @brief Returns current column.
			 */
			inline column_coordinate_type j() const {
				return current_values.first.second;
			}

			/**
			 * @brief Returns the current matrix value.
			 *
			 * @return nonzero_value_type #diagonal_value if \code row == column \endcode (i.e. if \code this-> \endcode
			 * #i() \code == \endcode \code this-> \endcode #j()), #non_diagonal_value otherwise
			 */
			inline nonzero_value_type v() const {
				return j() == i() ? diagonal_value : non_diagonal_value;
			}

		private:
			// offsets w.r.t. rows
			array_t col_coords;        ///< coordinates corresponding to current column
			array_t column_max_values; ///< maximum values for the column coordinates, to stop column increment
			//// and reset the column coordinates
			value_type current_values; ///< triple storing the current value for row, column and matrix element

			/**
			 * @brief Updates the maximum values each column coordinate can reach, according to the row coordinates.
			 *
			 * To be called after each row coordinates update.
			 */
			void update_column_max_values() {
				for( std::size_t i { 0 }; i < column_max_values.size(); i++ ) {
					column_max_values[ i ] = std::min( this->physical_sizes[ i ] - 1, this->row_coords[ i ] + halo );
				}
			}

			/**
			 * @brief Resets the value of column dimension \p dim to the first possible value.
			 *
			 * The final value of #col_coords[dim] depends on the current row (#row_coords) and on the \p halo
			 * and is \f$ max(0, \f$ #row_coords \f$[dim])\f$.
			 *
			 * @param dim the dimension to reset
			 */
			void reset_column_coords( std::size_t dim ) {
				// cannot use std::max because row_coords is unsigned and can wrap-around
				col_coords[ dim ] = this->row_coords[ dim ] <= halo ? 0 : ( this->row_coords[ dim ] - halo );
			}

			/**
			 * @brief resets all values in #col_coords to the initial coordinates,
			 * iterating from on the current row.
			 */
			void reset_all_columns() {
				for( std::size_t i { 0 }; i < col_coords.size(); i++ ) {
					reset_column_coords( i );
				}
			}

			/**
			 * @brief Increment the column according to the iteration order, thus resetting the column coordinates
			 * when the last possible column value for the current row has been reached.
			 *
			 * @return true if the column coordinates have been reset, and thus also the row must be incremented
			 * @return false if the column coordinates
			 */
			bool increment_column() {
				bool rewind;
				typename array_t::size_type i { 0 };
				do {
					typename array_t::value_type & col = col_coords[ i ];
					// must rewind dimension if the column offset is already at the max value
					// or if the column coordinates are already at the max value
					rewind = ( col == column_max_values[ i ] );
					if( rewind ) {
						// col = this->row_coords[i] == 0 ? 0 : this->row_coords[i] - (halo);
						reset_column_coords( i );
					} else {
						++col;
					}
					++i;
				} while( rewind && i < col_coords.size() );

				// if we change z, then we also must reset x and y; if only y, we must reset x, and so on
				return rewind;
			}
		};

		// ===============================================================

		/**
		 * @brief Class to generate the coarsening matrix of an underlying \p DIMS -dimensional system.
		 *
		 * This class coarsens a finer system to a coarser system by projecting each input value (column),
		 * espressed in finer coordinates, to an output (row) value espressed in coarser coordinates.
		 * The coarser sizes are assumed to be row_generator#physical_sizes, while the finer sizes are here
		 * stored inside #finer_sizes.
		 *
		 * The corresponding refinement matrix is obtained by transposing the coarsening matrix.
		 *
		 * @tparam DIMS number of dimensions of the system
		 * @tparam T type of matrix values
		 */
		template< std::size_t DIMS, typename T = double >
		struct coarsener_generator_iterator : public row_generator< DIMS > {

			using row_coordinate_type = typename row_generator< DIMS >::row_coordinate_type;
			using column_coordinate_type = typename row_generator< DIMS >::row_coordinate_type;
			using nonzero_value_type = T;
			using array_t = typename row_generator< DIMS >::array_t;
			using value_type = std::pair< std::pair< row_coordinate_type, column_coordinate_type >, T >;

			// the sizes to project from
			const array_t finer_sizes; ///< the size of the finer system (columns)
			array_t steps;             ///< array of steps, i.e. how much each column coordinate (finer system) must be
			//// incremented when incrementing the row coordinates; is is the ration between
			//// #finer_sizes and row_generator#physical_sizes

			/**
			 * @brief Construct a new \c coarsener_generator_iterator object from the coarser and finer sizes,
			 * setting its row at \p _current_row and the column at the corresponding value.
			 *
			 * Each finer size <b>must be an exact multiple of the corresponding coarser size</b>, otherwise the
			 * construction will throw an exception.
			 *
			 * @param _coarser_sizes sizes of the coarser system (rows)
			 * @param _finer_sizes sizes of the finer system (columns)
			 * @param _current_row row (in the coarser system) to set the iterator on
			 */
			coarsener_generator_iterator( const array_t & _coarser_sizes, const array_t & _finer_sizes, row_coordinate_type _current_row ) :
				row_generator< DIMS >( _coarser_sizes, _current_row ), finer_sizes( _finer_sizes ), steps( { 0 } ) {
				for( std::size_t i { 0 }; i < DIMS; i++ ) {
					// finer size MUST be an exact multiple of coarser_size
					typename array_t::value_type step { _finer_sizes[ i ] / _coarser_sizes[ i ] };
					if( step == 0 || finer_sizes[ i ] / step != this->physical_sizes[ i ] ) {
						throw std::invalid_argument( std::string( "finer size "
																  "of "
																  "dimension"
																  " " ) +
							std::to_string( i ) +
							std::string( "is not an exact multiple of coarser "
										 "size" ) );
					}
					steps[ i ] = step;
				}
				current_values.first.first = _current_row;
				current_values.first.second = coords_to_finer_col();
				current_values.second = v();
			}

			coarsener_generator_iterator( const coarsener_generator_iterator & o ) = default;

			coarsener_generator_iterator( coarsener_generator_iterator && o ) = default;

			/**
			 * @brief Increments the row and the column according to the respective physical sizes,
			 * thus iterating onto the coarsening matrix coordinates.
			 *
			 * @return \code *this \endcode, i.e. the same object with the updates row and column
			 */
			coarsener_generator_iterator< DIMS, T > & operator++() {
				this->increment_row();
				current_values.first.first = this->coords_to_rowcol( this->row_coords );
				current_values.first.second = coords_to_finer_col();
				current_values.second = v();
				return *this;
			}

			/**
			 * @brief Returns whether \c this and \p o differ.
			 */
			bool operator!=( const coarsener_generator_iterator< DIMS, T > & o ) const {
				if( this->i() != o.i() ) {
					return true;
				}
				return this->j() != o.j();
			}

			/**
			 * @brief Returns whether \c this and \p o are equal.
			 */
			bool operator==( const coarsener_generator_iterator< DIMS, T > & o ) const {
				return this->i() == o.i() && this->j() == o.j();
			}

			/**
			 * @brief Operator returning the triple to directly access row, column and element values.
			 *
			 * Useful when building the matrix by copying the triple of coordinates and value,
			 * like for the BSP1D backend.
			 */
			const value_type & operator*() const {
				return current_values;
			}

			/**
			 * @brief Returns the current row, according to the coarser system.
			 */
			inline row_coordinate_type i() const {
				return current_values.first.first;
			}

			/**
			 * @brief Returns the current column, according to the finer system.
			 */
			inline column_coordinate_type j() const {
				return current_values.first.second;
			}

			/**
			 * @brief Returns always 1, as the coarsening keeps the same value.
			 */
			inline nonzero_value_type v() const {
				return static_cast< nonzero_value_type >( 1 );
			}

		private:
			value_type current_values; ///< triple storing the current value for row, column and matrix element

			/**
			 * @brief Returns the row coordinates converted to the finer system, to compute
			 * the column value.
			 */
			column_coordinate_type coords_to_finer_col() const {
				column_coordinate_type row { 0 };
				column_coordinate_type s { 1 };
				for( typename array_t::size_type i { 0 }; i < this->row_coords.size(); i++ ) {
					s *= steps[ i ];
					row += s * this->row_coords[ i ];
					s *= this->physical_sizes[ i ];
				}
				return row;
			}
		};

	} // namespace utils
} // namespace grb

#endif // _H_NDIM_MATRIX_BUILDERS
