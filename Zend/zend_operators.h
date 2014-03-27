/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2014 Zend Technologies Ltd. (http://www.zend.com) |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef ZEND_OPERATORS_H
#define ZEND_OPERATORS_H

#include <errno.h>
#include <math.h>
#include <assert.h>

#ifdef __GNUC__
#include <stddef.h>
#endif

#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif

#include "zend_strtod.h"
#include "zend_multiply.h"

#if 0&&HAVE_BCMATH
#include "ext/bcmath/libbcmath/src/bcmath.h"
#endif

#define LONG_SIGN_MASK (1L << (8*sizeof(long)-1))

BEGIN_EXTERN_C()
ZEND_API int add_function(zval *result, zval *op1, zval *op2 TSRMLS_DC);
ZEND_API int sub_function(zval *result, zval *op1, zval *op2 TSRMLS_DC);
ZEND_API int mul_function(zval *result, zval *op1, zval *op2 TSRMLS_DC);
ZEND_API int div_function(zval *result, zval *op1, zval *op2 TSRMLS_DC);
ZEND_API int mod_function(zval *result, zval *op1, zval *op2 TSRMLS_DC);
ZEND_API int boolean_xor_function(zval *result, zval *op1, zval *op2 TSRMLS_DC);
ZEND_API int boolean_not_function(zval *result, zval *op1 TSRMLS_DC);
ZEND_API int bitwise_not_function(zval *result, zval *op1 TSRMLS_DC);
ZEND_API int bitwise_or_function(zval *result, zval *op1, zval *op2 TSRMLS_DC);
ZEND_API int bitwise_and_function(zval *result, zval *op1, zval *op2 TSRMLS_DC);
ZEND_API int bitwise_xor_function(zval *result, zval *op1, zval *op2 TSRMLS_DC);
ZEND_API int shift_left_function(zval *result, zval *op1, zval *op2 TSRMLS_DC);
ZEND_API int shift_right_function(zval *result, zval *op1, zval *op2 TSRMLS_DC);
ZEND_API int concat_function(zval *result, zval *op1, zval *op2 TSRMLS_DC);

ZEND_API int is_equal_function(zval *result, zval *op1, zval *op2 TSRMLS_DC);
ZEND_API int is_identical_function(zval *result, zval *op1, zval *op2 TSRMLS_DC);
ZEND_API int is_not_identical_function(zval *result, zval *op1, zval *op2 TSRMLS_DC);
ZEND_API int is_not_equal_function(zval *result, zval *op1, zval *op2 TSRMLS_DC);
ZEND_API int is_smaller_function(zval *result, zval *op1, zval *op2 TSRMLS_DC);
ZEND_API int is_smaller_or_equal_function(zval *result, zval *op1, zval *op2 TSRMLS_DC);

ZEND_API zend_bool instanceof_function_ex(const zend_class_entry *instance_ce, const zend_class_entry *ce, zend_bool interfaces_only TSRMLS_DC);
ZEND_API zend_bool instanceof_function(const zend_class_entry *instance_ce, const zend_class_entry *ce TSRMLS_DC);
END_EXTERN_C()

#if ZEND_DVAL_TO_LVAL_CAST_OK
# define zend_dval_to_lval(d) ((long) (d))
#elif SIZEOF_LONG == 4
static zend_always_inline long zend_dval_to_lval(double d)
{
	if (d > LONG_MAX || d < LONG_MIN) {
		double	two_pow_32 = pow(2., 32.),
				dmod;

		dmod = fmod(d, two_pow_32);
		if (dmod < 0) {
			/* we're going to make this number positive; call ceil()
			 * to simulate rounding towards 0 of the negative number */
			dmod = ceil(dmod) + two_pow_32;
		}
		return (long)(unsigned long)dmod;
	}
	return (long)d;
}
#else
static zend_always_inline long zend_dval_to_lval(double d)
{
	/* >= as (double)LONG_MAX is outside signed range */
	if (d >= LONG_MAX || d < LONG_MIN) {
		double	two_pow_64 = pow(2., 64.),
				dmod;

		dmod = fmod(d, two_pow_64);
		if (dmod < 0) {
			/* no need to call ceil; original double must have had no
			 * fractional part, hence dmod does not have one either */
			dmod += two_pow_64;
		}
		return (long)(unsigned long)dmod;
	}
	return (long)d;
}
#endif
/* }}} */

#define ZEND_IS_DIGIT(c) ((c) >= '0' && (c) <= '9')
#define ZEND_IS_XDIGIT(c) (((c) >= 'A' && (c) <= 'F') || ((c) >= 'a' && (c) <= 'f'))

/**
 * Checks whether the string "str" with length "length" is numeric. The value
 * of allow_errors determines whether it's required to be entirely numeric, or
 * just its prefix. Leading whitespace is allowed.
 *
 * The function returns 0 if the string did not contain a valid number; IS_LONG
 * if it contained a number that fits within the range of a long; or IS_DOUBLE
 * if the number was out of long range or contained a decimal point/exponent.
 * The number's value is returned into the respective pointer, *lval or *dval,
 * if that pointer is not NULL.
 *
 * This variant also gives information if a string that represents an integer
 * could not be represented as such due to overflow. It writes 1 to oflow_info
 * if the integer is larger than LONG_MAX and -1 if it's smaller than LONG_MIN.
 */
static inline zend_uchar is_numeric_string_ex(const char *str, int length, long *lval, double *dval, int allow_errors, int *oflow_info)
{
	const char *ptr;
	int base = 10, digits = 0, dp_or_e = 0;
	double local_dval = 0.0;
	zend_uchar type;

	if (!length) {
		return 0;
	}

	if (oflow_info != NULL) {
		*oflow_info = 0;
	}

	/* Skip any whitespace
	 * This is much faster than the isspace() function */
	while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r' || *str == '\v' || *str == '\f') {
		str++;
		length--;
	}
	ptr = str;

	if (*ptr == '-' || *ptr == '+') {
		ptr++;
	}

	if (ZEND_IS_DIGIT(*ptr)) {
		/* Handle hex numbers
		 * str is used instead of ptr to disallow signs and keep old behavior */
		if (length > 2 && *str == '0' && (str[1] == 'x' || str[1] == 'X')) {
			base = 16;
			ptr += 2;
		}

		/* Skip any leading 0s */
		while (*ptr == '0') {
			ptr++;
		}

		/* Count the number of digits. If a decimal point/exponent is found,
		 * it's a double. Otherwise, if there's a dval or no need to check for
		 * a full match, stop when there are too many digits for a long */
		for (type = IS_LONG; !(digits >= MAX_LENGTH_OF_LONG && (dval || allow_errors == 1)); digits++, ptr++) {
check_digits:
			if (ZEND_IS_DIGIT(*ptr) || (base == 16 && ZEND_IS_XDIGIT(*ptr))) {
				continue;
			} else if (base == 10) {
				if (*ptr == '.' && dp_or_e < 1) {
					goto process_double;
				} else if ((*ptr == 'e' || *ptr == 'E') && dp_or_e < 2) {
					const char *e = ptr + 1;

					if (*e == '-' || *e == '+') {
						ptr = e++;
					}
					if (ZEND_IS_DIGIT(*e)) {
						goto process_double;
					}
				}
			}

			break;
		}

		if (base == 10) {
			if (digits >= MAX_LENGTH_OF_LONG) {
				if (oflow_info != NULL) {
					*oflow_info = *str == '-' ? -1 : 1;
				}
				dp_or_e = -1;
				goto process_double;
			}
		} else if (!(digits < SIZEOF_LONG * 2 || (digits == SIZEOF_LONG * 2 && ptr[-digits] <= '7'))) {
			if (dval) {
				local_dval = zend_hex_strtod(str, &ptr);
			}
			if (oflow_info != NULL) {
				*oflow_info = 1;
			}
			type = IS_DOUBLE;
		}
	} else if (*ptr == '.' && ZEND_IS_DIGIT(ptr[1])) {
process_double:
		type = IS_DOUBLE;

		/* If there's a dval, do the conversion; else continue checking
		 * the digits if we need to check for a full match */
		if (dval) {
			local_dval = zend_strtod(str, &ptr);
		} else if (allow_errors != 1 && dp_or_e != -1) {
			dp_or_e = (*ptr++ == '.') ? 1 : 2;
			goto check_digits;
		}
	} else {
		return 0;
	}

	if (ptr != str + length) {
		if (!allow_errors) {
			return 0;
		}
		if (allow_errors == -1) {
			zend_error(E_NOTICE, "A non well formed numeric value encountered");
		}
	}

	if (type == IS_LONG) {
		if (digits == MAX_LENGTH_OF_LONG - 1) {
			int cmp = strcmp(&ptr[-digits], long_min_digits);

			if (!(cmp < 0 || (cmp == 0 && *str == '-'))) {
				if (dval) {
					*dval = zend_strtod(str, NULL);
				}
				if (oflow_info != NULL) {
					*oflow_info = *str == '-' ? -1 : 1;
				}

				return IS_DOUBLE;
			}
		}

		if (lval) {
			*lval = strtol(str, NULL, base);
		}

		return IS_LONG;
	} else {
		if (dval) {
			*dval = local_dval;
		}

		return IS_DOUBLE;
	}
}

static inline zend_uchar is_numeric_string(const char *str, int length, long *lval, double *dval, int allow_errors) {
    return is_numeric_string_ex(str, length, lval, dval, allow_errors, NULL);
}

static inline const char *
zend_memnstr(const char *haystack, const char *needle, int needle_len, char *end)
{
	const char *p = haystack;
	const char ne = needle[needle_len-1];

	if (needle_len == 1) {
		return (char *)memchr(p, *needle, (end-p));
	}

	if (needle_len > end-haystack) {
		return NULL;
	}

	end -= needle_len;

	while (p <= end) {
		if ((p = (char *)memchr(p, *needle, (end-p+1))) && ne == p[needle_len-1]) {
			if (!memcmp(needle, p, needle_len-1)) {
				return p;
			}
		}

		if (p == NULL) {
			return NULL;
		}

		p++;
	}

	return NULL;
}

static inline const void *zend_memrchr(const void *s, int c, size_t n)
{
	register const unsigned char *e;

	if (n <= 0) {
		return NULL;
	}

	for (e = (const unsigned char *)s + n - 1; e >= (const unsigned char *)s; e--) {
		if (*e == (const unsigned char)c) {
			return (const void *)e;
		}
	}

	return NULL;
}

BEGIN_EXTERN_C()
ZEND_API int increment_function(zval *op1);
ZEND_API int decrement_function(zval *op2);

ZEND_API void convert_scalar_to_number(zval *op TSRMLS_DC);
ZEND_API void _convert_to_cstring(zval *op ZEND_FILE_LINE_DC);
ZEND_API void _convert_to_string(zval *op ZEND_FILE_LINE_DC);
ZEND_API void convert_to_long(zval *op);
ZEND_API void convert_to_double(zval *op);
ZEND_API void convert_to_long_base(zval *op, int base);
ZEND_API void convert_to_null(zval *op);
ZEND_API void convert_to_boolean(zval *op);
ZEND_API void convert_to_array(zval *op);
ZEND_API void convert_to_object(zval *op);
ZEND_API void multi_convert_to_long_ex(int argc, ...);
ZEND_API void multi_convert_to_double_ex(int argc, ...);
ZEND_API void multi_convert_to_string_ex(int argc, ...);
ZEND_API int add_char_to_string(zval *result, const zval *op1, const zval *op2);
ZEND_API int add_string_to_string(zval *result, const zval *op1, const zval *op2);
#define convert_to_cstring(op) if ((op)->type != IS_STRING) { _convert_to_cstring((op) ZEND_FILE_LINE_CC); }
#define convert_to_string(op) if ((op)->type != IS_STRING) { _convert_to_string((op) ZEND_FILE_LINE_CC); }

ZEND_API double zend_string_to_double(const char *number, zend_uint length);

ZEND_API int zval_is_true(zval *op);
ZEND_API int compare_function(zval *result, zval *op1, zval *op2 TSRMLS_DC);
ZEND_API int numeric_compare_function(zval *result, zval *op1, zval *op2 TSRMLS_DC);
ZEND_API int string_compare_function_ex(zval *result, zval *op1, zval *op2, zend_bool case_insensitive TSRMLS_DC);
ZEND_API int string_compare_function(zval *result, zval *op1, zval *op2 TSRMLS_DC);
ZEND_API int string_case_compare_function(zval *result, zval *op1, zval *op2 TSRMLS_DC);
#if HAVE_STRCOLL
ZEND_API int string_locale_compare_function(zval *result, zval *op1, zval *op2 TSRMLS_DC);
#endif

ZEND_API void zend_str_tolower(char *str, unsigned int length);
ZEND_API char *zend_str_tolower_copy(char *dest, const char *source, unsigned int length);
ZEND_API char *zend_str_tolower_dup(const char *source, unsigned int length);

ZEND_API int zend_binary_zval_strcmp(zval *s1, zval *s2);
ZEND_API int zend_binary_zval_strncmp(zval *s1, zval *s2, zval *s3);
ZEND_API int zend_binary_zval_strcasecmp(zval *s1, zval *s2);
ZEND_API int zend_binary_zval_strncasecmp(zval *s1, zval *s2, zval *s3);
ZEND_API int zend_binary_strcmp(const char *s1, uint len1, const char *s2, uint len2);
ZEND_API int zend_binary_strncmp(const char *s1, uint len1, const char *s2, uint len2, uint length);
ZEND_API int zend_binary_strcasecmp(const char *s1, uint len1, const char *s2, uint len2);
ZEND_API int zend_binary_strncasecmp(const char *s1, uint len1, const char *s2, uint len2, uint length);
ZEND_API int zend_binary_strncasecmp_l(const char *s1, uint len1, const char *s2, uint len2, uint length);

ZEND_API void zendi_smart_strcmp(zval *result, zval *s1, zval *s2);
ZEND_API void zend_compare_symbol_tables(zval *result, HashTable *ht1, HashTable *ht2 TSRMLS_DC);
ZEND_API void zend_compare_arrays(zval *result, zval *a1, zval *a2 TSRMLS_DC);
ZEND_API void zend_compare_objects(zval *result, zval *o1, zval *o2 TSRMLS_DC);

ZEND_API int zend_atoi(const char *str, int str_len);
ZEND_API long zend_atol(const char *str, int str_len);

ZEND_API void zend_locale_sprintf_double(zval *op ZEND_FILE_LINE_DC);
END_EXTERN_C()

#define convert_to_ex_master(pzv, lower_type, upper_type)	\
	if (Z_TYPE_P(pzv)!=IS_##upper_type) {					\
		SEPARATE_ZVAL_IF_NOT_REF(pzv);						\
		convert_to_##lower_type(pzv);						\
	}

#define convert_to_explicit_type(pzv, type)		\
	do {										\
		switch (type) {							\
			case IS_NULL:						\
				convert_to_null(pzv);			\
				break;							\
			case IS_LONG:						\
				convert_to_long(pzv);			\
				break;							\
			case IS_DOUBLE:						\
				convert_to_double(pzv);			\
				break;							\
			case IS_BOOL:						\
				convert_to_boolean(pzv);		\
				break;							\
			case IS_ARRAY:						\
				convert_to_array(pzv);			\
				break;							\
			case IS_OBJECT:						\
				convert_to_object(pzv);			\
				break;							\
			case IS_STRING:						\
				convert_to_string(pzv);			\
				break;							\
			default:							\
				assert(0);						\
				break;							\
		}										\
	} while (0);

#define convert_to_explicit_type_ex(pzv, str_type)	\
	if (Z_TYPE_P(pzv) != str_type) {				\
		SEPARATE_ZVAL_IF_NOT_REF(pzv);				\
		convert_to_explicit_type(pzv, str_type);	\
	}

#define convert_to_boolean_ex(pzv)	convert_to_ex_master(pzv, boolean, BOOL)
#define convert_to_long_ex(pzv)		convert_to_ex_master(pzv, long, LONG)
#define convert_to_double_ex(pzv)	convert_to_ex_master(pzv, double, DOUBLE)
#define convert_to_string_ex(pzv)	convert_to_ex_master(pzv, string, STRING)
#define convert_to_array_ex(pzv)	convert_to_ex_master(pzv, array, ARRAY)
#define convert_to_object_ex(pzv)	convert_to_ex_master(pzv, object, OBJECT)
#define convert_to_null_ex(pzv)		convert_to_ex_master(pzv, null, NULL)

#define convert_scalar_to_number_ex(pzv)							\
	if (Z_TYPE_P(pzv)!=IS_LONG && Z_TYPE_P(pzv)!=IS_DOUBLE) {		\
		SEPARATE_ZVAL_IF_NOT_REF(pzv);								\
		convert_scalar_to_number(pzv TSRMLS_CC);					\
	}

#if HAVE_SETLOCALE && defined(ZEND_WIN32) && !defined(ZTS) && defined(_MSC_VER) && (_MSC_VER >= 1400)
/* This is performance improvement of tolower() on Windows and VC2005
 * Gives 10-18% on bench.php
 */
#define ZEND_USE_TOLOWER_L 1
#endif

#ifdef ZEND_USE_TOLOWER_L
ZEND_API void zend_update_current_locale(void);
#else
#define zend_update_current_locale()
#endif

/* The offset in bytes between the value and type fields of a zval */
#define ZVAL_OFFSETOF_TYPE	\
	(offsetof(zval,type) - offsetof(zval,value))

static zend_always_inline int fast_increment_function(zval *op1)
{
	if (EXPECTED(Z_TYPE_P(op1) == IS_LONG)) {
#if defined(__GNUC__) && defined(__i386__)
		__asm__(
			"incl (%0)\n\t"
			"jno  0f\n\t"
			"movl $0x0, (%0)\n\t"
			"movl $0x41e00000, 0x4(%0)\n\t"
			"movb %1, %c2(%0)\n"
			"0:"
			:
			: "r"(&op1->value),
			  "n"(IS_DOUBLE),
			  "n"(ZVAL_OFFSETOF_TYPE)
			: "cc");
#elif defined(__GNUC__) && defined(__x86_64__)
		__asm__(
			"incq (%0)\n\t"
			"jno  0f\n\t"
			"movl $0x0, (%0)\n\t"
			"movl $0x43e00000, 0x4(%0)\n\t"
			"movb %1, %c2(%0)\n"
			"0:"
			:
			: "r"(&op1->value),
			  "n"(IS_DOUBLE),
			  "n"(ZVAL_OFFSETOF_TYPE)
			: "cc");
#else
		if (UNEXPECTED(Z_LVAL_P(op1) == LONG_MAX)) {
			/* switch to double */
			Z_DVAL_P(op1) = (double)LONG_MAX + 1.0;
			Z_TYPE_P(op1) = IS_DOUBLE;
		} else {
			Z_LVAL_P(op1)++;
		}
#endif
		return SUCCESS;
	}
	return increment_function(op1);
}

static zend_always_inline int fast_decrement_function(zval *op1)
{
	if (EXPECTED(Z_TYPE_P(op1) == IS_LONG)) {
#if defined(__GNUC__) && defined(__i386__)
		__asm__(
			"decl (%0)\n\t"
			"jno  0f\n\t"
			"movl $0x00200000, (%0)\n\t"
			"movl $0xc1e00000, 0x4(%0)\n\t"
			"movb %1,%c2(%0)\n"
			"0:"
			:
			: "r"(&op1->value),
			  "n"(IS_DOUBLE),
			  "n"(ZVAL_OFFSETOF_TYPE)
			: "cc");
#elif defined(__GNUC__) && defined(__x86_64__)
		__asm__(
			"decq (%0)\n\t"
			"jno  0f\n\t"
			"movl $0x00000000, (%0)\n\t"
			"movl $0xc3e00000, 0x4(%0)\n\t"
			"movb %1,%c2(%0)\n"
			"0:"
			:
			: "r"(&op1->value),
			  "n"(IS_DOUBLE),
			  "n"(ZVAL_OFFSETOF_TYPE)
			: "cc");
#else
		if (UNEXPECTED(Z_LVAL_P(op1) == LONG_MIN)) {
			/* switch to double */
			Z_DVAL_P(op1) = (double)LONG_MIN - 1.0;
			Z_TYPE_P(op1) = IS_DOUBLE;
		} else {
			Z_LVAL_P(op1)--;
		}
#endif
		return SUCCESS;
	}
	return decrement_function(op1);
}

static zend_always_inline int fast_add_function(zval *result, zval *op1, zval *op2 TSRMLS_DC)
{
	if (EXPECTED(Z_TYPE_P(op1) == IS_LONG)) {
		if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
#if defined(__GNUC__) && defined(__i386__)
		__asm__(
			"movl	(%1), %%eax\n\t"
			"addl   (%2), %%eax\n\t"
			"jo     0f\n\t"     
			"movl   %%eax, (%0)\n\t"
			"movb   %3, %c5(%0)\n\t"
			"jmp    1f\n"
			"0:\n\t"
			"fildl	(%1)\n\t"
			"fildl	(%2)\n\t"
			"faddp	%%st, %%st(1)\n\t"
			"movb   %4, %c5(%0)\n\t"
			"fstpl	(%0)\n"
			"1:"
			: 
			: "r"(&result->value),
			  "r"(&op1->value),
			  "r"(&op2->value),
			  "n"(IS_LONG),
			  "n"(IS_DOUBLE),
			  "n"(ZVAL_OFFSETOF_TYPE)
			: "eax","cc");
#elif defined(__GNUC__) && defined(__x86_64__)
		__asm__(
			"movq	(%1), %%rax\n\t"
			"addq   (%2), %%rax\n\t"
			"jo     0f\n\t"     
			"movq   %%rax, (%0)\n\t"
			"movb   %3, %c5(%0)\n\t"
			"jmp    1f\n"
			"0:\n\t"
			"fildq	(%1)\n\t"
			"fildq	(%2)\n\t"
			"faddp	%%st, %%st(1)\n\t"
			"movb   %4, %c5(%0)\n\t"
			"fstpl	(%0)\n"
			"1:"
			: 
			: "r"(&result->value),
			  "r"(&op1->value),
			  "r"(&op2->value),
			  "n"(IS_LONG),
			  "n"(IS_DOUBLE),
			  "n"(ZVAL_OFFSETOF_TYPE)
			: "rax","cc");
#else
			/*
			 * 'result' may alias with op1 or op2, so we need to
			 * ensure that 'result' is not updated until after we
			 * have read the values of op1 and op2.
			 */

			if (UNEXPECTED((Z_LVAL_P(op1) & LONG_SIGN_MASK) == (Z_LVAL_P(op2) & LONG_SIGN_MASK)
				&& (Z_LVAL_P(op1) & LONG_SIGN_MASK) != ((Z_LVAL_P(op1) + Z_LVAL_P(op2)) & LONG_SIGN_MASK))) {
				Z_DVAL_P(result) = (double) Z_LVAL_P(op1) + (double) Z_LVAL_P(op2);
				Z_TYPE_P(result) = IS_DOUBLE;
			} else {
				Z_LVAL_P(result) = Z_LVAL_P(op1) + Z_LVAL_P(op2);
				Z_TYPE_P(result) = IS_LONG;
			}
#endif
			return SUCCESS;
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			Z_DVAL_P(result) = ((double)Z_LVAL_P(op1)) + Z_DVAL_P(op2);
			Z_TYPE_P(result) = IS_DOUBLE;
			return SUCCESS;
		}
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_DOUBLE)) {
		if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			Z_DVAL_P(result) = Z_DVAL_P(op1) + Z_DVAL_P(op2);
			Z_TYPE_P(result) = IS_DOUBLE;
			return SUCCESS;
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			Z_DVAL_P(result) = Z_DVAL_P(op1) + ((double)Z_LVAL_P(op2));
			Z_TYPE_P(result) = IS_DOUBLE;
			return SUCCESS;
		}
	}
	return add_function(result, op1, op2 TSRMLS_CC);
}

static zend_always_inline int fast_sub_function(zval *result, zval *op1, zval *op2 TSRMLS_DC)
{
	if (EXPECTED(Z_TYPE_P(op1) == IS_LONG)) {
		if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
#if defined(__GNUC__) && defined(__i386__)
		__asm__(
			"movl	(%1), %%eax\n\t"
			"subl   (%2), %%eax\n\t"
			"jo     0f\n\t"     
			"movl   %%eax, (%0)\n\t"
			"movb   %3, %c5(%0)\n\t"
			"jmp    1f\n"
			"0:\n\t"
			"fildl	(%2)\n\t"
			"fildl	(%1)\n\t"
#if defined(__clang__) && (__clang_major__ < 2 || (__clang_major__ == 2 && __clang_minor__ < 10))
			"fsubp  %%st(1), %%st\n\t"  /* LLVM bug #9164 */
#else
			"fsubp	%%st, %%st(1)\n\t"
#endif
			"movb   %4, %c5(%0)\n\t"
			"fstpl	(%0)\n"
			"1:"
			: 
			: "r"(&result->value),
			  "r"(&op1->value),
			  "r"(&op2->value),
			  "n"(IS_LONG),
			  "n"(IS_DOUBLE),
			  "n"(ZVAL_OFFSETOF_TYPE)
			: "eax","cc");
#elif defined(__GNUC__) && defined(__x86_64__)
		__asm__(
			"movq	(%1), %%rax\n\t"
			"subq   (%2), %%rax\n\t"
			"jo     0f\n\t"     
			"movq   %%rax, (%0)\n\t"
			"movb   %3, %c5(%0)\n\t"
			"jmp    1f\n"
			"0:\n\t"
			"fildq	(%2)\n\t"
			"fildq	(%1)\n\t"
#if defined(__clang__) && (__clang_major__ < 2 || (__clang_major__ == 2 && __clang_minor__ < 10))
			"fsubp  %%st(1), %%st\n\t"  /* LLVM bug #9164 */
#else
			"fsubp	%%st, %%st(1)\n\t"
#endif
			"movb   %4, %c5(%0)\n\t"
			"fstpl	(%0)\n"
			"1:"
			: 
			: "r"(&result->value),
			  "r"(&op1->value),
			  "r"(&op2->value),
			  "n"(IS_LONG),
			  "n"(IS_DOUBLE),
			  "n"(ZVAL_OFFSETOF_TYPE)
			: "rax","cc");
#else
			Z_LVAL_P(result) = Z_LVAL_P(op1) - Z_LVAL_P(op2);

			if (UNEXPECTED((Z_LVAL_P(op1) & LONG_SIGN_MASK) != (Z_LVAL_P(op2) & LONG_SIGN_MASK)
				&& (Z_LVAL_P(op1) & LONG_SIGN_MASK) != (Z_LVAL_P(result) & LONG_SIGN_MASK))) {
				Z_DVAL_P(result) = (double) Z_LVAL_P(op1) - (double) Z_LVAL_P(op2);
				Z_TYPE_P(result) = IS_DOUBLE;
			} else {
				Z_TYPE_P(result) = IS_LONG;
			}
#endif
			return SUCCESS;
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			Z_DVAL_P(result) = ((double)Z_LVAL_P(op1)) - Z_DVAL_P(op2);
			Z_TYPE_P(result) = IS_DOUBLE;
			return SUCCESS;
		}
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_DOUBLE)) {
		if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			Z_DVAL_P(result) = Z_DVAL_P(op1) - Z_DVAL_P(op2);
			Z_TYPE_P(result) = IS_DOUBLE;
			return SUCCESS;
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			Z_DVAL_P(result) = Z_DVAL_P(op1) - ((double)Z_LVAL_P(op2));
			Z_TYPE_P(result) = IS_DOUBLE;
			return SUCCESS;
		}
	}
	return sub_function(result, op1, op2 TSRMLS_CC);
}

static zend_always_inline int fast_mul_function(zval *result, zval *op1, zval *op2 TSRMLS_DC)
{
	if (EXPECTED(Z_TYPE_P(op1) == IS_LONG)) {
		if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			long overflow;

			ZEND_SIGNED_MULTIPLY_LONG(Z_LVAL_P(op1), Z_LVAL_P(op2), Z_LVAL_P(result), Z_DVAL_P(result), overflow);
			Z_TYPE_P(result) = overflow ? IS_DOUBLE : IS_LONG;
			return SUCCESS;
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			Z_DVAL_P(result) = ((double)Z_LVAL_P(op1)) * Z_DVAL_P(op2);
			Z_TYPE_P(result) = IS_DOUBLE;
			return SUCCESS;
		}
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_DOUBLE)) {
		if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			Z_DVAL_P(result) = Z_DVAL_P(op1) * Z_DVAL_P(op2);
			Z_TYPE_P(result) = IS_DOUBLE;
			return SUCCESS;
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			Z_DVAL_P(result) = Z_DVAL_P(op1) * ((double)Z_LVAL_P(op2));
			Z_TYPE_P(result) = IS_DOUBLE;
			return SUCCESS;
		}
	}
	return mul_function(result, op1, op2 TSRMLS_CC);
}

static zend_always_inline int fast_div_function(zval *result, zval *op1, zval *op2 TSRMLS_DC)
{
#if 0
	if (EXPECTED(Z_TYPE_P(op1) == IS_LONG) && 0) {
		if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			if (UNEXPECTED(Z_LVAL_P(op2) == 0)) {
				zend_error(E_WARNING, "Division by zero");
				Z_LVAL_P(result) = 0;
				Z_TYPE_P(result) = IS_BOOL;
				return FAILURE;
			} else if (UNEXPECTED(Z_LVAL_P(op2) == -1 && Z_LVAL_P(op1) == LONG_MIN)) {
				/* Prevent overflow error/crash */
				Z_DVAL_P(result) = (double) LONG_MIN / -1;
				Z_TYPE_P(result) = IS_DOUBLE;
			} else if (EXPECTED(Z_LVAL_P(op1) % Z_LVAL_P(op2) == 0)) {
				/* integer */
				Z_LVAL_P(result) = Z_LVAL_P(op1) / Z_LVAL_P(op2);
				Z_TYPE_P(result) = IS_LONG;
			} else {
				Z_DVAL_P(result) = ((double) Z_LVAL_P(op1)) / ((double)Z_LVAL_P(op2));
				Z_TYPE_P(result) = IS_DOUBLE;
			}
			return SUCCESS;
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			if (UNEXPECTED(Z_DVAL_P(op2) == 0)) {
				zend_error(E_WARNING, "Division by zero");
				Z_LVAL_P(result) = 0;
				Z_TYPE_P(result) = IS_BOOL;
				return FAILURE;
			}
			Z_DVAL_P(result) = ((double)Z_LVAL_P(op1)) / Z_DVAL_P(op2);
			Z_TYPE_P(result) = IS_DOUBLE;
			return SUCCESS;
		}
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_DOUBLE) && 0) {
		if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			if (UNEXPECTED(Z_DVAL_P(op2) == 0)) {
				zend_error(E_WARNING, "Division by zero");
				Z_LVAL_P(result) = 0;
				Z_TYPE_P(result) = IS_BOOL;
				return FAILURE;
			}
			Z_DVAL_P(result) = Z_DVAL_P(op1) / Z_DVAL_P(op2);
			Z_TYPE_P(result) = IS_DOUBLE;
			return SUCCESS;
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			if (UNEXPECTED(Z_LVAL_P(op2) == 0)) {
				zend_error(E_WARNING, "Division by zero");
				Z_LVAL_P(result) = 0;
				Z_TYPE_P(result) = IS_BOOL;
				return FAILURE;
			}
			Z_DVAL_P(result) = Z_DVAL_P(op1) / ((double)Z_LVAL_P(op2));
			Z_TYPE_P(result) = IS_DOUBLE;
			return SUCCESS;
		}
	}
#endif
	return div_function(result, op1, op2 TSRMLS_CC);
}

static zend_always_inline int fast_mod_function(zval *result, zval *op1, zval *op2 TSRMLS_DC)
{
	if (EXPECTED(Z_TYPE_P(op1) == IS_LONG)) {
		if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			if (UNEXPECTED(Z_LVAL_P(op2) == 0)) {
				zend_error(E_WARNING, "Division by zero");
				Z_LVAL_P(result) = 0;
				Z_TYPE_P(result) = IS_BOOL;
				return FAILURE;
			} else if (UNEXPECTED(Z_LVAL_P(op2) == -1)) {
				/* Prevent overflow error/crash if op1==LONG_MIN */
				Z_LVAL_P(result) = 0;
				Z_TYPE_P(result) = IS_LONG;
				return SUCCESS;
			}
			Z_LVAL_P(result) = Z_LVAL_P(op1) % Z_LVAL_P(op2);
			Z_TYPE_P(result) = IS_LONG;
			return SUCCESS;
		}
	}
	return mod_function(result, op1, op2 TSRMLS_CC);
}

static zend_always_inline int fast_equal_function(zval *result, zval *op1, zval *op2 TSRMLS_DC)
{
	if (EXPECTED(Z_TYPE_P(op1) == IS_LONG)) {
		if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			return Z_LVAL_P(op1) == Z_LVAL_P(op2);
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			return ((double)Z_LVAL_P(op1)) == Z_DVAL_P(op2);
		}
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_DOUBLE)) {
		if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			return Z_DVAL_P(op1) == Z_DVAL_P(op2);
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			return Z_DVAL_P(op1) == ((double)Z_LVAL_P(op2));
		}
	}
	compare_function(result, op1, op2 TSRMLS_CC);
	return Z_LVAL_P(result) == 0;
}

static zend_always_inline int fast_not_equal_function(zval *result, zval *op1, zval *op2 TSRMLS_DC)
{
	if (EXPECTED(Z_TYPE_P(op1) == IS_LONG)) {
		if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			return Z_LVAL_P(op1) != Z_LVAL_P(op2);
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			return ((double)Z_LVAL_P(op1)) != Z_DVAL_P(op2);
		}
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_DOUBLE)) {
		if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			return Z_DVAL_P(op1) != Z_DVAL_P(op2);
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			return Z_DVAL_P(op1) != ((double)Z_LVAL_P(op2));
		}
	}
	compare_function(result, op1, op2 TSRMLS_CC);
	return Z_LVAL_P(result) != 0;
}

static zend_always_inline int fast_is_smaller_function(zval *result, zval *op1, zval *op2 TSRMLS_DC)
{
	if (EXPECTED(Z_TYPE_P(op1) == IS_LONG)) {
		if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			return Z_LVAL_P(op1) < Z_LVAL_P(op2);
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			return ((double)Z_LVAL_P(op1)) < Z_DVAL_P(op2);
		}
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_DOUBLE)) {
		if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			return Z_DVAL_P(op1) < Z_DVAL_P(op2);
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			return Z_DVAL_P(op1) < ((double)Z_LVAL_P(op2));
		}
	}
	compare_function(result, op1, op2 TSRMLS_CC);
	return Z_LVAL_P(result) < 0;
}

static zend_always_inline int fast_is_smaller_or_equal_function(zval *result, zval *op1, zval *op2 TSRMLS_DC)
{
	if (EXPECTED(Z_TYPE_P(op1) == IS_LONG)) {
		if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			return Z_LVAL_P(op1) <= Z_LVAL_P(op2);
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			return ((double)Z_LVAL_P(op1)) <= Z_DVAL_P(op2);
		}
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_DOUBLE)) {
		if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			return Z_DVAL_P(op1) <= Z_DVAL_P(op2);
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			return Z_DVAL_P(op1) <= ((double)Z_LVAL_P(op2));
		}
	}
	compare_function(result, op1, op2 TSRMLS_CC);
	return Z_LVAL_P(result) <= 0;
}

#define ZEND_TRY_BINARY_OBJECT_OPERATION(opcode)                                                  \
	if (Z_TYPE_P(op1) == IS_OBJECT && Z_OBJ_HANDLER_P(op1, do_operation)) {                       \
		if (SUCCESS == Z_OBJ_HANDLER_P(op1, do_operation)(opcode, result, op1, op2 TSRMLS_CC)) {  \
			return SUCCESS;                                                                       \
		}                                                                                         \
	} else if (Z_TYPE_P(op2) == IS_OBJECT && Z_OBJ_HANDLER_P(op2, do_operation)) {                \
		if (SUCCESS == Z_OBJ_HANDLER_P(op2, do_operation)(opcode, result, op1, op2 TSRMLS_CC)) {  \
			return SUCCESS;                                                                       \
		}                                                                                         \
	}

#define ZEND_TRY_UNARY_OBJECT_OPERATION(opcode)                                                   \
	if (Z_TYPE_P(op1) == IS_OBJECT && Z_OBJ_HANDLER_P(op1, do_operation)                          \
	 && SUCCESS == Z_OBJ_HANDLER_P(op1, do_operation)(opcode, result, op1, NULL TSRMLS_CC)        \
	) {                                                                                           \
		return SUCCESS;                                                                           \
	}

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
