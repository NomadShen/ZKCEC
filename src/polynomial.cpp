#include "polynomial.h"

polynomial::polynomial(vector<block> _coefficient) {
    vector<block> mcoefficient;
    vector<block> coefficient;
    for (int i = 0; i < _coefficient.size(); i ++){
        block d, m;
        fill_data_and_mac(d, m);
        if (ostriple->party == ALICE) {
            block diff_coefficient;
            diff_coefficient= d^_coefficient[i];
            ostriple->io ->send_data(&diff_coefficient, sizeof(block));
            coefficient.push_back(_coefficient[i]);
            mcoefficient.push_back(m);
        }
        if (ostriple->party == BOB){
            block diff_coefficient;
            ostriple->io ->recv_data(&diff_coefficient, sizeof(block));
            gfmul(ostriple->delta, diff_coefficient, &diff_coefficient);
            coefficient.push_back(d);
            mcoefficient.push_back(m ^diff_coefficient);
        }
    }
    this->coefficient = coefficient;
    this->mcoefficient = mcoefficient;
}

polynomial::polynomial(vector <uint64_t> roots) {
    GF2EX res = get_GF2EX_with_roots(roots);

    std::vector<block> _coefficient;

    #ifdef PADDING
        long degree = DEGREE;
    #else
        long degree = roots.size()+1;
    #endif

    for (long i = 0; i < degree; i ++){
        GF2E c = NTL::coeff(res, i);
        block tmp = zero_block;
        GF2block(tmp, c);
        _coefficient.push_back(tmp);
    }
    vector<block> mcoefficient;
    vector<block> coefficient;
    for (int i = 0; i < degree; i ++){
        block d, m;
        fill_data_and_mac(d, m);

        if (ostriple->party == ALICE) {
            block diff_coefficient;
            diff_coefficient= d^_coefficient[i];
            ostriple->io ->send_data(&diff_coefficient, sizeof(block));
            coefficient.push_back(_coefficient[i]);
            mcoefficient.push_back(m);
        }
        if (ostriple->party == BOB){
            block diff_coefficient;
            ostriple->io ->recv_data(&diff_coefficient, sizeof(block));
            gfmul(ostriple->delta, diff_coefficient, &diff_coefficient);
            coefficient.push_back(d);
            mcoefficient.push_back(m ^diff_coefficient);
        }
    }
    this->coefficient = coefficient;
    this->mcoefficient = mcoefficient;
}

void polynomial::Evaluate(block &res, block &mres, block &input) const {
    int degree = this->coefficient.size(); 
    assert(degree > 1);
    multiply_const(res, mres, this->coefficient[degree-1], this->mcoefficient[degree-1], input, ostriple->party);
    compute_xor(res, mres, res, mres, this->coefficient[degree-2], this->mcoefficient[degree-2]);
    for (int i = degree-3; i > -1; i--){
        multiply_const(res, mres, res, mres, input, ostriple->party);
        compute_xor(res, mres, res, mres, this->coefficient[i] ,this->mcoefficient[i]);
    }
}

void polynomial::Equal(const polynomial& lhs) const{
    io->flush();
    block r = io->get_hash_block();
    block res[2], mac[2];
    this->Evaluate(res[0], mac[0], r);
    lhs.Evaluate(res[1], mac[1], r);
    check_zero_MAC(mac[0]^mac[1]);
}

void polynomial::InnerProductEqual(vector<polynomial> &p1, vector<polynomial> &p2) {
    io->flush();
    block r =io->get_hash_block();
    int d = p1.size();
    assert(p1.size() == p2.size());
    block res = zero_block;
    block mres = zero_block;
    for (int  i = 0; i < d; i ++){
        block tmp, mt, xx, xm, yy, ym;
        p1[i].Evaluate(xx, xm, r);
        p2[i].Evaluate(yy, ym, r);
        ostriple->compute_mul(tmp, mt, xx, xm, yy, ym);
        res = res ^ tmp;
        mres = mres ^ mt;
    }
    block resp, mresp;
    this->Evaluate(resp, mresp, r);
    check_zero_MAC(mresp^mres);
}

void polynomial::ProductEqual(polynomial& p1, polynomial &p2) {
    io->flush();
    block r =io->get_hash_block();
    block res = zero_block;
    block mres = zero_block;
    block xx, xm, yy, ym;
    p1.Evaluate(xx, xm, r);
    p2.Evaluate(yy, ym, r);
    ostriple->compute_mul(res, mres, xx, xm, yy, ym);
    block resp, mresp;
    this->Evaluate(resp, mresp, r);

    check_zero_MAC(mresp^mres );
}

void polynomial::ConverseCheck(polynomial & lhs) {


    io->flush();
    block r =io->get_hash_block();
    block converse_r = ((block) get_128uint_from_uint64(constant))^r;
    block xx, xm, yy, ym;
    this->Evaluate(xx, xm, r);
    lhs.Evaluate(yy, ym, converse_r);
    check_zero_MAC(xm^ym);       
}