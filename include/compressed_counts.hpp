#pragma once

#include "sdsl/vectors.hpp"

#include "constants.hpp"
#include "collection.hpp"

struct compressed_counts {
    typedef sdsl::int_vector<>::size_type size_type;
    typedef sdsl::rrr_vector<63> bv_type;
    typedef sdsl::vlc_vector<> vector_type;

private:
    bv_type m_bv;
    bv_type::rank_1_type m_bv_rank;
    vector_type m_counts;

public:

    compressed_counts() = default;

    template<class t_cst,class t_node_type>
    uint32_t compute_contexts(t_cst& cst,t_node_type node)
    {
	    static std::vector<typename t_cst::csa_type::value_type> preceding_syms(12312312);
	    static std::vector<typename t_cst::csa_type::size_type> left(12312312);
	    static std::vector<typename t_cst::csa_type::size_type> right(12312312);
	    auto lb = cst.lb(node);
	    auto rb = cst.rb(node);
	    typename t_cst::csa_type::size_type num_syms = 0;
	    sdsl::interval_symbols(cst.csa.wavelet_tree, lb, rb + 1, num_syms, preceding_syms, left, right);
	    if (num_syms == 1)
	        return cst.degree(node);
	    else {
	        auto total_contexts = 0;
	        for (size_t i = 0; i < num_syms; i++) {
	            auto new_lb = cst.csa.C[cst.csa.char2comp[preceding_syms[i]]] + left[i];
	            auto new_rb = cst.csa.C[cst.csa.char2comp[preceding_syms[i]]] + right[i] - 1;
	            if (new_lb == new_rb)
	                total_contexts++;
	            else {
	                auto new_node = cst.node(new_lb, new_rb);
	                auto deg = cst.degree(new_node);
	                total_contexts += deg;
	            }
	        }
	        return total_contexts;
	    }
    }

    template<class t_cst>
    compressed_counts(t_cst& cst,uint64_t max_node_depth) {
    	sdsl::bit_vector tmp_bv(cst.nodes());
    	std::vector<uint32_t> counts;

        auto root = cst.root();
        int skip = SKIP_SYMS; // skip 0 and 1, 2, 3, 4 subtrees
        for (const auto& child : cst.children(root)) {
            if (skip) { // skip
                skip--;
                continue;
            }
		    auto itr = cst.begin(child);
		    auto end = cst.end(child);
		    while (itr != end) {
		        if (itr.visit() == 1) {
		            auto node = *itr;
		            auto node_id = cst.id(node);
		            if (cst.is_leaf(node)) {
		            	tmp_bv[node_id] = 0;
		            } else {
		                auto depth = cst.depth(node);
		                if( depth > max_node_depth ) {
		                	tmp_bv[node_id] = 0;
		                	itr.skip_subtree();
		                } else {
		                	auto c = compute_contexts(cst, node);
		                	counts.push_back(c);
		                	tmp_bv[node_id] = 1;
		                }
		            }
		        }
		        ++itr;
		    }
        }

	    m_counts = vector_type(counts);
    	m_bv = bv_type(tmp_bv);
    }

    size_type serialize(std::ostream& out, sdsl::structure_tree_node* v = NULL, std::string name = "") const
    {
        sdsl::structure_tree_node* child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        size_type written_bytes = 0;
        written_bytes += sdsl::serialize(m_bv,out,child,"bv");
        written_bytes += sdsl::serialize(m_counts,out,child,"counts");
        sdsl::structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    template<class t_cst,class t_node_type>
    uint64_t lookup(t_cst& cst,t_node_type node) const {
    	auto id = cst.id(node);
    	if(m_bv[id]==0)
    		return 1;
    	else {
    		auto rank_in_vec = m_bv_rank(id);
    		return m_counts[rank_in_vec];
    	}
    }

    void load(std::istream& in)
    {
        sdsl::load(m_bv, in);
        m_bv_rank.load(in,&m_bv);
        sdsl::load(m_counts, in);
    }

};