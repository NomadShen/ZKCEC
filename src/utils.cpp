//
// Created by anonymized on 12/7/21.
//


//
// Created by anonymized on 8/9/21.
//
#include "utils.h"
#include "commons.h"
#include "encoder.hpp"


using namespace  std;
using  namespace  NTL;
//using namespace emp;


int DEGREE = 4;
block *mac, *data;
uint64_t data_mac_pointer;
SVoleF2k <BoolIO<NetIO>> *svole;
F2kOSTriple <BoolIO<NetIO>> *ostriple;
uint64_t constant;
BoolIO <NetIO> *io;
Encoder *encoder;

/* Encode literal: the highest bit indicates the sign of that literal;
 * the rest bits indicate the value of the index;
 * for example: x2 is encoded as [1,0, ..., 0, 1, 0] , and not x2 is encoded as [0,0, ..., 0, 1, 0];
 * [0,0, ..., 0, 0, 0] and [1,0, ..., 0, 0, 0] are dummy bits;
 * used by prover locally;
 */

uint64_t wrap(int64_t input){
    if(ostriple->party == ALICE) {
        return encoder->encode(input);
        // if(input == 0) {
        //     return (uint64_t)0;
        // }
        // uint64_t  res = 0UL;
        // if(input >  0) {
        //     res = (uint64_t) input | constant;
        //     res = res | 1UL<<(VAL_SZ-1);
        // }
        // if(input <  0) {
        //     res = (uint64_t)(-input);
        //     res = res | 1UL<<(VAL_SZ-1);
        // }
        // return  res;
    } else {
        return (uint64_t)0;
    }
}

int64_t unwrap(uint64_t input) {

    if ((input & MSB_MASK) == 0) {
        return 0;
    }

    if ((input & SMSB_MASK) != 0) {
        //positive
        uint64_t res = input & ~(MSB_MASK | SMSB_MASK);
        return (int64_t)res;
    } else {
        //negative
        uint64_t res = input & ~MSB_MASK;
        return -(int64_t)res;
    }
}

//get mac of val

void pack(block& mac, const Integer& val, int size) {
    block res;
    vector_inn_prdt_sum_red(&res, (block*)(val.bits.data()), gp.base, size);
    mac = res;
}


/*
 * multiply a committed value x by cst; mac of x is m;
 * val = m * cst;
 * mac is the mac of val;
 */

void multiply_const(block &val, block &mac,
                           const block& x, const block& m, const block& cst, int party) {
                            
    // cout << "=====before ==== \n";
    // cout << (cst) << endl; 
    //  cout << (x) << endl;     
    // cout << (val) << endl;                       
    if(party == ALICE) {
        gfmul(x, cst, &val);//   cout << mac << endl;
    }
    gfmul(m, cst, &mac);
    // cout << "====after ===== \n";
    // cout << (cst) << endl; 
    // cout << (x) << endl;     
    // cout << (val) << endl; 
 }

/*
 * compute addition of vala and valb;
 * valc = vala + valb; maca is mac of a, macb is mac of b;
 * macc = mac(valc)
 */
void compute_xor(block &valc, block &macc,
                        const block vala, const block maca, const block valb, const block macb){
    valc = vala ^ valb;
    macc = maca ^ macb;
}


__uint128_t get_128uint_from_uint64(uint64_t value) {
    return (((__uint128_t)value)) ;
}

void check_MAC_valid(block X, block MAC) {
    BoolIO<NetIO>* io = ostriple->io;
    int party = ostriple->party;
    if(party == ALICE) {
        io->send_data(&X, sizeof(block));
        io->send_data(&MAC, sizeof(block));
        int64_t res = -1;
        io-> recv_data(&res, 8);
        if (res != 1) error("check_MAC failed!\n");
    } else {
        block M = zero_block, x = zero_block;
        io->recv_data(&x, sizeof(block));
        io->recv_data(&M, sizeof(block));
        gfmul(x, ostriple->delta, &x);
        x = x ^ MAC;
        if (memcmp(&x, &M, sizeof(block))!=0) {
            int64_t  res = 0;
            io->send_data(&res, 8);
            error("check_MAC failed!\n");
        }else {
            int64_t  res = 1;
            io->send_data(&res, 8);
        }

    }

}

/*
 * check if a block is a mac of zero.
 * It is an accumulator that takes all mac blocks to be check
 * At the end of the proof, prover by setting end = 1 will hash all blocks, and send it to verifier.
*/

void check_zero_MAC(block MAC, int end) {
    static Hash hash;
    hash.put(&MAC, sizeof(block));
    if (end == 1){
        BoolIO<NetIO>* io = ostriple->io;
        int party = ostriple->party;
        char dig[Hash::DIGEST_SIZE];
        hash.digest(dig);

        if(party == ALICE) {
            io->send_data(dig, sizeof(dig));
        } else {
            char receive[Hash::DIGEST_SIZE];
            io->recv_data(receive, sizeof(receive));
            cout << "receive size: " << sizeof(receive) << endl;
            cout << "digest size:  " << sizeof(dig) << endl;
            if (memcmp(receive, dig, sizeof(dig))!=0) {
                error("check_zero failed!\n");
            }else {
                cout << "check_zero succeed!!!" << endl;
            }
        }
        return;
    }

}




/*
 * padding a vector of int64 to the size of degree
 * used when prover input a clause
 */
void padding(vector<uint64_t>& input){
    for (int i = input.size() ; i < DEGREE; i ++){
        input.push_back(0UL);
    }
}

void padding(vector<uint64_t>& input, size_t degree){
    assert(input.size() <= degree);
    for (int i = input.size() ; i < degree; i ++){
        input.push_back(0UL);
    }
}

/*
 * conversion from block to Galois field element of size 128
 * */

void block2GF(GF2E& res, const block& a ){
    // GF2X raw;
    // //  cout <<(a) << endl;
    // auto low = _mm_extract_epi64(a, 0);
    // auto high = _mm_extract_epi64(a, 1);
    // // cout << low << endl;
    // //cout << high << endl;

    // raw.SetMaxLength(128);
    // for(int i = 0; i < 64; i++){
    //     SetCoeff(raw, long(i), (1 &(low >> (i))));
    //     SetCoeff(raw, long(i+64), (1 &(high >> (i))));
    // }
    // res._GF2E__rep = raw;
    NTL::GF2XFromBytes(res._GF2E__rep, (const unsigned char*)&a, 16);
}

/*
 * conversion from Galois field element to block of size 128
 @ssr
 * */
void GF2block(block& res, const GF2E& a) {
    // GF2X raw = a._GF2E__rep;
    // res = zero_block; 
    // for (int i = 0; i < 128; i++) {
    //     if (IsOne(NTL::coeff(raw, i)))
    //         res = set_bit(res, i);
    // }

    //@ssr better version
    const GF2X& raw = a._GF2E__rep;
    NTL::BytesFromGF2X((unsigned char*)&res, raw, 16);
}


/*
 * get inverse of an block in the field. In particular, inv* input = 1 in F_{2^k}
 * */

void inverse(block& inv, block& input){
    GF2E _input, _inv;
    block2GF(_input, input );
    if(_input == GF2E(0)) {
        cout << "no inverse!" << endl;
        error("no inverse!");
        return;
    }
    NTL::inv(_inv, _input);
    GF2block(inv, _inv);
}

void fill_data_and_mac(block& d, block& m){
    if (data_mac_pointer == svole->param.n){
        svole->extend_inplace(data, mac, svole->param.n);
        data_mac_pointer = 0;
    }
    d = data[data_mac_pointer];
    m = mac[data_mac_pointer];
    data_mac_pointer = data_mac_pointer + 1;

}

GF2EX get_GF2EX_with_roots(vector<uint64_t>& roots){
    GF2EX res, tmp;
    SetCoeff(res, 0); // res = 1
    for (const auto& r : roots){
        tmp = GF2EX();
        GF2E coefficient, constant;
        if (r == 0){
            block2GF(coefficient, zero_block);
            block2GF(constant, one_block);
            SetCoeff(tmp, 0, constant);
            SetCoeff(tmp, 1, coefficient);
        }else{
            block2GF(constant, (block)get_128uint_from_uint64(r));
            block2GF(coefficient, one_block);
            SetCoeff(tmp, 0, constant);
            SetCoeff(tmp, 1, coefficient);
        }
        res = tmp * res;
    }
    return res;
}

typedef  vector<int64_t> CLS;
typedef vector<int64_t> SPT;
typedef vector<int64_t> PVT;

void readproof(string filename, int& d, vector<CLS>& clauses, vector<SPT>& supports, vector<PVT>& pivots, int& ncls, int& nres) {
    std::ifstream file(filename);
    std::string str;
    ncls = 0;
    nres = 0;
    d = 0;

    while (std::getline(file, str)) {
        istringstream ss(str);
        string word;
        while (ss >> word) {
            if (word == "clause:") {
                int nltr  = 0;
                CLS clause;
                ss >> word;
                while (word != "support:") {
                    int i = stoi(word);
                    clause.push_back(i);
                    nltr = nltr + 1;
                    ss >> word;
                }
                if (d < nltr) d = nltr;
                clauses.push_back(clause);
                ncls ++ ;
            }
            if (word == "support:") {
                SPT support;
                ss >> word;
                while (word != "pivot:") {
                    int i = stoi(word);
                    support.push_back(i);
                    ss >> word;
                }
                supports.push_back(support);
            }
            if (word == "pivot:") {
                SPT pchain;
                ss >> word;
                while (word != "end:") {
                    int i = stoi(word);
                    pchain.push_back(wrap(i));
                    ss >> word;
                }
                if (pchain.size() > 0){
                    nres = nres + 1;
                }
                pivots.push_back(pchain);

            }
            if (word == "DEGREE:"){
                ss >> word;
                d = stoi(word);
            }
        }
    }
}

void read_info(string filename, int& c1_begin, int& c1_end, int& pub_end, int& n_c1_cls, vector<int>& assignment) {
    std::ifstream file(filename);
    std::string str;

    int input_lit = 0;
    int c1_lit = 0;
    int c2_lit = 0;
    int miter_lit = 0;
    int c1_cls = 0;
    int c2_cls = 0;
    int miter_cls = 0;

    assignment.clear();

    while(getline(file, str)) {
        istringstream ss(str);
        string word;

        ss >> word;
        if (word == "input_lit:") {
            ss >> word;
            input_lit = stoi(word);
        } else if(word == "c1_lit:") {
            ss >> word;
            c1_lit = stoi(word);
        } else if(word == "c2_lit:") {
            ss >> word;
            c2_lit = stoi(word);
        } else if(word == "miter_lit:") {
            ss >> word;
            miter_lit = stoi(word);
        } else if(word == "c1_cls:") {
            ss >> word;
            c1_cls = stoi(word);
        } else if(word == "c2_cls:") {
            ss >> word;
            c2_cls = stoi(word);
        } else if(word == "miter_cls:") {
            ss >> word;
            miter_cls = stoi(word);
        } else if(word == "c1_sat:") {
            while(ss >> word) {
                if(ostriple->party == ALICE) {//only Prover knows the witness
                    assignment.push_back(stoi(word));
                } else {
                    assignment.push_back(0);
                }
            }
        }
    }

    c1_begin = input_lit + 1;
    c1_end = input_lit + c1_lit;
    pub_end = input_lit + c1_lit + c2_lit + miter_lit;

    n_c1_cls = c1_cls;

    return;
}

