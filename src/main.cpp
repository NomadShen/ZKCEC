#include <iostream>
#include "clause.h"
#include "clauseRAM.h"
#include "commons.h"
#include "encoder.hpp"
#include "clauseCheck.hpp"

using namespace emp;
using namespace NTL;
extern Encoder *encoder;

int main(int argc, char **argv) {
    #ifndef UNFOLD
        cout << "Optimized version!" << endl;
    #endif
    int port, party;
    const int threads = 8;
    cout << "---- begin ----" << endl;
    constant = CONSTANT;
    parse_party_and_port(argv, &party, &port);
    BoolIO <NetIO> *ios[threads];
    cout << party << endl;
    cout << port << endl;
    for (int i = 0; i < threads; ++i)
        ios[i] = new BoolIO<NetIO>(new NetIO(party == ALICE ? nullptr : "127.0.0.1", port + i), party == ALICE);       
    char *prooffile = argv[3];
    char *infofile = argv[4];

    int tmp_party = party;
    setup_zk_bool < BoolIO < NetIO >> (ios, threads, tmp_party);
    ZKBoolCircExec <BoolIO<NetIO>> *exec = (ZKBoolCircExec < BoolIO < NetIO >> *)(CircuitExecution::circ_exec);
    io = exec->ostriple->io;
    
    ostriple = new F2kOSTriple <BoolIO<NetIO>>(party, exec->ostriple->threads, exec->ostriple->ios,
        exec->ostriple->ferret, exec->ostriple->pool);
        svole = ostriple->svole;


    data_mac_pointer = 0;
    uint64_t test_n = svole->param.n;
    uint64_t mem_need = svole->byte_memory_need_inplace(test_n);


    data = new block[svole->param.n];
    mac = new block[svole->param.n];
    svole->extend_inplace(data, mac, svole->param.n);
    cout << "----set up----" << endl;

    //define the finite field
    GF2X P;
    SetCoeff(P, 128, 1);
    SetCoeff(P, 7, 1);
    SetCoeff(P, 2, 1);
    SetCoeff(P, 1, 1);
    SetCoeff(P, 0, 1);
    GF2E::init(P);

    //define paramenter
    int ncls = 0, nres = 0;
    int private_begin = 0, private_end = 0, public_end = 0;
    int n_c1_cls = 0;
    vector<int> assignment;
    vector<uint64_t> enc_assignment;

    //print current design
    cout << "curr design: " << string(prooffile) << endl;

    //read info
    read_info(string(infofile), private_begin, private_end, public_end, n_c1_cls, assignment);

    //init encoder
    encoder = new Encoder(private_begin, private_end);
    encoder->init();

    //encode assignment
    for (int i = 0; i < assignment.size(); i++) {
        uint64_t tmp;
        if(ostriple->party == ALICE) {
            tmp = encoder->encode(assignment[i]);
            io->send_data(&tmp, sizeof(uint64_t));
        } else {
            io->recv_data(&tmp, sizeof(uint64_t));
        }
        enc_assignment.push_back(tmp);
    }

    //read proof
    vector <CLS> clauses;
    vector <SPT> supports;
    vector <PVT> pivots;

    if (party == ALICE) {
        readproof(string(prooffile), DEGREE, clauses, supports, pivots, ncls, nres);
        cout << string(prooffile) << endl;
        cout << "----input proof----" << endl;
        io->send_data(&nres, 4);
        io->send_data(&ncls, 4);
        io->send_data(&DEGREE, 4);

    }

    if (party == BOB) {
        io->recv_data(&nres, 4);
        io->recv_data(&ncls, 4);
        io->recv_data(&DEGREE, 4);

        clauses = vector<CLS>(ncls);
        supports = vector<SPT>(ncls);
        pivots = vector <PVT> (ncls);
    }

    cout << "nres " << nres << endl;
    cout << "ncls " << ncls << endl;
    cout << "DEGREE " << DEGREE << endl;

    double cost_input = 0;
    double cost_rom = 0;
    double cost_sat = 0;
    double cost_disjoint = 0;
    double cost_resolve = 0;
    double cost_access = 0 ;

/*
 * encode the formula and resolution proof
 * */
    auto timer_0 = chrono::high_resolution_clock::now();
    vector<clause> raw_formula;
    float delta = 0 ;

    for (int i = 0; i < ncls; i++) {
        delta = delta + 1;
        if ((delta / ncls) > 0.1){
            float  progress = (float(i) / ncls);
            delta = 0;
            int barWidth = 30;
            std::cout << "[";
            int pos = barWidth * progress;
            for (int i = 0; i < barWidth; ++i) {
                if (i < pos) std::cout << "=";
                else if (i == pos) std::cout << ">";
                else std::cout << " ";
            }
            std::cout << "] " << int(progress * 100.0) << " %\r";
            cout << endl;
        }


        vector <uint64_t> literals;
        for (int64_t lit: clauses[i]) {
            literals.push_back((wrap(lit)));
        }
        #ifdef PADDING
            padding(literals);
        #else
        int lit_size;
        if(ostriple->party == ALICE) {
            if(literals.size()==0) literals.push_back(0UL);
            lit_size = literals.size();
            io->send_data(&lit_size, sizeof(int));
        } else {
            io->recv_data(&lit_size, sizeof(int));
            literals.resize(lit_size);
        }
        #endif
        clause c(literals);
        raw_formula.push_back(c);
    }
    auto timer_1 = chrono::high_resolution_clock::now();
    cost_input = chrono::duration<double>(timer_1 - timer_0).count();

    timer_0 = chrono::high_resolution_clock::now();
    //check non-intersection
    cout << "check non-intersection!" << endl;
    clauseChecker* checker = new clauseChecker();//init checker
    vector<uint64_t> pub_lits;
    for(int i = private_end + 1; i <= public_end; i++) {
        pub_lits.push_back(encoder->encode(i));
        pub_lits.push_back(encoder->encode(-i));
    }
    for(int i = 0; i < n_c1_cls;i++) {
       checker->check(raw_formula[i], pub_lits);
    }

    check_zero_MAC(zero_block, 1);
    delete checker;
    timer_1 = chrono::high_resolution_clock::now();
    cost_disjoint= chrono::duration<double>(timer_1 - timer_0).count();

    //check SAT
    timer_0 = chrono::high_resolution_clock::now();
    cout << "check C1 SAT!" << endl;
    for(int i = 0; i < n_c1_cls;i++) {
        checkSAT(raw_formula[i], enc_assignment);
    }
    check_zero_MAC(zero_block, 1);
    timer_1 = chrono::high_resolution_clock::now();
    cost_sat = chrono::duration<double>(timer_1 - timer_0).count();

    //init RAM
    timer_0 = chrono::high_resolution_clock::now();
    cout << "init clauseRAM!" << endl;
    clauseRAM<BoolIO<NetIO>>* formula = new clauseRAM<BoolIO<NetIO>>(party, INDEX_SZ);
    formula->init(raw_formula);
    raw_formula.clear();
    raw_formula.shrink_to_fit();
    cout <<"finish  input!" << endl;
    timer_1 = chrono::high_resolution_clock::now();
    cost_rom = chrono::duration<double>(timer_1 - timer_0).count();

    delta = 0;


    for (int64_t i = ncls - nres; i < ncls; i++) {
	    delta = delta + 1;
        if ((delta / nres) > 0.1){
            float  progress = (float(i + nres - ncls) / nres);
            delta = 0;
            int barWidth = 30;
            std::cout << "[";
            int pos = barWidth * progress;
            for (int i = 0; i < barWidth; ++i) {
                if (i < pos) std::cout << "=";
                else if (i == pos) std::cout << ">";
                else std::cout << " ";
            }
            std::cout << "] " << int(progress * 100.0) << " %\r";
            cout << endl;
        }

        int64_t chain_length = 0L;
        vector<uint64_t> pvt;
        vector<Integer> chain;

        if (party == ALICE) {
            chain_length = supports[i].size();
            io->send_data(&chain_length, 8);
        }else{
            io->recv_data(&chain_length, 8);
        }

        SPT s = supports[i];
        PVT p = pivots[i];


        if (party == BOB) {
            for (int j = s.size(); j < chain_length; j++) {
                s.push_back(0L);
            }
            for (int j = p.size(); j < chain_length - 1; j++) {
                p.push_back(0L);
            }
        }


        assert(s.size() == p.size() +1);
        for (uint64_t index: s) {
            chain.push_back(Integer(INDEX_SZ, index, ALICE));
        }

        pvt.push_back(0UL);
        for(uint64_t pp: p){
            pvt.push_back(pp);
        }

        bool last_clause = (i == (ncls - 1));

        auto cost = check_chain(chain, pvt, i, formula, last_clause);
        cost_resolve = cost_resolve + cost.second;
        cost_access = cost_access + cost.first;

    }
    
    check_zero_MAC(zero_block, 1);

    auto timer_4 = chrono::high_resolution_clock::now();

    formula->check();

    auto timer_5 = chrono::high_resolution_clock::now();
    cost_access = cost_access +  chrono::duration<double>(timer_5 - timer_4).count();

    bool cheat = finalize_zk_bool<BoolIO<NetIO>>();
    if(cheat)error("cheat!\n");

    uint64_t  counter = 0;

    for(int i = 0; i < threads; ++i) {
        counter = ios[i]->counter + counter;
        delete ios[i]->io;
        delete ios[i];
    }
        
    cout << "----report----" << endl;
    cout << "c1 lits: " << private_end << endl;
    cout << "c1 cls: " << n_c1_cls << endl;
    cout << "Length: " << nres << endl;
    cout << "Width: " << DEGREE << endl;
    cout << "p1 cost: "<< cost_input << endl;
    cout << "p2 cost: " << cost_access + cost_resolve + cost_rom << endl;
    cout << "p3 cost: "<< cost_sat << endl;
    cout << "p4 cost: "<< cost_disjoint << endl;
    cout << "total cost: "<< cost_access + cost_resolve + cost_input + cost_disjoint + cost_sat + cost_rom << endl;
    cout << "rom cost: "<< cost_rom << endl;
    cout << "access cost: "<< cost_access << endl;
    cout << "resolve cost: "<< cost_resolve << endl;
    cout << "communication: " << (double)counter/(1024*1024) << " MB" << endl;
    cout << "----end----" << endl;
    
    return 0;

}
