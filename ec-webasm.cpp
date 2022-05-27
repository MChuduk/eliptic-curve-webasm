#include <iostream>
#include <bitset>

struct Point {
    int64_t x = 0;
    int64_t y = 0;
};

void scalarMultiply(int64_t a, int64_t b, int64_t M, int64_t x, int64_t y, int64_t k, int64_t* dx, int64_t* dy, float* time);
Point add(Point p1, Point p2, int64_t a, int64_t M);
int64_t getSlope(Point p1, Point p2, int64_t a, int64_t M);
int64_t modulo(int64_t m, int64_t n);
int64_t inverseMod(int64_t a, int64_t m);

int main()
{
    int64_t dx = 0;
    int64_t dy = 0;
    float time = 0;
    scalarMultiply(3, 7, 11, 0, 4, 20, &dx, &dy, &time);

    std::cout << "dx: " << dx << std::endl;
    std::cout << "dy: " << dy << std::endl;
    std::cout << "time: " << time << std::endl;
}

void scalarMultiply(int64_t a, int64_t b, int64_t M, int64_t x, int64_t y, int64_t k, int64_t* dx, int64_t* dy, float* time) {
    clock_t begin_time = clock();
    Point P;
    P.x = x;
    P.y = y;
    Point Q;

    std::string bin = std::bitset<64>(k).to_string();

    for (int i = 0; i < bin.length(); i++) {
        Q = add(Q, Q, a, M);
        if (bin[i] == '1') {
            Q = add(Q, P, a, M);
        }
    }
    float time_out = float(clock() - begin_time);
    *dx = Q.x;
    *dy = Q.y;
    *time = time_out;
}

Point add(Point p1, Point p2, int64_t a, int64_t M) {
    if (p1.x == 0 && p1.y == 0) return p2;
    if (p2.x == 0 && p2.y == 0) return p1;

    int64_t slope = getSlope(p1, p2, a, M);
    int64_t x3 = modulo(slope * slope - p1.x - p2.x, M);
    int64_t y3 = modulo(slope * (p1.x - x3) - p1.y, M);

    Point P;
    P.x = x3;
    P.y = y3;
    return P;
}

/* This function finds the slope of two points
 * if the two points are equal, it means point doubling,
 * otherwise, it's point addition.
 * Point doubling: slope = (3*x1*x1 + a)/2*y1 % M
 * Point addition: slope = (y2-y1)/(x2-x1) % M
 */
int64_t getSlope(Point p1, Point p2, int64_t a, int64_t M) {
    int64_t slope = 0;
    int64_t numerator, denominator;

    // point doubling
    if ((p1.x == p2.x) && (p1.y == p2.y)) {
        numerator = modulo(3 * p1.x * p1.x + a, M);
        denominator = modulo(2 * p1.y, M);
    }
    // point addition
    else {
        numerator = modulo(p2.y - p1.y, M);
        denominator = modulo(p2.x - p1.x, M);
    }
    if (denominator == 0) {
        return 0;
    }
    /* since slope is modular division, if the numerator and denominator
     * are not coprime, we need to multiply the numerator by the modular
     * inverse of denominator.
     */
    int64_t inv = inverseMod(denominator, M);
    slope = inv * numerator % M;
    return slope;
}

int64_t modulo(int64_t m, int64_t n) {
    if (m >= 0) {
        return m % n;
    }
    else {
        int64_t mModn = m % n;
        mModn = abs(mModn);
        return (n - mModn) % n;
    }
}

int64_t inverseMod(int64_t a, int64_t m) {
    int m0 = m;
    int y = 0, x = 1;

    if (m == 1)
        return 0;

    while (a > 1) {
        // q is quotient
        int q = a / m;
        int t = m;

        // m is remainder now, process same as
        // Euclid's algo
        m = a % m, a = t;
        t = y;

        // Update y and x
        y = x - q * y;
        x = t;
    }

    // Make x positive
    if (x < 0)
        x += m0;

    return x;
}
