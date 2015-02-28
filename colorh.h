struct ind_name {
	const char* name;
	int code;
};

const struct ind_name *ind_name_lookup(const char *str, const unsigned int len);

#define TOTAL_KEYWORDS 24
#define WORD_LENGTH 2
#define MIN_HASH_VALUE 0
#define MAX_HASH_VALUE 53
