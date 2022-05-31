#include <stdio.h>
#include <time.h>
#include "bignum.h"
#include "eccrypt.h"
#include <emscripten/emscripten.h>

struct eccrypt_curve_t curve;
struct eccrypt_point_t p1, p2, rslt;
bignum_digit_t k[ECCRYPT_BIGNUM_DIGITS];
char buff[256];
clock_t start_t, end_t;
double total_t;

void EMSCRIPTEN_KEEPALIVE initCurve(char* a, char* b, char* p, char* gx, char* gy) {
	/* инициализируем параметры кривой */
	bignum_fromhex(curve.a, a, ECCRYPT_BIGNUM_DIGITS);
	bignum_fromhex(curve.b, b, ECCRYPT_BIGNUM_DIGITS);
	bignum_fromhex(curve.m, p, ECCRYPT_BIGNUM_DIGITS);
	bignum_fromhex(curve.g.x, gx, ECCRYPT_BIGNUM_DIGITS);
	bignum_fromhex(curve.g.y, gy, ECCRYPT_BIGNUM_DIGITS);
	curve.g.is_inf = 0;

	printf("initCurve() a=%s, b=%s, p=%s, gx=%s, gy=%s\n", a, b, p, gx, gy);
}

EMSCRIPTEN_KEEPALIVE
void multByScalar(char* x, char* y, char* k_in) {
	start_t = clock();
	bignum_fromhex(p1.x, x, ECCRYPT_BIGNUM_DIGITS);
	bignum_fromhex(p1.y, y, ECCRYPT_BIGNUM_DIGITS);
	p1.is_inf = bignum_iszero(p1.x, ECCRYPT_BIGNUM_DIGITS) &&
		bignum_iszero(p1.y, ECCRYPT_BIGNUM_DIGITS);

	bignum_fromhex(k, k_in, ECCRYPT_BIGNUM_DIGITS);
	eccrypt_point_mul(&rslt, &p1, k, &curve);

	end_t = clock();
	total_t = (double)(end_t - start_t) / CLOCKS_PER_SEC;

	printf("multByScalar() x=%s, y=%s, k=%s, time=%f\n", x, y, k_in, total_t);
}

EMSCRIPTEN_KEEPALIVE
char* get_qx() {
	if (rslt.is_inf) {
		return "0";
	}
	else {
		bignum_tohex(rslt.x, buff, sizeof(buff), ECCRYPT_BIGNUM_DIGITS);
		return buff;
	}
}

EMSCRIPTEN_KEEPALIVE
char* get_qy() {
	if (rslt.is_inf) {
		return "0";
	}
	else {
		bignum_tohex(rslt.y, buff, sizeof(buff), ECCRYPT_BIGNUM_DIGITS);
		return buff;
	}
}

double EMSCRIPTEN_KEEPALIVE get_time() {
	return total_t;
}

int main() {
	initCurve("3", "5", "11", "0", "0");
	multByScalar("1", "1", "5");

	if (rslt.is_inf) {
		printf("0\n0\n(infinite point)\n");
	}
	else {
		bignum_tohex(rslt.x, buff, sizeof(buff), ECCRYPT_BIGNUM_DIGITS);
		printf("%s\n", buff);
		bignum_tohex(rslt.y, buff, sizeof(buff), ECCRYPT_BIGNUM_DIGITS);
		printf("%s\n", buff);
		printf("(real point)\n");
	}
}