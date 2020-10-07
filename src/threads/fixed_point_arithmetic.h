// Created by jihun for q3
// Used for fixed point arithmetic in mlfqs

#ifndef THREADS_FIXED_POINT_ARITHMETIC_H
#define THREADS_FIXED_POINT_ARITHMETIC_H


#define F (1 << 14)

#define N_TO_FP(n) (n * F)
#define X_TO_INT_TZ(x) (x / F) // toward zero
#define X_TO_INT_TN(x) (x >= 0 ? ((x + F / 2) / F) : ((x - F / 2) / F)) // toward nearest

#define X_PLUS_Y(x, y) (x + y)
#define X_PLUS_N(x, n) (x + n * F)

#define X_MINUS_Y(x, y) (x - y)
#define X_MINUS_N(x, n) (x - n * F)

#define X_TIMES_Y(x, y) (((int64_t)x) * y / F)
#define X_TIMES_N(x, n) (x * n)

#define X_OVER_Y(x, y) (((int64_t)x) * F / y)
#define X_OVER_N(x, n) (x / n)


#endif //THREADS_FIXED_POINT_ARITHMETIC_H
