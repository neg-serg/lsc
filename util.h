#ifndef LSC_UTIL_H
#define LSC_UTIL_H

// Die after printing message for errno
noreturn void die_errno(void);

// Die after printing message
noreturn void die(const char *s);

// Die if malloc fails
void *xmalloc(size_t nmemb, size_t size);

// Die if realloc fails
void *xrealloc(void *ptr, size_t nmemb, size_t size);

#undef assert

#define assert(expr) (likely(expr) ? (void)0 : abort())

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define MAX(a,b) ((a)>(b) ? (a) : (b))
#define MIN(a,b) ((a)<(b) ? (a) : (b))

static inline bool size_mul_overflow(size_t a, size_t b, size_t *result) {
	static_assert(INTPTR_MAX != 0, "stdint not included");
#if defined(__clang__) || __GNUC__ >= 5
#if INTPTR_MAX == INT32_MAX
	return __builtin_umul_overflow(a, b, result);
#else
	return __builtin_umull_overflow(a, b, result);
#endif
#else
	*result = a * b;
	return a && *result / a != b;
#endif
}

#endif
