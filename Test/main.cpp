#include <iostream>
#include <vector>
#include <memory>
#include <array>
#include <random>
#include "Profiler.h"

// Store the objects contiguously in memory
#define CONTIGUOUS
// make each Num fill an entire cache line
#define ALIGN_TO_CACHE

#if defined ALIGN_TO_CACHE
struct alignas ( 64 ) Num
#else
struct Num
#endif
{
	Num ( ) = default;
	constexpr Num ( const int num ) noexcept : num { num }
	{
	}

	int num { };
};

int main ( )
{
	Profiler profiler;
	constexpr size_t total_objects = 100'000;
	constexpr size_t total_lookups = 100'000;
	constexpr size_t total_tests = 1000;

	//std::random_device seeder;
	std::mt19937 rng_machine { /*seeder ( )*/ };
	std::uniform_int_distribution<int> value_rng;
	std::uniform_int_distribution<size_t> rng { 0, total_objects - 1 };


#if defined CONTIGUOUS
	std::vector<Num> objects;
	objects.resize ( total_objects );
	for ( auto& object : objects )
	{
		object = { value_rng ( rng_machine ) };
	}
#else
	std::vector<std::unique_ptr<Num>> objects;
	objects.reserve ( total_objects );
	for ( size_t index = 0u; index < total_objects; ++index )
	{
		objects.push_back ( std::make_unique<Num> ( value_rng ( rng_machine ) ) );
	}
#endif


	std::vector<Num*> lookups;
	lookups.resize ( total_lookups );
	for ( auto& lookup : lookups )
	{
		auto index = rng ( rng_machine );
#if defined CONTIGUOUS
		lookup = &objects [ index ];
#else
		lookup = objects [ index ].get ( );
#endif
	}

	std::vector<unsigned long long> totals;
	totals.resize ( total_tests );

	for ( size_t i = 0; i < total_tests; ++i )
	{
		profiler.Start ( );
		unsigned long long total = 0;
		for ( auto lookup : lookups )
		{
			const auto object = *lookup;
			total += object.num;
		}
		profiler.End ( );

		totals [ i ] = total;
	}

	auto profile = profiler.Flush ( );
	std::cout << "mean: " << profile.mean << '\n';
	std::cout << "lowest: " << profile.lowest << '\n';
	std::cout << "highest: " << profile.highest << '\n';
	std::cout << "median: " << profile.median << '\n';
	std::cout << "standard_deviation: " << profile.standard_deviation << '\n';

	system ( "pause" );
	return 0;
}