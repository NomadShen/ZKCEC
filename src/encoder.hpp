#ifndef ZKUNSAT_ENCODER_H
#define ZKUNSAT_ENCODER_H

#include <unordered_map>

#include "commons.h"
#include "sodium.h"

class Encoder {

private:
    unsigned char key_[crypto_shorthash_KEYBYTES];

    int secret_start_;
    int secret_end_;
    int secret_size_;

    unordered_map<int, uint64_t> forward_map_;
    unordered_map<uint64_t, int> reverse_map_;

    uint64_t compute_hash(int val) {
        unsigned char hash_out[crypto_shorthash_BYTES]; // 8 bytes
        
        crypto_shorthash(hash_out, 
                         reinterpret_cast<const unsigned char*>(&val), 
                         sizeof(val), 
                         key_);

        // cast to uint64_t
        uint64_t result;
        std::memcpy(&result, hash_out, sizeof(result));
        result &= ~(MSB_MASK);
        return result;
    }

    bool secret_range_check(int id) {
        int abs_id = abs(id);
        return (abs_id >= secret_start_ && abs_id <= secret_end_);
    }

public:
    Encoder(int secret_start, int secret_end) {
        sodium_init();//init sodium

        //init parameter
        secret_start_ = secret_start;
        secret_end_ = secret_end;
        secret_size_ = 2 * (secret_end - secret_start + 1);

        crypto_shorthash_keygen(key_);

        forward_map_.reserve(secret_size_);
        reverse_map_.reserve(secret_size_);
    }

    int init() {//init encode map

        for(int i = secret_start_; i <= secret_end_; i ++){
                uint64_t hash = compute_hash(i);
                uint64_t hash_neg = hash ^ CONSTANT;
            forward_map_[i] = hash;
            forward_map_[-i] = hash_neg;
            reverse_map_[hash] = i;
            reverse_map_[hash_neg] = -i;
        }

        return 0;
    }


    int sync_key() {// sync key and re-compute Bob's encode map
        if(ostriple->party == ALICE) {
            ostriple->io->send_data(key_, sizeof(key_));
        } else {
            ostriple->io->recv_data(key_, sizeof(key_)); 

            for(int i = secret_start_; i <= secret_end_; i ++){
                uint64_t hash = compute_hash(i);
                uint64_t hash_neg = hash ^ CONSTANT;
                forward_map_[i] = hash;
                forward_map_[-i] = hash_neg;
                reverse_map_[hash] = i;
                reverse_map_[hash_neg] = -i;
            }   
        }

        return 0;
    }


    //int -> uint64_t
    uint64_t encode(int val) {
        if(val == 0) return (uint64_t)0;

        if(secret_range_check(val)) {//in secret range, use keyed hash encoding
            return forward_map_[val];
        } else {
            if(val > 0) return (uint64_t) val | MSB_MASK;
            if(val < 0) return (uint64_t) (-val | SMSB_MASK) | MSB_MASK;
        }
    }

    //uint64_t -> int
    int decode(uint64_t hash) {
        if(hash == 0) return 0;

        if(hash & MSB_MASK) {
            if((hash & SMSB_MASK) != 0) {
                //positive
                uint64_t res = hash & ~(MSB_MASK | SMSB_MASK);
                return (int)res;
            } else {
                //negative
                uint64_t res = hash & ~MSB_MASK;
                return -(int)res;
            }
        } else {        
            auto it = reverse_map_.find(hash);
            if (it == reverse_map_.end()) {
                throw std::runtime_error("Hash not found");
            }
            return it->second;
        }

        return 0;

    }

};

#endif