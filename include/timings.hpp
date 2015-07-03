#pragma once

#include <sdsl/suffix_arrays.hpp>
#include <sdsl/suffix_trees.hpp>
#include <sdsl/suffix_array_algorithm.hpp>

#include <array>
#include <chrono>

using namespace std::chrono;
using watch = std::chrono::high_resolution_clock;

struct lm_construct_timer {
	watch::time_point start;
	std::string name;
	lm_construct_timer(const std::string& _n) : name(_n) {
		LOG(INFO) << "START_CONSTRUCT(" << name << ")";
		start = watch::now();
	}
	~lm_construct_timer() {
		auto stop = watch::now();
		LOG(INFO) << "STOP_CONSTRUCT(" << name << ") - " << duration_cast<milliseconds>(stop - start).count() / 1000.0f << " sec";
	}
};

enum class timer_type {
	N1PlusBack = 0,
	N123PlusBack,
	N1PlusFront,
	N123PlusFront,
	N1PlusFrontBack,
	N123PlusFrontBack,
	forward_search ,
	backward_search,
	highestorder,
	lowerorder,
	lowestorder
};
const uint64_t num_timer_types = 8;

std::string
timer_type_to_str(int type) {
	timer_type t = static_cast<timer_type>(type);
    switch(t) {
         case timer_type::N1PlusBack      : return "N1PlusBack";
	 case timer_type::N123PlusBack      : return "N123PlusBack";
         case timer_type::N1PlusFront     : return "N1PlusFront";
	 case timer_type::N123PlusFront     : return "N123PlusFront";
         case timer_type::N1PlusFrontBack : return "N1PlusFrontBack";
	 case timer_type::N123PlusFrontBack : return "N123PlusFrontBack";
         case timer_type::forward_search  : return "forward_search";
         case timer_type::backward_search : return "backward_search";
         case timer_type::highestorder    : return "highestorder";
         case timer_type::lowerorder      : return "lowerorder";
         case timer_type::lowestorder     : return "lowestorder";
    }
    return "SHOULD NEVER HAPPEN";
}

struct bench_data {
	std::array<uint64_t,num_timer_types> num_calls{{0}};
	std::array<nanoseconds,num_timer_types> total_time{{nanoseconds::zero()}};
};

struct lm_timer {
	timer_type type;
	watch::time_point start;
	bench_data& bd;
	lm_timer(timer_type t,bench_data& _bd) : type(t), bd(_bd) {
		start = watch::now();
	}
	~lm_timer() {
		auto stop = watch::now();
		auto time_spent = stop-start;
		/* update stats */
		bd.num_calls[static_cast<int>(type)]++;
		bd.total_time[static_cast<int>(type)] += time_spent;
	}
};

struct lm_bench {
private:
	static bench_data& data() {
		static bench_data bd;
		return bd;
	}
public:
	static lm_timer bench(timer_type t) {
		auto& bd = data();
		return lm_timer(t,bd);
	}
	static void reset() {
		auto& d = data();
		d = bench_data();
	}
	static void print() {
		auto& d = data();
		LOG(INFO) << "TIMINGS";
		for(size_t i=0;i<num_timer_types;i++) {
			LOG(INFO) 	  << std::setw(17) << timer_type_to_str(i) 
		    << " Calls="  << std::setw(10) << d.num_calls[i] 
			<< " Total="  << std::setw(10) << std::setprecision(6) << duration_cast<milliseconds>(d.total_time[i]).count() / 1000.0f << " sec"
			<< " Avg=" 	  << std::setw(10) << d.total_time[i].count() / (d.num_calls[i]==0 ? 1 : d.num_calls[i]) << " ns";
		}
	}
};

//factored out for timing
template<class t_cst>
bool
forward_search_wrapper(
    const t_cst& cst,
    typename t_cst::node_type& v,
    const typename t_cst::size_type d,
    const typename t_cst::char_type c,
    typename t_cst::size_type& char_pos
)
{
    auto timer = lm_bench::bench(timer_type::forward_search);
    auto ok = forward_search(cst, v, d, c, char_pos);
    return (ok != 0);
}

// factored out for timing
template<class t_csa>
bool
backward_search_wrapper(
    const t_csa& csa,
    typename t_csa::size_type l,
    typename t_csa::size_type r,
    typename t_csa::char_type c,
    typename t_csa::size_type& l_res,
    typename t_csa::size_type& r_res
)
{
    auto timer = lm_bench::bench(timer_type::backward_search);
    return backward_search(csa, l, r, c, l_res, r_res);
}

// convenience function
template<class t_cst>
bool
backward_search_wrapper(
    const t_cst& cst,
    typename t_cst::node_type& v,
    const typename t_cst::char_type c
)
{
    auto timer = lm_bench::bench(timer_type::backward_search);
    typename t_cst::size_type l = cst.lb(v), r = cst.rb(v);
    backward_search(cst.csa, l, r, c, l, r);
    if (r >= l) v = cst.node(l, r);
    return (r >= l);
}
