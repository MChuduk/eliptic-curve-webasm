#include <string.h>
#include "eccrypt.h"

/*
 *  (c) Alexandr A Alexeev 2010 | http://eax.me/
 */

 // TODO: ������������ ����� ����� ��� ������������ a � b � eccrypt_curve_t

 /* ����������� ����� */
void eccrypt_point_cpy(struct eccrypt_point_t* to, /* ���� �������� */
    struct eccrypt_point_t* from) { /* ������ */
    if (to == from) return;
    if (to->is_inf = from->is_inf)
        return;

    memcpy(to->x, from->x, BIGNUM_SIZE(ECCRYPT_BIGNUM_DIGITS));
    memcpy(to->y, from->y, BIGNUM_SIZE(ECCRYPT_BIGNUM_DIGITS));
}

/* �������� ����� ������������� ������ */
void eccrypt_point_add(struct eccrypt_point_t* s, /* ����� */
    struct eccrypt_point_t* p, /* ������ ��������� */
    struct eccrypt_point_t* q, /* ������ ��������� */
    struct eccrypt_curve_t* curve) { /* ��������� ������ */
    struct eccrypt_point_t rslt; /* ������ �������� �������� s = p = q */
    bignum_digit_t lambda[ECCRYPT_BIGNUM_DIGITS]; /* ����������� ������ */
    struct eccrypt_point_t* tp; /* ��������� ��������� */
    int equal = 1; /* ������������, ��� ����� ����� */

    /* �������� �� �������������: P + O = O + P = P ��� ����� ����� ���� */
    if (p->is_inf) { tp = p; p = q; q = tp; }
    if (q->is_inf) {
        eccrypt_point_cpy(s, p);
        return;
    }

    if ((p != q) && (equal = !bignum_cmp(p->x, q->x, ECCRYPT_BIGNUM_DIGITS)))
        /* ������ ����� �����������, �� ���������� �� x ����� */
        if (!(equal = !bignum_cmp(p->y, q->y, ECCRYPT_BIGNUM_DIGITS))) {
            /* ��������� ������ Q = -P */
            memcpy(rslt.x, p->y, BIGNUM_SIZE(ECCRYPT_BIGNUM_DIGITS));
            bignum_madd(rslt.x, q->y, curve->m, ECCRYPT_BIGNUM_DIGITS);
            if (bignum_iszero(rslt.x, ECCRYPT_BIGNUM_DIGITS)) {
                s->is_inf = 1;
                return;
            }
        }

    /* ��������� ������ */
    if (equal) {
        /* lambda = (3*x*x + a) / (2*y)  (mod m) */
        bignum_setzero(rslt.x, ECCRYPT_BIGNUM_DIGITS);   /* t := 2 */
        rslt.x[0] = 2;
        bignum_mmul(rslt.x, p->y, curve->m, ECCRYPT_BIGNUM_DIGITS); /* t *= y */
        bignum_setzero(lambda, ECCRYPT_BIGNUM_DIGITS);   /* l := 3 */
        lambda[0] = 3;
        bignum_mmul(lambda, p->x, curve->m, ECCRYPT_BIGNUM_DIGITS); /* l *= x */
        bignum_mmul(lambda, p->x, curve->m, ECCRYPT_BIGNUM_DIGITS); /* l *= x */
        bignum_madd(lambda, curve->a, curve->m, ECCRYPT_BIGNUM_DIGITS); /* l += a */
    }
    else {
        /* lambda = (y1 - y2) / (x1 - x2)  (mod m) */
        memcpy(rslt.x, p->x, BIGNUM_SIZE(ECCRYPT_BIGNUM_DIGITS)); /* t := x1 */
        bignum_msub(rslt.x, q->x, curve->m, ECCRYPT_BIGNUM_DIGITS); /* t -= x2 */
        memcpy(lambda, p->y, BIGNUM_SIZE(ECCRYPT_BIGNUM_DIGITS)); /* l := y1 */
        bignum_msub(lambda, q->y, curve->m, ECCRYPT_BIGNUM_DIGITS); /* l -= y2 */
    }
    bignum_mdiv(lambda, rslt.x, curve->m, ECCRYPT_BIGNUM_DIGITS); /* l /= t */

    /* x3 = lambda*lambda - x1 - x2  (mod m) */
    memcpy(rslt.x, lambda, BIGNUM_SIZE(ECCRYPT_BIGNUM_DIGITS)); /* x3 := l */
    bignum_mmul(rslt.x, rslt.x, curve->m, ECCRYPT_BIGNUM_DIGITS); /* x3 *= l */
    bignum_msub(rslt.x, p->x, curve->m, ECCRYPT_BIGNUM_DIGITS); /* x3 -= x1 */
    bignum_msub(rslt.x, q->x, curve->m, ECCRYPT_BIGNUM_DIGITS); /* x3 -= x2 */

    /* y3 = lambda*(x1 - x3) - y1  (mod m) */
    memcpy(rslt.y, p->x, BIGNUM_SIZE(ECCRYPT_BIGNUM_DIGITS)); /* y3 := x1 */
    bignum_msub(rslt.y, rslt.x, curve->m, ECCRYPT_BIGNUM_DIGITS); /* y3 -= x3 */
    bignum_mmul(rslt.y, lambda, curve->m, ECCRYPT_BIGNUM_DIGITS); /* y3 *= l */
    bignum_msub(rslt.y, p->y, curve->m, ECCRYPT_BIGNUM_DIGITS); /* y3 -= y1 */

    rslt.is_inf = 0; /* �������� ����������� ����� */
    eccrypt_point_cpy(s, &rslt);
}

/* ��������� ����� ������������� ������ */
void eccrypt_point_mul(struct eccrypt_point_t* rslt, /* ��������� */
    struct eccrypt_point_t* p, /* ����� */
    bignum_digit_t* k, /* ��������� */
    struct eccrypt_curve_t* curve) { /* ��������� ������ */
    struct eccrypt_point_t point; /* ����� ����� */
    bignum_digit_t digit; /* �������� �������� ������� ��������� */
    int digit_num = 0; /* ����� ������� */
    int bits = 0; /* ���-�� �������������� ��� � ������� */
    int n = ECCRYPT_BIGNUM_DIGITS; /* ����� �������� �������� */

    if (p->is_inf) {
        rslt->is_inf = 1;
        return; /* n * O = O */
    }

    while ((n > 0) && !k[n - 1]) n--; /* ���������� ������� �������� ������ */
    if (n) eccrypt_point_cpy(&point, p); /* ������� ����� ����� */

    /* ������������� �����, ������ �� �� ����� ������ rslt,
    ��� ��� �������� �������� rslt == p */
    rslt->is_inf = 1;

    /* ���� ���� �������������� ���� */
    while ((digit_num < n) || digit) {
        if (digit_num) /* ��� �������� �� ������ �� ����� */
            eccrypt_point_add(&point, &point, &point, curve); /* point *= 2 */

        if (!bits) { /* ������� ������ ��� ��������� ��� ��� �� ��������������� */
            digit = k[digit_num++];
            bits = sizeof(bignum_digit_t) * 8;
        }

        if (digit & 1)
            eccrypt_point_add(rslt, rslt, &point, curve); /* rslt += point */

        digit >>= 1;
        bits--;
    }
}