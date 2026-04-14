#ifndef ZKUNSAT_NEW_CLAUSE_CHECKER_H
#define ZKUNSAT_NEW_CLAUSE_CHECKER_H

#include "clause.h"
#include "commons.h"

class clauseChecker {
    public:
    block one_d;
    block one_m;
    clauseChecker() {
        //init one block
        block d, m;
        fill_data_and_mac(d, m);
        block one = (block) get_128uint_from_uint64(1);
        if(ostriple->party == ALICE) {
            block tmp = d ^ one;
            ostriple->io ->send_data(&tmp, sizeof(block));
            one_d = one;
            one_m = m;
        } else {
            block tmp;
            ostriple->io ->recv_data(&tmp, sizeof(block));
            gfmul(ostriple->delta, tmp, &tmp);
            one_d = d;
            one_m = m ^ tmp;
        }
    }

    void check(const clause& input, const vector<uint64_t>& pub_lits) {
        block xx, xm;
        block r;

        for(const uint64_t& lit: pub_lits) {

            r = (block)get_128uint_from_uint64(lit);
            input.poly.Evaluate(xx, xm, r);

            block d, m;
            block inv, inv_mac;
            fill_data_and_mac(d, m);

            if (ostriple->party == ALICE) {
                inverse(inv, xx);
                inv_mac = m;
                block masked_inv;
                masked_inv = d ^ inv;
                ostriple->io ->send_data(&masked_inv, sizeof(block));
            } else {
                block masked_inv;
                ostriple->io ->recv_data(&masked_inv, sizeof(block));
                gfmul(ostriple->delta, masked_inv, &masked_inv);
                inv = zero_block;
                inv_mac = m ^ masked_inv;
            }

            block tx, tm;
            ostriple->compute_mul(tx, tm, xx, xm, inv, inv_mac);
            
            check_zero_MAC(tm^one_m);
        }
    }

};

inline void checkSAT(const clause& input, const vector<uint64_t>& witness) {

    block xx, xm;
    block tx, tm;
    for(int i = 0; i < witness.size(); i++) {
        block r = (block)get_128uint_from_uint64(witness[i]);
        input.poly.Evaluate(xx, xm, r);
        if(i==0) {
            tx = xx;
            tm = xm;
        } else {
            ostriple->compute_mul(tx, tm, xx, xm, tx, tm);
        }
    }

    check_zero_MAC(tm);
        
}

#endif