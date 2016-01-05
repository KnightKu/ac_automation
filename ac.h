#ifndef _AC_H_
#define _AC_H_
#include <stdlib.h>

struct ac_node {
	struct ac_node* fail;
	struct ac_node** children;
	struct ac_node* parent;
	size_t size;
	unsigned int count;
	unsigned int value;
	unsigned int flag;
	unsigned int depth;
};
    
struct ac_dict {
	struct ac_node* root;
};

enum MAT_TYPE
{
	MATCH_FULL=1,
	MATCH_FRONT,
	MATCH_MIDDLE,
	MATCH_END,
	MATCH_MAX=10
};

struct ac_dict* ac_create_dict(void);
void ac_destory_dict(struct ac_dict* dict);
int ac_match(struct ac_dict* dict, char* text, enum MAT_TYPE type);
int ac_add_word(struct ac_dict* dict, char* word, size_t len);
int ac_add_word_reverse(struct ac_dict* dict, char* word, size_t len);
int ac_build_automation(struct ac_dict* dict);
void ac_verify(struct ac_node *node);
void ac_dump_words(struct ac_node *node);

#endif
