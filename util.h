#define log(fmt, ...) (assertx(fprintf(stderr, fmt "\n", __VA_ARGS__) >= 0))

#ifdef program_name

#define warn(fmt, ...) (log("%s: " fmt, program_name, __VA_ARGS__))

#define die(fmt, ...) do { \
	warn(fmt, __VA_ARGS__); \
	exit(1); \
} while (0)

#define warn_errno(fmt, ...) warn(fmt ": %s", __VA_ARGS__, strerror(errno))
#define die_errno(fmt, ...) die(fmt ": %s", __VA_ARGS__, strerror(errno))

#endif // program_name

#define assertx(expr) (likely(expr) ? (void)0 : abort())

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define MAX(a,b) ((a)>(b) ? (a) : (b))
#define MIN(a,b) ((a)<(b) ? (a) : (b))

bool size_mul_overflow(size_t a, size_t b, size_t *result);

void *xmalloc(size_t size);
void *xrealloc(void *p, size_t size);
void *xmallocr(size_t nmemb, size_t size);
void *xreallocr(void *p, size_t nmemb, size_t size);
