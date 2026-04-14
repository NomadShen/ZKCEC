#pragma once
#ifndef ZKUNSAT_NEW_POLYNOMIAL_H
#define ZKUNSAT_NEW_POLYNOMIAL_H
#include "utils.h"
#include "commons.h"

class polynomial {
public:
    vector <block> coefficient;
    vector <block> mcoefficient;
    polynomial(){
    }
    polynomial(vector<block> coefficient);
    polynomial(vector<uint64_t> roots);
    void Evaluate(block &res, block &mres, block &input) const ;
    void Equal(const polynomial& lfh) const;
    void InnerProductEqual(vector<polynomial>& p1, vector<polynomial>& p2);
    void ProductEqual(polynomial& p1, polynomial& p2);
    void ConverseCheck(polynomial& lhs);

    void print() {
        for (block b: coefficient) {
            cout << "[" << (b) << "]";
        }
        cout << endl;
    }

    void print_info() {
        cout << "poly degree: " << coefficient.size() << endl;
    }
};
inline void GF2EX2polynomial(GF2EX& a, polynomial& b){

    long d = deg(a);
    assert(!(d > DEGREE));
    std::vector<block> coeff;
    for (long i = 0; i < DEGREE; i ++){
        GF2E c = NTL::coeff(a, i);
        block tmp;
        GF2block(tmp, c);
        coeff.push_back(tmp);
    }
    b = polynomial(coeff);
}



#endif //ZKUNSAT_NEW_POLYNOMIAL_H
