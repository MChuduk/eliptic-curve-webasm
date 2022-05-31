#include <stdio.h>
#include <string.h>
#include "bignum.h"

/*
 *  (c) Alexandr A Alexeev 2010 | http://eax.me/
 */

 /* �������������� ����� � ������ */
void bignum_tohex(bignum_digit_t* num, char* buff, int buffsize, int digits) {
    static char hextbl[] = "0123456789ABCDEF";
    int pos, offset = (sizeof(bignum_digit_t) << 1) - 1;

    while ((digits > 0) && !num[digits - 1]) /* ���� ������ �������� ������ */
        digits--;

    if (!digits) {
        if (buffsize >= 2) {
            buff[0] = '0';
            buff[1] = 0;
        }
        return;
    }

    pos = digits - 1;

    while (!((num[pos] >> (offset << 2)) & 0xF))
        offset--;

    while ((pos >= 0) && (buffsize > 1)) {
        *buff++ = hextbl[(num[pos] >> (offset-- << 2)) & 0xF];
        buffsize--;
        if (offset < 0) {
            offset = sizeof(bignum_digit_t) * 2 - 1;
            pos--;
        }
    }

    *buff = 0;
}

/* �������������� ������ � ����� */
void bignum_fromhex(bignum_digit_t* num, char* str, int digits) {
    int pos = 0, offset = 0, len = strlen(str);
    char c;

    memset(num, 0, BIGNUM_SIZE(digits));

    while (len && (pos < digits)) {
        c = str[--len];
        if (c >= '0' && c <= '9')
            c -= '0';
        else if (c >= 'A' && c <= 'F')
            c -= 'A' - 10;
        else if (c >= 'a' && c <= 'f')
            c -= 'a' - 10;
        else return;

        num[pos] = num[pos] | (c << (offset++ << 2));

        if (offset == sizeof(bignum_digit_t) * 2) {
            offset = 0;
            pos++;
        }
    }
}

/* �������� �� ����� ����� */
int bignum_iszero(bignum_digit_t* num, int digits) {
    while (digits--)
        if (num[digits])
            return 0; /* �� ���� */
    return 1;
}

/* ��������� ����� �������� ���� */
void bignum_setzero(bignum_digit_t* num, int digits) {
    memset(num, 0, BIGNUM_SIZE(digits));
}

/* ��������� ���� �����: 0 => �����, 1 => a > b, -1 => a < b*/
int bignum_cmp(bignum_digit_t* a, bignum_digit_t* b, int digits) {
    while (digits--)
        if (a[digits] > b[digits])
            return 1;
        else if (a[digits] < b[digits])
            return -1;
    return 0;
}

/* ����������� ����� */
void bignum_cpy(bignum_digit_t* to, bignum_digit_t* from,
    int to_digits, int from_digits) {
    int i = 0;
    while ((i < to_digits) && (i < from_digits)) {
        to[i] = from[i];
        i++;
    }

    while (i < to_digits)
        to[i++] = 0;
}

/* ��������� b � a. digits - ���-�� �������� � ������ */
void bignum_add(bignum_digit_t* a, bignum_digit_t* b, int digits) {
    int i;
    bignum_double_t carry = 0;

    for (i = 0; i < digits; i++)
        a[i] = carry = ((bignum_double_t)a[i] + b[i] + (carry >> BIGNUM_DIGIT_BITS));
}

/* ������� b �� a. digits - ���-�� �������� � ������ */
void bignum_sub(bignum_digit_t* a, bignum_digit_t* b, int digits) {
    bignum_double_t borrow = 0;
    int i;

    for (i = 0; i < digits; i++)
        a[i] = borrow = ((bignum_double_t)a[i] - b[i] - (borrow >> (BIGNUM_DIGIT_BITS * 2 - 1)));
}

/* ����� ������������ a �� b � �������� ��������� � result
result ������ �������� �� 2*digits ��������
��� ������� ������������ ������ ������ ������ */
void bignum_mul_raw(bignum_digit_t* a, bignum_digit_t* b,
    bignum_digit_t* result, int digits) {
    bignum_double_t carry;
    int i, j;

    bignum_setzero(result, BIGNUM_MAX_DIGITS * 2);

    for (i = 0; i < digits; i++) {
        carry = 0;
        for (j = 0; j < digits; j++)
            /* ��� ���������� ���� a[i] ��������� �� ����� �������� �� amd64 */
            result[i + j] = carry = (result[i + j] + (bignum_double_t)a[i] * b[j] +
                (carry >> BIGNUM_DIGIT_BITS));

        result[i + digits] = carry >> BIGNUM_DIGIT_BITS;
    }
}

/* �������� a �� b */
void bignum_mul(bignum_digit_t* a, bignum_digit_t* b, int digits) {
    bignum_digit_t result[BIGNUM_MAX_DIGITS * 2]; /* ������������ */
    bignum_mul_raw(a, b, result, digits);
    bignum_cpy(a, result, digits, BIGNUM_MAX_DIGITS * 2);
}

/* ��������� ������� a �� b, ����� ������� � �������
��� ���������� ������ ������� mmul �� ��������� ������������ �����,
���������� BIGNUM_MAX_DIGITS*2 �������� */
void bignum_div(bignum_digit_t* a, /* ������� */
    bignum_digit_t* b, /* �������� */
    bignum_digit_t* q, /* �������, ����� ���� a, b ��� NULL */
    bignum_digit_t* r, /* �������, ����� ���� a, b ��� NULL */
    int digits) { /* ���-�� �������� � ������ */
    bignum_digit_t td[BIGNUM_MAX_DIGITS * 2]; /* ��������������� �������� */
    bignum_digit_t tq[BIGNUM_MAX_DIGITS * 2]; /* ������� */
    bignum_digit_t tr[BIGNUM_MAX_DIGITS * 2 + 1]; /* ��������������� ������� */
    bignum_double_t qhat, rhat, product, carry; /* ������������ ���� ����� */
    bignum_signed_double_t temp;
    int i, j, n, shift = 0;

    bignum_setzero(tq, BIGNUM_MAX_DIGITS * 2);

    /* ���������� ������� ��������� ������ �������� */
    n = digits;
    while ((n > 1) && !b[n - 1]) n--;

    if (n == 1) {
        /* �������� ����� ����� ���� ������ - ���������� ������� ������� ��������.
        ��� �� ����������� - ���������� ����� ��������� �������, ����� �������� ����
        ������ ��� ������� */
        carry = 0;
        for (j = digits - 1; j >= 0; j--) {
            tq[j] = ((carry << BIGNUM_DIGIT_BITS) | a[j]) / b[0];
            carry = ((carry << BIGNUM_DIGIT_BITS) | a[j]) - tq[j] * b[0];
        }

        if (q) /* ��������� ������� */
            bignum_cpy(q, tq, digits, BIGNUM_MAX_DIGITS * 2);

        if (r) {
            bignum_setzero(r, digits);
            r[0] = carry; /* ��������� ������� */
        }
        return;
    }

    /* ���������� shift - �� ������� ��� ����� ����� ���������
    ��������, ����� ������� ������ ���� ����� ������� */
    while (!((b[n - 1] << shift) & (1 << (BIGNUM_DIGIT_BITS - 1))))
        shift++;

    bignum_setzero(td, BIGNUM_MAX_DIGITS * 2);
    bignum_setzero(tr, BIGNUM_MAX_DIGITS * 2 + 1);

    /* �� amd64 ��� bignum_digit_t ������������ ����� int. ��� shift = 0
    a[digits - 1] >> 32 == a[digits - 1], ��� ������ ���������� ���������� ���� */
    tr[digits] = (bignum_double_t)a[digits - 1] >> (BIGNUM_DIGIT_BITS - shift);
    for (i = digits - 1; i > 0; i--)
        tr[i] = (a[i] << shift) | ((bignum_double_t)a[i - 1] >> (BIGNUM_DIGIT_BITS - shift));
    tr[0] = a[0] << shift;

    /* ���������� ������������ �������� */
    for (i = n - 1; i > 0; i--)
        td[i] = (b[i] << shift) | ((bignum_double_t)b[i - 1] >> (BIGNUM_DIGIT_BITS - shift));
    td[0] = b[0] << shift;

    for (j = digits - n; j >= 0; j--) { /* ������� ���� */
      /* ��������� ������ j-�� ������� �������� � ��������������� ������� */
        qhat = (((bignum_double_t)tr[j + n] << BIGNUM_DIGIT_BITS) | tr[j + n - 1]) / td[n - 1];
        rhat = (((bignum_double_t)tr[j + n] << BIGNUM_DIGIT_BITS) | tr[j + n - 1]) - qhat * td[n - 1];

        while ((qhat >= ((bignum_double_t)1 << BIGNUM_DIGIT_BITS)) ||
            (qhat * td[n - 2] > ((rhat << BIGNUM_DIGIT_BITS) | tr[j + n - 2]))) {
            qhat--;
            rhat += td[n - 1];
            if (rhat >= ((bignum_double_t)1 << BIGNUM_DIGIT_BITS))
                break;
        }

        carry = 0; /* ��������� � ��������� */
        for (i = 0; i < n; i++) {
            tr[i + j] = temp = tr[i + j] - carry - ((product = qhat * td[i]) & BIGNUM_DIGIT_MASK);
            carry = (product >> BIGNUM_DIGIT_BITS) - (temp >> BIGNUM_DIGIT_BITS);
        }

        tr[j + n] = temp = tr[j + n] - carry;
        tq[j] = qhat; /* ��������� ������ �������� */
        if (temp < 0) {
            /* ����� ������� ����� - ���������� �����. ��-�� ����� ��������� t ������
            ����� �������� ���. ������ ������� ���������� ������ ����� -
            ������ ����� ���� � ����������� */
            tq[j]--; carry = 0;
            for (i = 0; i < n; i++) {
                /* �������������� ����� ����� ���������� ��� amd64 */
                tr[i + j] = temp = (bignum_double_t)tr[i + j] + td[i] + carry;
                carry = temp >> BIGNUM_DIGIT_BITS;
            }
            tr[j + n] += carry;
        }
    } /* ����� �������� ����� */

    if (q) /* ��������� ������� */
        bignum_cpy(q, tq, digits, BIGNUM_MAX_DIGITS * 2);

    if (r) { /* ������������� ������� � ��������� ��� */
        bignum_setzero(r, digits);
        for (i = 0; i < n; i++)
            r[i] = (tr[i] >> shift) | ((bignum_double_t)tr[i + 1] << (BIGNUM_DIGIT_BITS - shift));
    }
}

/* ����� ����� a � b � ���� ������� �� ������ m */
void bignum_madd(bignum_digit_t* a, bignum_digit_t* b,
    bignum_digit_t* m, int digits) {
    bignum_digit_t ta[BIGNUM_MAX_DIGITS + 1];
    bignum_digit_t tb[BIGNUM_MAX_DIGITS + 1];
    bignum_digit_t tm[BIGNUM_MAX_DIGITS + 1];

    bignum_cpy(ta, a, digits + 1, digits);
    bignum_cpy(tb, b, digits + 1, digits);
    bignum_cpy(tm, m, digits + 1, digits);

    bignum_add(ta, tb, digits + 1);
    if (bignum_cmp(ta, tm, digits + 1) >= 0) /* a >= m */
        bignum_sub(ta, tm, digits + 1);

    bignum_cpy(a, ta, digits, digits + 1);
}

/* ����� �������� a � b � ���� ������� �� ������ m */
void bignum_msub(bignum_digit_t* a, bignum_digit_t* b,
    bignum_digit_t* m, int digits) {
    /* ����� ����� ���������� ������������� ������������,
    �� ��� ��� ��������� �� �������� */
    if (bignum_cmp(a, b, digits) < 0) /* a < b */
        bignum_add(a, m, digits);
    bignum_sub(a, b, digits);
}

/* ����� ������������ a �� b � ���� ������� �� ������ m */
void bignum_mmul(bignum_digit_t* a, bignum_digit_t* b,
    bignum_digit_t* m, int digits) {
    bignum_digit_t ta[BIGNUM_MAX_DIGITS * 2];
    bignum_digit_t tm[BIGNUM_MAX_DIGITS * 2];

    bignum_cpy(tm, m, digits << 1, digits);
    bignum_mul_raw(a, b, ta, digits);

    if (bignum_cmp(ta, tm, digits << 1) >= 0) /* a >= m */
        bignum_div(ta, tm, 0, ta, digits << 1);

    bignum_cpy(a, ta, digits, digits << 1);
}

/* ����� �������, ���������������� �������� � num � ���� ������� �� ������ m */
void bignum_inv(bignum_digit_t* num, bignum_digit_t* m, int digits) {
    /* r, q, d, d1 = 1, d2 = 0, u = m, v = num; */
    bignum_digit_t r[BIGNUM_MAX_DIGITS]; bignum_digit_t q[BIGNUM_MAX_DIGITS];
    bignum_digit_t d[BIGNUM_MAX_DIGITS]; bignum_digit_t d1[BIGNUM_MAX_DIGITS];
    bignum_digit_t d2[BIGNUM_MAX_DIGITS]; bignum_digit_t u[BIGNUM_MAX_DIGITS];
    bignum_digit_t* pu = u, * pv = num, * pr = r, * pd = d, * pd1 = d1, * pd2 = d2, * pt;

    bignum_cpy(u, m, digits, digits);
    bignum_setzero(d2, digits);
    bignum_setzero(d1, digits);
    d1[0]++;

    while (!bignum_iszero(pv, digits)) { /* while(v != 0) */
      /* r = u % v; q = (u - r) / v; */
        bignum_div(pu, pv, q, pr, digits);

        /* d = d2 - q*d1 (mod m) */
        bignum_mmul(q, pd1, m, digits);
        bignum_cpy(pd, pd2, digits, digits);
        bignum_msub(pd, q, m, digits);

        /* u = v; v = r; d2 = d1; d1 = d; */
        pt = pu; pu = pv; pv = pr; pr = pt;
        pt = pd2; pd2 = pd1; pd1 = pd; pd = pt;
    }

    /* ���� u = 1, �� d2 - �����, �������� num � ������ ������� �� ������ m
    ����� - ��������� �������� �� ���������� */
    if (pd2 != num) bignum_cpy(num, pd2, digits, digits);
}

/* ����� ������� a � b � ���� ������� �� ������ m */
void bignum_mdiv(bignum_digit_t* a, /* ������� */
    bignum_digit_t* b, /* �������� */
    bignum_digit_t* m, /* ������ */
    int digits) { /* ���-�� �������� � ������ */
    bignum_digit_t binv[BIGNUM_MAX_DIGITS];

    /* ������� b ^ -1 - �������, ���������������� �������� � b */
    bignum_cpy(binv, b, digits, digits);
    bignum_inv(binv, m, digits);

    /* �������� a �� (b ^ -1) */
    bignum_mmul(a, binv, m, digits);
}