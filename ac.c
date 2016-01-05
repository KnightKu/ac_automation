#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "ac.h"

#define ROOT_CHAR    (0xFFFFFFFF)
#define AC_CHILD_NUM (100)
#define QLENGTH  5000000

struct ac_node* ac_create_node(unsigned int value)
{
	struct ac_node* node = malloc(sizeof(struct ac_node));


	if (!node)
		return NULL;

	memset(node, 0, sizeof(struct ac_node));

	node->value = value;
	node->count = 0;
	node->size = AC_CHILD_NUM;
	node->children = malloc(sizeof(struct ac_node*) * AC_CHILD_NUM);
	if (!node->children) {
		free(node);
		return NULL;
	}
	memset(node->children, 0, sizeof(struct ac_node*) * AC_CHILD_NUM);
	node->parent = NULL;
	return node;
}

static void ac_destroy_node(struct ac_node* node)
{
	size_t k;

	for(k=0; k<node->count; k++) {
		if(node->children[k]) {
			ac_destroy_node(node->children[k]);
		}
	}
	free(node);
}

static char *revstr(char *str, size_t len)
{

	char    *start = str;
	char    *end = str + len - 1;
	char    ch;

	if (!str || len == 0)
		return NULL;

	while (start < end) {
		ch = *start;
		*start++ = *end;
		*end-- = ch;
	}

	return str;
}

/**
 * keep children in order, so query is more fast.
 **/
static int ac_add_subnode(struct ac_node* parent,
			  struct ac_node* child)
{
	size_t size = 0;
	size_t i, k;

	assert(parent && child);
	assert(parent->count <= parent->size);

	if(parent->count == parent->size) {
		void * new_buf;

		size = parent->size + AC_CHILD_NUM;
		new_buf = realloc(parent->children,
				  size * sizeof(struct ac_node*));
		if (!new_buf)
			return -1;
		parent->children = new_buf;
		parent->size = size;
	}

	/* find the pos to insert */
	for(i = 0; i < parent->count; i++)
		if(parent->children[i]->value > child->value)
			break;

 	if(i < parent->count) {
        	for(k = parent->count; k > i; k--)
 			parent->children[k] = parent->children[k - 1];
	}

	parent->children[i] = child;
 	parent->count++;

	child->depth = parent->depth+1;
	child->parent = parent;

	return 0;
}

/* Binary search */
struct ac_node* ac_has_child(struct ac_node* node, unsigned int val)
{
	int mid, left, right;
	struct ac_node** p = node->children;

	if(node->count == 0)
		return NULL;
	left = 0, right = node->count-1;

	while(left <= right) {
		mid = (left + right) >> 1;

		if(p[mid]->value == val)
			return p[mid];
		else if(p[mid]->value < val)
			left = mid+1;
		else if(p[mid]->value > val)
			right = mid-1;
	}

	return NULL;
}

void ac_verify(struct ac_node* node)
{
	size_t k;

	if(!node->count)
		return;

	for(k=0; k < node->count - 1; k++) {
		if(!(node->children[k]->value < node->children[k+1]->value)) {
			printf(" %d <- %d\n", node->children[k]->value,
				node->children[k+1]->value);
		}
		ac_verify(node->children[k]);
	}
}

static int __ac_add_word(struct ac_dict* dict, char* word, size_t len, int reverse)
{
	assert(dict && word);
	struct ac_node* node = dict->root;
	struct ac_node* child = NULL;
	struct ac_node* new = NULL;
	size_t i, k;
	char *buf;
	unsigned int ch;
	int ret = 0;

	if (reverse) {
		buf = malloc(sizeof(unsigned int) * len);
		if (!buf)
			return -1;
		strncpy(buf, word, len);
		buf = revstr(buf, len);
	} else {
		buf = word;
	}

	for(i=0; i < len; i++) {
		assert(node);
		ch = buf[i];
		for(k=0; k<node->count; k++) {
			if(ch == node->children[k]->value)
				break;
		}

		if((child = ac_has_child(node, ch)) == NULL) {
			/* no found , add child */
			new = ac_create_node(ch);
			if (!new) {
				ret = -1;
				goto out;
			}
			ret = ac_add_subnode(node, new);
			if (ret) {
				ac_destroy_node(new);
				goto out;
			}
			node = new;
		} else {
			node = child;
		}
	}
	/* a word node */
	node->flag++;
out:
	if (reverse)
		free(buf);
	return ret;
}

int ac_add_word(struct ac_dict* dict, char* word, size_t len)
{
	return __ac_add_word(dict, word, len, 0);
}

int ac_add_word_reverse(struct ac_dict* dict, char* word, size_t len)
{
	return __ac_add_word(dict, word, len, 1);
}

int ac_build_automation(struct ac_dict* dict)
{
	struct ac_node* root = dict->root;
	struct ac_node* node;
	struct ac_node* pfail;
	struct ac_node* child;
	struct ac_node** queue;
	unsigned int v;
	size_t head, tail, i;

	queue = malloc(sizeof(struct ac_node*) * QLENGTH);
	if (!queue) {
		printf("ENOMEM");
		return -1;
	}

	root->fail = NULL;
	memset(queue, 0, sizeof(struct ac_node*) * QLENGTH);
	head = tail = 0;
	queue[head++] = root;

	while(tail != head) {
		node = queue[tail++];
		for(i=0; i<node->count; i++) {
			if(node == root) {
				node->children[i]->fail = root;
			} else {
				v = node->children[i]->value;
				pfail = node->fail;
				while(pfail != NULL) {
					if((child = ac_has_child(pfail, v)) != NULL)  {
						node->children[i]->fail = child;
						break;
					}
					pfail = pfail->fail;
				}
				if(pfail == NULL)
					node->children[i]->fail = root;
			}
			if(head >= QLENGTH)
				break;
			queue[head++] = node->children[i];
		}
	}
	free(queue);
}

static struct ac_dict* ac_new_empty_dict(void)
{
	struct ac_dict* dict = malloc(sizeof(struct ac_dict));

	if (!dict)
		return NULL;

	memset(dict, 0, sizeof(struct ac_dict));
	dict->root = ac_create_node(ROOT_CHAR);
	if (!dict->root) {
		free(dict);
		return NULL;
	}
	return dict;
}

struct ac_dict* ac_create_dict(void)
{
	struct ac_dict* dict = NULL;

	dict = ac_new_empty_dict();

	return dict;
}

static void ac_print_node(struct ac_node *node)
{
	char buf[128] = {0};
	int i = 0;

	while (node->parent) {
		buf[i] = node->value;
		node = node->parent;
		i++;
	}
	printf("found: %s\n", revstr(buf, strlen(buf)));
}

int ac_match(struct ac_dict* dict, char* text, enum MAT_TYPE type)
{
	size_t len;
	char *buf;
	unsigned int i = 0;
	unsigned int val;
	struct ac_node *node;
	struct ac_node *next;
	int ret = -1;

	assert(text && dict);

	len = strlen(text);
	/* MATCH_END case, reverse the text */
	if (type == MATCH_END) {
		buf = malloc(len * sizeof(char));
		if (!buf)
			return ret;

		strncpy(buf, text, len);
		buf = revstr(buf, len);
	} else {
		buf = text;
	}

	node = dict->root;
	while(i < len) {
		val = (unsigned int)buf[i];
		while(node != dict->root && (next = ac_has_child(node, val)) == NULL)
			node = node->fail;
		next = ac_has_child(node, val);
		node = (next == NULL)? dict->root : next;
		if(node->flag) {
			ac_print_node(node);
			/* MATCH_FRONT check */
			if (type == MATCH_FRONT || type == MATCH_END) {
				if (node->depth == (i + 1)) {
					ret = 0;
					goto out;
				}
			} else {
				ret = 0;
				goto free_out;
			}
		}
		i++;
	}
free_out:
	if (type == MATCH_END)
		free(buf);
out:
	return ret;
}

void ac_destory_dict(struct ac_dict* dict)
{
	ac_destroy_node(dict->root);
	free(dict);
}

void ac_dump_words(struct ac_node *node)
{
	size_t k;

	if (node->flag)
		ac_print_node(node);

	if(!node->count)
		return;

	for(k=0; k < node->count; k++) {
		ac_dump_words(node->children[k]);
	}
}
