#include "utility.h"

bool majority(bool votes[], int size) {
    int count = 0;
    for (int i = 0; i < size; ++i) {
        if (votes[i]) {
            ++count;
        }
    }
    if (count > size/2) {
        return true;
    }
    return false;
}

bool failure(unsigned fault_chance) {
    bool fail = false;
    std::random_device d;
    std::mt19937 rng(d());
    std::uniform_int_distribution<std::mt19937::result_type> distribution(0,100);

    if (distribution(rng) <= fault_chance) {
        fail = true;
    }

    return fail;
}

int rand(int min_timeout, int max_timeout) {
    std::random_device d;
    std::mt19937 rng(d());
    std::uniform_int_distribution<std::mt19937::result_type> distribution(min_timeout, max_timeout);

    return distribution(rng);
}