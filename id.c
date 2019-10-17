#include <sys/types.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"

struct uid { uid_t uid; struct uid *next; char name[]; };
struct gid { gid_t gid; struct gid *next; char name[]; };

static struct uid *ucache;

const char *getuser(uid_t uid) {
	for (struct uid *p = ucache; p; p = p->next)
		if (p->uid == uid)
			return p->name[0] ? p->name : NULL;
	struct passwd *pwd = getpwuid(uid);
	char *name = pwd ? pwd->pw_name : "";
	struct uid *p = xmalloc(sizeof(*p) + strlen(name) + 1);
	p->uid = uid;
	strcpy(p->name, name);
	p->next = ucache;
	ucache = p;
	return p->name[0] ? p->name : NULL;
}

static struct gid *gcache;

const char *getgroup(gid_t gid) {
	for (struct gid *p = gcache; p; p = p->next)
		if (p->gid == gid)
			return p->name[0] ? p->name : NULL;
	struct passwd *pwd = getpwuid(gid);
	char *name = pwd ? pwd->pw_name : "";
	struct gid *p = xmalloc(sizeof(*p) + strlen(name) + 1);
	p->gid = gid;
	strcpy(p->name, name);
	p->next = gcache;
	gcache = p;
	return p->name[0] ? p->name : NULL;
}
