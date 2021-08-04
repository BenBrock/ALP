
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

#include <exception>
#include <iostream>
#include <vector>

#include <inttypes.h>

#include <graphblas/algorithms/simple_pagerank.hpp>
#include <graphblas/utils/Timer.hpp>
#include <graphblas/utils/parser.hpp>

#include <graphblas.hpp>

using namespace grb;
using namespace algorithms;

struct input {
	char filename[ 1024 ];
	bool direct;
	size_t rep;
};

struct output {
	int error_code;
	size_t rep;
	size_t iterations;
	double residual;
	grb::utils::TimerResults times;
	PinnedVector< double > pinnedVector;
};

void grbProgram( const struct input & data_in, struct output & out ) {

	// get user process ID
	const size_t s = spmd<>::pid();
	assert( s < spmd<>::nprocs() );

	// get input n
	grb::utils::Timer timer;
	timer.reset();

	// sanity checks on input
	if( data_in.filename[ 0 ] == '\0' ) {
		std::cerr << s << ": no file name given as input." << std::endl;
		out.error_code = ILLEGAL;
		return;
	}

	// assume successful run
	out.error_code = 0;

	// create local parser
	grb::utils::MatrixFileReader< void, std::conditional< ( sizeof( grb::config::RowIndexType ) > sizeof( grb::config::ColIndexType ) ), grb::config::RowIndexType, grb::config::ColIndexType >::type >
		parser( data_in.filename, data_in.direct );
	assert( parser.m() == parser.n() );
	const size_t n = parser.n();
	out.times.io = timer.time();
	timer.reset();

	// load into GraphBLAS
	Matrix< void > L( n, n );
	{
		// const RC rc = buildMatrixUnique( L, parser.begin( SEQUENTIAL ), parser.end( SEQUENTIAL ), SEQUENTIAL );
		const RC rc = buildMatrixUnique( L, parser.begin( PARALLEL ), parser.end( PARALLEL ), PARALLEL );
		if( rc != SUCCESS ) {
			std::cerr << "Failure: call to buildMatrixUnique did not succeed (" << toString( rc ) << ")." << std::endl;
			out.error_code = 10;
			return;
		}
	}

	// check number of nonzeroes
	try {
		const size_t global_nnz = nnz( L );
		const size_t parser_nnz = parser.nz();
		if( global_nnz != parser_nnz ) {
			std::cerr << "Failure: global nnz (" << global_nnz << ") does not equal parser nnz (" << parser_nnz << ")." << std::endl;
			out.error_code = 15;
			return;
		}
	} catch( const std::runtime_error & ) {
		std::cout << "Info: nonzero check skipped as the number of nonzeroes "
					 "cannot be derived from the matrix file header. The "
					 "grb::Matrix reports "
				  << nnz( L ) << " nonzeroes.\n";
	}

	// test default pagerank run
	Vector< double > pr( n );
	Vector< double > buf1( n ), buf2( n ), buf3( n );
	out.times.preamble = timer.time();

	// by default, copy input requested repetitions to output repititions performed
	out.rep = data_in.rep;
	// time a single call
	RC rc = SUCCESS;
	if( out.rep == 0 ) {
		timer.reset();
		rc = simple_pagerank< descriptors::no_operation >( pr, L, buf1, buf2, buf3, 0.85, .00000001, 1000, &( out.iterations ), &( out.residual ) );
		double single_time = timer.time();
		if( rc != SUCCESS ) {
			std::cerr << "Failure: call to simple_pagerank did not succeed (" << toString( rc ) << ")." << std::endl;
			out.error_code = 20;
		}
		if( rc == SUCCESS ) {
			rc = collectives<>::reduce( single_time, 0, operators::max< double >() );
		}
		if( rc != SUCCESS ) {
			out.error_code = 25;
		}
		out.times.useful = single_time;
		out.rep = static_cast< size_t >( 1000.0 / single_time ) + 1;
		if( rc == SUCCESS ) {
			if( s == 0 ) {
				std::cout << "Info: cold pagerank completed within " << out.iterations << " iterations. Last computed residual is " << out.residual << ". Time taken was " << single_time
						  << " ms. Deduced inner repetitions parameter of " << out.rep << " to take 1 second or more per inner benchmark.\n";
			}
		}
	} else {
		// do benchmark
		double time_taken;
		timer.reset();
		for( size_t i = 0; i < out.rep && rc == SUCCESS; ++i ) {
			rc = grb::clear( pr );
			if( rc == SUCCESS ) {
				rc = simple_pagerank< descriptors::no_operation >( pr, L, buf1, buf2, buf3, 0.85, .00000001, 1000, &( out.iterations ), &( out.residual ) );
			}
		}
		time_taken = timer.time();
		if( rc == SUCCESS ) {
			out.times.useful = time_taken / static_cast< double >( out.rep );
		}
		sleep( 1 );
#ifndef NDEBUG
		// print timing at root process
		if( grb::spmd<>::pid() == 0 ) {
			std::cout << "Time taken for a " << out.rep << " PageRank calls (hot start): " << out.times.useful << ". Error code is " << out.error_code << std::endl;
		}
#endif
	}

	// start postamble
	timer.reset();

	// set error code
	if( rc == FAILED ) {
		out.error_code = 30;
		// no convergence, but will print output
	} else if( rc != SUCCESS ) {
		std::cerr << "Benchmark run returned error: " << toString( rc ) << "\n";
		out.error_code = 35;
		return;
	}

	// output
	out.pinnedVector = PinnedVector< double >( pr, SEQUENTIAL );

	// finish timing
	const double time_taken = timer.time();
	out.times.postamble = time_taken;

	// done
	return;
}

int main( int argc, char ** argv ) {
	// sanity check
	if( argc < 3 || argc > 5 ) {
		std::cout << "Usage: " << argv[ 0 ]
				  << " <dataset> <direct/indirect> (inner iterations) (outer "
					 "iterations)\n";
		std::cout << "<dataset> and <direct/indirect> are mandatory "
					 "arguments.\n";
		std::cout << "(inner iterations) is optional, the default is " << grb::config::BENCHMARKING::inner()
				  << ". If set to zero, the program will select a number of "
					 "iterations approximately required to take at least one "
					 "second to complete.\n";
		std::cout << "(outer iterations) is optional, the default is " << grb::config::BENCHMARKING::outer() << ". This value must be strictly larger than 0." << std::endl;
		return 0;
	}
	std::cout << "Test executable: " << argv[ 0 ] << std::endl;

	// the input struct
	struct input in;

	// get file name
	(void)strncpy( in.filename, argv[ 1 ], 1023 );
	in.filename[ 1023 ] = '\0';

	// get direct or indirect addressing
	if( strncmp( argv[ 2 ], "direct", 6 ) == 0 ) {
		in.direct = true;
	} else {
		in.direct = false;
	}

	// get inner number of iterations
	in.rep = grb::config::BENCHMARKING::inner();
	char * end = NULL;
	if( argc >= 4 ) {
		in.rep = strtoumax( argv[ 3 ], &end, 10 );
		if( argv[ 3 ] == end ) {
			std::cerr << "Could not parse argument " << argv[ 2 ] << " for number of inner experiment repititions." << std::endl;
			return 2;
		}
	}

	// get outer number of iterations
	size_t outer = grb::config::BENCHMARKING::outer();
	if( argc >= 5 ) {
		outer = strtoumax( argv[ 4 ], &end, 10 );
		if( argv[ 4 ] == end ) {
			std::cerr << "Could not parse argument " << argv[ 3 ] << " for number of outer experiment repititions." << std::endl;
			return 4;
		}
	}

	std::cout << "Executable called with parameters " << in.filename << ", inner repititions = " << in.rep << ", and outer reptitions = " << outer << std::endl;

	// the output struct
	struct output out;

	// set standard exit code
	grb::RC rc = SUCCESS;

	// launch estimator (if requested)
	if( in.rep == 0 ) {
		grb::Launcher< AUTOMATIC > launcher;
		rc = launcher.exec( &grbProgram, in, out, true );
		if( rc == SUCCESS ) {
			in.rep = out.rep;
		}
		if( rc != SUCCESS ) {
			std::cerr << "launcher.exec returns with non-SUCCESS error code " << (int)rc << std::endl;
			return 6;
		}
	}

	// launch benchmark
	if( rc == SUCCESS ) {
		grb::Benchmarker< AUTOMATIC > benchmarker;
		rc = benchmarker.exec( &grbProgram, in, out, 1, outer, true );
	}
	if( rc != SUCCESS ) {
		std::cerr << "benchmarker.exec returns with non-SUCCESS error code " << grb::toString( rc ) << std::endl;
		return 8;
	} else if( out.error_code == 0 ) {
		std::cout << "Benchmark completed successfully and took " << out.iterations << " iterations to converge with residual " << out.residual << ".\n";
	}

	std::cout << "Error code is " << out.error_code << ".\n";
	std::cout << "Size of pr is " << out.pinnedVector.length() << ".\n";
	if( out.error_code == 0 && out.pinnedVector.length() > 0 ) {
		std::cout << "First 10 elements of pr are: ( ";
		if( out.pinnedVector.mask( 0 ) ) {
			std::cout << out.pinnedVector[ 0 ];
		} else {
			std::cout << "0";
		}
		for( size_t i = 1; i < out.pinnedVector.length() && i < 10; ++i ) {
			std::cout << ", ";
			if( out.pinnedVector.mask( i ) ) {
				std::cout << out.pinnedVector[ i ];
			} else {
				std::cout << "0";
			}
		}
		std::cout << " )" << std::endl;
		std::cout << "First 10 nonzeroes of pr are: ( ";
		size_t nnzs = 0;
		size_t i = 0;
		while( i < out.pinnedVector.length() && nnzs == 0 ) {
			if( out.pinnedVector.mask( i ) ) {
				std::cout << out.pinnedVector[ i ];
				++nnzs;
			}
			++i;
		}
		while( nnzs < 10 && i < out.pinnedVector.length() ) {
			if( out.pinnedVector.mask( i ) ) {
				std::cout << ", " << out.pinnedVector[ i ];
				++nnzs;
			}
			++i;
		}
		std::cout << " )" << std::endl;
	}

	if( out.error_code != 0 ) {
		std::cout << "Test FAILED.\n";
	} else {
		std::cout << "Test OK.\n";
	}
	std::cout << std::endl;

	// done
	return 0;
}