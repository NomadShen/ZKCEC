#pragma once
#ifndef ZKUNSAT_NEW_UTILS_H
#define ZKUNSAT_NEW_UTILS_H

#include "commons.h"
#include "encoder.hpp"


using namespace  std;
using  namespace  NTL;


extern Encoder *encoder;

#define BAR_WIDTH 30


/* Encode literal: the highest bit indicates the sign of that literal;
 * the rest bits indicate the value of the index;
 * for example: x2 is encoded as [1,0, ..., 0, 1, 0] , and not x2 is encoded as [0,0, ..., 0, 1, 0];
 * [0,0, ..., 0, 0, 0] and [1,0, ..., 0, 0, 0] are dummy bits;
 * used by prover locally;
 */
const block one_block = makeBlock(0, 1);
inline GaloisFieldPacking gp;

uint64_t wrap(int64_t input);
inline int64_t unwrap(uint64_t input);

//get mac of val

void pack(block& mac, const Integer& val, int size = VAL_SZ);


/*
 * multiply a committed value x by cst; mac of x is m;
 * val = m * cst;
 * mac is the mac of val;
 */

void multiply_const(block &val, block &mac,
                           const block& x, const block& m, const block& cst, int party);

/*
 * compute addition of vala and valb;
 * valc = vala + valb; maca is mac of a, macb is mac of b;
 * macc = mac(valc)
 */
void compute_xor(block &valc, block &macc,
                        const block vala, const block maca, const block valb, const block macb);


__uint128_t get_128uint_from_uint64(uint64_t value);

void check_MAC_valid(block X, block MAC);

/*
 * check if a block is a mac of zero.
 * It is an accumulator that takes all mac blocks to be check
 * At the end of the proof, prover by setting end = 1 will hash all blocks, and send it to verifier.
*/

void check_zero_MAC(block MAC, int end = 0);




/*
 * padding a vector of int64 to the size of degree
 * used when prover input a clause
 */
void padding(vector<uint64_t>& input);

void padding(vector<uint64_t>& input, size_t degree);

/*
 * conversion from block to Galois field element of size 128
 * */

void block2GF(GF2E& res, const block& a );

/*
 * conversion from Galois field element to block of size 128
 * */
void GF2block(block& res, const GF2E& a);


/*
 * get inverse of an block in the field. In particular, inv* input = 1 in F_{2^k}
 * */

void inverse(block& inv, block& input);

void fill_data_and_mac(block& d, block& m);

GF2EX get_GF2EX_with_roots(vector<uint64_t>& roots);

typedef  vector<int64_t> CLS;
typedef vector<int64_t> SPT;
typedef vector<int64_t> PVT;

void readproof(string filename, int& d, vector<CLS>& clauses, vector<SPT>& supports, vector<PVT>& pivots, int& ncls, int& nres);
void read_info(string filename, int& c1_begin, int& c1_end, int& pub_end, int& n_c1_cls, vector<int>& assignment);

#endif //ZKUNSAT_NEW_UTILS_H
