#ifndef UTILITY_H
#define UTILITY_H

#include <random>

bool majority(bool votes[], int size);

/* Calculate node failure based on fault chance */
bool failure(unsigned fault_chance);

/* Returns a random integer */
int rand(int min_timeout, int max_timeout);

#endif