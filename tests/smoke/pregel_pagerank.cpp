
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

#include <graphblas/algorithms/pregel_pagerank.hpp>
#include <graphblas/utils/Timer.hpp>
#include <graphblas/utils/parser.hpp>

#include <graphblas.hpp>
#include <utils/output_verification.hpp>

#ifndef PR_CONVERGENCE_MODE
 #error "PR_CONVERGENCE_MODE must be defined and read either true or false"
#endif


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
	//double residual;
	grb::utils::TimerResults times;
	PinnedVector< double > pinnedVector;
};

void grbProgram( const struct input &data_in, struct output &out ) {

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
	grb::utils::MatrixFileReader< void,
		std::conditional<
			(sizeof(grb::config::RowIndexType) > sizeof(grb::config::ColIndexType)),
			grb::config::RowIndexType,
			grb::config::ColIndexType
		>::type
	> parser( data_in.filename, data_in.direct );
	assert( parser.m() == parser.n() );
	const size_t n = parser.n();
	out.times.io = timer.time();
	timer.reset();

        // prepare Pregel interface
        grb::interfaces::Pregel< void > pregel(
		parser.n(), parser.m(),
		parser.begin(), parser.end(),
		SEQUENTIAL
	);

        // prepare for launching pagerank algorithm
	// 1. initalise pagerank scores and message buffers
        grb::Vector< double > pr( n ), in_msgs( n ), out_msgs( n );
	grb::Vector < double > out_buffer = interfaces::config::out_sparsify
		? grb::Vector< double >( n )
		: grb::Vector< double >( 0 );
	// 2. initialise PageRank parameters
	grb::algorithms::pregel::PageRank< double, PR_CONVERGENCE_MODE >::Data pr_data;
	// 3. get handle to Pregel PageRank program
	auto &pr_prog = grb::algorithms::pregel::PageRank<
			double, PR_CONVERGENCE_MODE
		>::program;

	out.times.preamble = timer.time();

	// by default, copy input requested repetitions to output repititions performed
	out.rep = data_in.rep;

	// time a single call
	RC rc = set( pr, 0 );
	if( out.rep == 0 ) {
		timer.reset();
	        rc = pregel.template execute<
				grb::operators::add< double >,
				grb::identities::zero
			> (
				pr_prog, pr, pr_data,
				in_msgs, out_msgs,
				out.iterations,
				out_buffer
		        );
		double single_time = timer.time();
		if( rc != SUCCESS ) {
			std::cerr << "Failure: call to pregel_pagerank did not succeed "
				<< "(" << toString( rc ) << ")." << std::endl;
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
				std::cout << "Info: cold pagerank completed within "
					<< out.iterations << " iterations. "
					//<< "Last computed residual is " << out.residual << ". "
					<< "Time taken was " << single_time
					<< " ms. Deduced inner repetitions parameter of " << out.rep
					<< " to take 1 second or more per inner benchmark.\n";
			}
		}
	} else {
		// do benchmark
		double time_taken;
		timer.reset();
		for( size_t i = 0; i < out.rep && rc == SUCCESS; ++i ) {
			rc = grb::set( pr, 0 );
			if( rc == SUCCESS ) {
				rc = algorithms::pregel::PageRank< double, PR_CONVERGENCE_MODE >::execute(
						pregel, pr, out.iterations, pr_data
					);
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
			std::cout << "Time taken for a " << out.rep
				<< " PageRank calls (hot start): " << out.times.useful << ". "
				<< "Error code is " << out.error_code << std::endl;
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
	if( argc < 3 || argc > 7 ) {
		std::cout << "Usage: " << argv[ 0 ] << " "
			<< "<dataset> <direct/indirect> "
			<< "(inner iterations) (outer iterations) (verification <truth-file>)\n";
		std::cout << "<dataset> and <direct/indirect> are mandatory arguments.\n";
		std::cout << "(inner iterations) is optional, the default is "
			<< grb::config::BENCHMARKING::inner() << ". "
			<< "If set to zero, the program will select a number of iterations "
			<< "approximately required to take at least one second to complete.\n";
		std::cout << "(outer iterations) is optional, the default is "
			<< grb::config::BENCHMARKING::outer()
			<< ". This value must be strictly larger than 0.\n";
		std::cout << "(verification <truth-file>) is optional. "
			<< "The <truth-file> must point to a pre-computed solution that the "
			<< "computed solution will be verified against." << std::endl;
		return 0;
	}
	std::cout << "Test executable: " << argv[ 0 ] << std::endl;

	// the input struct
	struct input in;

	// get file name
	(void) strncpy( in.filename, argv[ 1 ], 1023 );
	in.filename[ 1023 ] = '\0';

	// get direct or indirect addressing
	if( strncmp( argv[ 2 ], "direct", 6 ) == 0 ) {
		in.direct = true;
	} else {
		in.direct = false;
	}

	// get inner number of iterations
	in.rep = grb::config::BENCHMARKING::inner();
	char * end = nullptr;
	if( argc >= 4 ) {
		in.rep = strtoumax( argv[ 3 ], &end, 10 );
		if( argv[ 3 ] == end ) {
			std::cerr << "Could not parse argument " << argv[ 2 ]
				<< " for number of inner experiment repititions." << std::endl;
			return 2;
		}
	}

	// get outer number of iterations
	size_t outer = grb::config::BENCHMARKING::outer();
	if( argc >= 5 ) {
		outer = strtoumax( argv[ 4 ], &end, 10 );
		if( argv[ 4 ] == end ) {
			std::cerr << "Could not parse argument " << argv[ 3 ]
				<< " for number of outer experiment repititions." << std::endl;
			return 4;
		}
	}

	// check for verification of the output
	bool verification = false;
	char truth_filename[ 1024 ];
	if( argc >= 6 ) {
		if( strncmp( argv[ 5 ], "verification", 12 ) == 0 ) {
			verification = true;
			if( argc >= 7 ) {
				(void) strncpy( truth_filename, argv[ 6 ], 1023 );
				truth_filename[ 1023 ] = '\0';
			} else {
				std::cerr << "The verification file was not provided as an argument."
					<< std::endl;
				return 5;
			}
		} else {
			std::cerr << "Could not parse argument \"" << argv[ 5 ] << "\", "
				<< "the optional \"verification\" argument was expected." << std::endl;
			return 5;
		}
	}

	std::cout << "Executable called with parameters " << in.filename << ", "
		<< "inner repititions = " << in.rep << ", "
		<< "and outer reptitions = " << outer << std::endl;

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
			std::cerr << "launcher.exec returns with non-SUCCESS error code "
				<< grb::toString( rc ) << std::endl;
			return 6;
		}
	}

	// launch benchmark
	if( rc == SUCCESS ) {
		grb::Benchmarker< AUTOMATIC > benchmarker;
		rc = benchmarker.exec( &grbProgram, in, out, 1, outer, true );
	}
	if( rc != SUCCESS ) {
		std::cerr << "benchmarker.exec returns with non-SUCCESS error code "
			<< grb::toString( rc ) << std::endl;
		return 8;
	} else if( out.error_code == 0 ) {
		std::cout << "Benchmark completed successfully and took "
			<< out.iterations << " iterations to converge.\n";
			//<< "with residual " << out.residual << ".\n";
	}

	const size_t n = out.pinnedVector.size();
	std::cout << "Error code is " << out.error_code << ".\n";
	std::cout << "Size of pr is " << n << ".\n";
	if( out.error_code == 0 && n > 0 ) {
		std::cout << "First 10 nonzeroes of pr are: (\n";
		for( size_t k = 0; k < out.pinnedVector.nonzeroes() && k < 10; ++k ) {
			const auto &index = out.pinnedVector.getNonzeroIndex( k );
			const auto &value = out.pinnedVector.getNonzeroValue( k );
			std::cout << "\t " << index << ", " << value << "\n";
		}
		std::cout << ")" << std::endl;
	}

	if( out.error_code != 0 ) {
		std::cerr << std::flush;
		std::cout << "Test FAILED\n";
	} else {
		if( verification ) {
			out.error_code = vector_verification(
				out.pinnedVector, truth_filename,
				1e-5, 1e-6
			);
			if( out.error_code == 0 ) {
				std::cout << "Verification OK\n";
				std::cout << "Test OK\n";
			} else {
				std::cerr << std::flush;
				std::cout << "Verification FAILED\n";
				std::cout << "Test FAILED\n";
			}
		} else {
			std::cout << "Test OK\n";
		}
	}
	std::cout << std::endl;

	// done
	return out.error_code;
}

