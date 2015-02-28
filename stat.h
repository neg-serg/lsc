#ifndef LSC_STAT_H
#define LSC_STAT_H

struct file_info {
	struct suf_indexed name;
	struct suf_indexed linkname;
	size_t size;
	mode_t mode;
	mode_t linkmode;
	uint32_t time;
	bool linkok;
};

struct file_list {
	struct file_info *data;
	size_t len;
	size_t cap;
	bool malloc;
};

void file_list_init(struct file_list *fl);
void file_list_clear(struct file_list *fl);
void file_list_free(struct file_list *fl);

void ls(struct file_list *l, char *name);

#endif
