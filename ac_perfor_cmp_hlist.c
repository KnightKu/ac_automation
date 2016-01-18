#include <stdio.h>
#include <string.h>
#include "hlist.h"
#include "ac.h"
#include <linux/types.h>

//#define DEBUG
#define TRACE_HASH_BITS 7
#define TRACE_FUNC_HASHSIZE (1 << TRACE_HASH_BITS)
#define func_file "lustre_runtime_funcs.txt"
#define MAX_LINES 10000

static struct hlist_head ftrace_func_hash[TRACE_FUNC_HASHSIZE];

struct trace_func_item {
	struct hlist_node	node;
	unsigned long		id;
};

char *front_pattern[] = {
	"lprocfs_add_",
	"lustre_cfg",
	"lmd_parse",
	"lustre_start",
	"ll_get",
	"ll_xattr",
	"ll_vm",
	"obd_",
	"osd_oi",
	"fid_",
	NULL
};
char *end_pattern[] = {
	"_init",
	"_cleanup",
	"fini",
	"_create",
	"_get",
	"_add",
	"_setup",
	"_nid",
	"_param",
	"_put",
	NULL
};

char *mid_pattern[] = {
	"alloc",
	"free",
	"fill",
	"split",
	"insert",
	"add",
	"remove",
	"_lfix_",
	"_node_",
	"page",
	NULL
};

static int func_match(char *str, char *regex, int len, enum MAT_TYPE type)
{
	int matched = 0;
	char *ptr;

	switch (type) {
	case MATCH_FULL:
		if (strcmp(str, regex) == 0)
			matched = 1;
		break;
	case MATCH_FRONT:
		if (strncmp(str, regex, len) == 0)
			matched = 1;
		break;
	case MATCH_MIDDLE:
		if (strstr(str, regex))
			matched = 1;
		break;
	case MATCH_END:
		ptr = strstr(str, regex);
		if (ptr && (ptr[len] == 0))
			matched = 1;
		break;
	}

	return matched;
}

static int time_substract(struct timeval *result,
			  struct timeval *begin,
			  struct timeval *end)
{
	if (begin->tv_sec > end->tv_sec)
		return -1;

	if((begin->tv_sec == end->tv_sec) && (begin->tv_usec > end->tv_usec))
		return -2;

	result->tv_sec = (end->tv_sec - begin->tv_sec);
	result->tv_usec = (end->tv_usec - begin->tv_usec);
	if(result->tv_usec < 0) {
		result->tv_sec--;
		result->tv_usec += 1000000;
	}

	return 0;
}

static double count_per_sec(int count, struct timeval tv)
{
	double sec = tv.tv_sec;

	sec += (double)tv.tv_usec/ 1000000.0;

	return (double)((double)count/sec);
}

static int add_pattern_words(struct ac_dict *dict, char *patterns[], int reverse)
{
	int ret = 0;

	if (reverse) {
		while (*patterns) {
			ret = ac_add_word_reverse(dict, *patterns, strlen(*patterns));
			if (ret)
				return ret;
			patterns++;
		}
	} else {
		while (*patterns) {
			ret = ac_add_word(dict, *patterns, strlen(*patterns));
			if (ret)
				return ret;
			patterns++;
		}
	}
}

static int func_trace_add(unsigned long item_id)
{
	unsigned long key;
	struct hlist_head *hhead;
	struct trace_func_item *item = malloc(sizeof(struct trace_func_item));

	if (!item)
		return -1;

	item->id = item_id;

	key = hash_64(item_id, TRACE_HASH_BITS);

	hhead = &ftrace_func_hash[key];

	hlist_add_head(&item->node, hhead);

	return 0;
}

static int match_and_add(char **text_book, char **patterns, enum MAT_TYPE type)
{
	int i = 0;
	int mcount = 0;

	while (text_book[i]) {
		int j = 0;

		while (patterns[j]) {
			if (func_match(text_book[i], patterns[j], strlen(patterns[j]), type)) {
				func_trace_add((unsigned long)text_book[i]);
				mcount++;
				break;
			}
			j++;
		}
		i++;
	}
	return mcount;
}

static struct trace_func_item *func_search(unsigned long item_id)
{
	unsigned long key;
	struct hlist_head *hhead;
	struct trace_func_item *item;
	struct hlist_node *n;

	key = hash_64(item_id, TRACE_HASH_BITS);

	hhead = &ftrace_func_hash[key];

	hlist_for_each_entry(item, n, hhead, node) {
		if (item->id == item_id) {
			return item;
		}
	}

	return NULL;
}

static int seach_and_match(char **text_book)
{
	int i = 0;
	int mcount = 0;

	while (text_book[i]) {
		if (func_search((unsigned long)text_book[i])) {
			mcount++;
		}
		i++;
	}
	return mcount;
}

static void cleanup_hlist(void)
{
	unsigned long i, count;
	struct hlist_head *hhead;
	struct trace_func_item *item;
	struct hlist_node *n, *next;

	i = count = 0;

	while (i < TRACE_FUNC_HASHSIZE) {
		hhead = &ftrace_func_hash[i];

		hlist_for_each_entry_safe(item, n, next, hhead, node) {
			hlist_del(n);
			free(item);
		}
		i++;
	}
}

static unsigned long dump_hlist(void)
{
	unsigned long i, count;
	struct hlist_head *hhead;
	struct trace_func_item *item;
	struct hlist_node *n;

	i = count = 0;

	while (i < TRACE_FUNC_HASHSIZE) {
		hhead = &ftrace_func_hash[i];

		hlist_for_each_entry(item, n, hhead, node) {
			printf("%s\n", (char *)item->id);
			count++;
		}
		i++;
	}
	return count;
}

static void test_match_front(char **text_book)
{
	struct ac_dict *dict;
	struct ac_dict *filter_dict = NULL;
	int i, ret, mcount;
	struct timeval start, stop, diff;

	memset(&start, 0, sizeof(struct timeval));
	memset(&stop, 0, sizeof(struct timeval));
	memset(&diff, 0, sizeof(struct timeval));

	printf("Start front match test==========\n");
	printf("step1...\n");
	dict = ac_create_dict();
	if (!dict)
		return;

	filter_dict = ac_create_dict();
	if (!filter_dict)
		goto out;

	printf("step2...\n");
	if (add_pattern_words(dict, front_pattern, 0))
		goto out;
	printf("step3...\n");
    	ret = ac_build_automation(dict);
	if (ret)
		goto out;
	i = mcount = 0;
#ifdef DEBUG
	ac_verify(dict->root);
	ac_dump_words(dict->root);
#endif
	printf("step4...\n");
	gettimeofday(&start, 0);
	while (text_book[i]) {
		ret = ac_match(dict, text_book[i], MATCH_FRONT);
		if (!ret) {
#ifdef DEBUG
			printf("matched text:%s\n", text_book[i]);
#endif
			ac_add_word(filter_dict, text_book[i], strlen(text_book[i]));
			mcount++;
		}
		i++;
	}
	printf("step4..Total:%d, matches:%d.\n", i, mcount);
	gettimeofday(&stop,0);
	time_substract(&diff, &start, &stop);
	printf("AC. Total time : %d s,%d us. Speed %.2f matches/sec\n",
	       (int)diff.tv_sec, (int)diff.tv_usec,
	       count_per_sec(i, diff));

	printf("step4.a...\n");
    	ret = ac_build_automation(filter_dict);
	if (ret)
		goto out;
	i = mcount = 0;

	gettimeofday(&start, 0);
	while (text_book[i]) {
		char *text = text_book[i];

		ret = ac_match(filter_dict, text, MATCH_FULL);
		if (!ret) {
			mcount++;
		}
		i++;
	}
	gettimeofday(&stop,0);
	printf("step4.a..Total:%d, matches:%d.\n", i, mcount);
	time_substract(&diff, &start, &stop);
	printf("AC. Match Total time : %d s,%d us. Speed %.2f matches/sec\n",
	       (int)diff.tv_sec, (int)diff.tv_usec,
	       count_per_sec(i, diff));

	printf("step4.b...\n");
	// hlist test...
	gettimeofday(&start, 0);
	mcount = match_and_add(text_book, front_pattern, MATCH_FRONT);
	gettimeofday(&stop,0);
	printf("step4.b..Total:%d, matches:%d.\n", i, mcount);
	time_substract(&diff, &start, &stop);
	printf("Hlist. Total time : %d s,%d us. Speed %.2f matches/sec\n",
	       (int)diff.tv_sec, (int)diff.tv_usec,
	       count_per_sec(i, diff));

	printf("step4.c...\n");
	// hlist test...
	gettimeofday(&start, 0);
	mcount = seach_and_match(text_book);
	gettimeofday(&stop,0);
	printf("step4.c..Total:%d, matches:%d.\n", i, mcount);
	time_substract(&diff, &start, &stop);
	printf("Hlist. MATCH  Total time : %d s,%d us. Speed %.2f matches/sec\n",
	       (int)diff.tv_sec, (int)diff.tv_usec,
	       count_per_sec(i, diff));
out:
	printf("step5...\n");
	cleanup_hlist();
	ac_destory_dict(dict);
	if (filter_dict)
		ac_destory_dict(filter_dict);
	printf("End front match test==========\n");
}

static void test_match_mid_full(char **text_book)
{
	struct ac_dict *dict;
	struct ac_dict *filter_dict = NULL;
	int i, ret, mcount;
	struct timeval start, stop, diff;

	memset(&start, 0, sizeof(struct timeval));
	memset(&stop, 0, sizeof(struct timeval));
	memset(&diff, 0, sizeof(struct timeval));

	printf("Start mid/full match test==========\n");
	printf("step1...\n");
	dict = ac_create_dict();
	if (!dict)
		return;
	filter_dict = ac_create_dict();
	if (!filter_dict)
		goto out;

	printf("step2...\n");
	if (add_pattern_words(dict, mid_pattern, 0))
		goto out;
	printf("step3...\n");
    	ret = ac_build_automation(dict);
	if (ret)
		goto out;
#ifdef DEBUG
	ac_verify(dict->root);
	ac_dump_words(dict->root);
#endif
	i = mcount = 0;
	printf("step4...\n");
	gettimeofday(&start, 0);
	while (text_book[i]) {
		ret = ac_match(dict, text_book[i], MATCH_MIDDLE);
		if (!ret) {
#ifdef DEBUG
			printf("matched text:%s\n", text_book[i]);
#endif
			ac_add_word(filter_dict, text_book[i], strlen(text_book[i]));
			mcount++;
		}
		i++;
	}
	printf("step4..Total:%d, matches:%d.\n", i, mcount);
	gettimeofday(&stop, 0);
	time_substract(&diff, &start, &stop);
	printf("Total time : %d s,%d us. Speed %.2f matches/sec\n",
	       (int)diff.tv_sec, (int)diff.tv_usec,
	       count_per_sec(i, diff));

	printf("step4.a...\n");
    	ret = ac_build_automation(filter_dict);
	if (ret)
		goto out;
	i = mcount = 0;

	gettimeofday(&start, 0);
	while (text_book[i]) {
		char *text = text_book[i];

		ret = ac_match(filter_dict, text, MATCH_FULL);
		if (!ret) {
			mcount++;
		}
		i++;
	}
	gettimeofday(&stop,0);
	printf("step4.a..Total:%d, matches:%d.\n", i, mcount);
	time_substract(&diff, &start, &stop);
	printf("AC. Match Total time : %d s,%d us. Speed %.2f matches/sec\n",
	       (int)diff.tv_sec, (int)diff.tv_usec,
	       count_per_sec(i, diff));
	
	printf("step4.b...\n");
	// hlist test...
	gettimeofday(&start, 0);
	mcount = match_and_add(text_book, mid_pattern, MATCH_MIDDLE);
	gettimeofday(&stop,0);
	printf("step4.b..Total:%d, matches:%d.\n", i, mcount);
	time_substract(&diff, &start, &stop);
	printf("Hlist. Total time : %d s,%d us. Speed %.2f matches/sec\n",
	       (int)diff.tv_sec, (int)diff.tv_usec,
	       count_per_sec(i, diff));

	printf("step4.c...\n");
	// hlist test...
	gettimeofday(&start, 0);
	mcount = seach_and_match(text_book);
	gettimeofday(&stop,0);
	printf("step4.c..Total:%d, matches:%d.\n", i, mcount);
	time_substract(&diff, &start, &stop);
	printf("Hlist. MATCH  Total time : %d s,%d us. Speed %.2f matches/sec\n",
	       (int)diff.tv_sec, (int)diff.tv_usec,
	       count_per_sec(i, diff));
out:
	printf("step5...\n");
	cleanup_hlist();
	ac_destory_dict(dict);
	if (filter_dict)
		ac_destory_dict(filter_dict);
	printf("End mid/full match test==========\n");
}

static void test_match_end(char **text_book)
{
	struct ac_dict *dict;
	struct ac_dict *filter_dict = NULL;
	int i, ret, mcount;
	struct timeval start, stop, diff;

	memset(&start, 0, sizeof(struct timeval));
	memset(&stop, 0, sizeof(struct timeval));
	memset(&diff, 0, sizeof(struct timeval));

	printf("Start end match test==========\n");
	printf("step1...\n");
	dict = ac_create_dict();
	if (!dict)
		return;
	filter_dict = ac_create_dict();
	if (!filter_dict)
		goto out;
	
	printf("step2...\n");
	if (add_pattern_words(dict, end_pattern, 1))
		goto out;
	printf("step3...\n");
    	ret = ac_build_automation(dict);
	if (ret)
		goto out;
	i = mcount = 0;
#ifdef DEBUG
	ac_verify(dict->root);
	ac_dump_words(dict->root);
#endif
	printf("step4...\n");
	gettimeofday(&start, 0);
	while (text_book[i]) {
		ret = ac_match(dict, text_book[i], MATCH_END);
		if (!ret) {
#ifdef DEBUG
			printf("matched text:%s\n", text_book[i]);
#endif
			ac_add_word(filter_dict, text_book[i], strlen(text_book[i]));
			mcount++;
		}
		i++;
	}
	printf("step4..Total:%d, matches:%d.\n", i, mcount);
	gettimeofday(&stop, 0);
	time_substract(&diff, &start, &stop);
	printf("Total time : %d s,%d us. Speed %.2f matches/sec\n",
	       (int)diff.tv_sec, (int)diff.tv_usec,
	       count_per_sec(i, diff));

	printf("step4.a...\n");
    	ret = ac_build_automation(filter_dict);
	if (ret)
		goto out;
	i = mcount = 0;

	gettimeofday(&start, 0);
	while (text_book[i]) {
		char *text = text_book[i];

		ret = ac_match(filter_dict, text, MATCH_FULL);
		if (!ret) {
			mcount++;
		}
		i++;
	}
	gettimeofday(&stop,0);
	printf("step4.a..Total:%d, matches:%d.\n", i, mcount);
	time_substract(&diff, &start, &stop);
	printf("AC. Match Total time : %d s,%d us. Speed %.2f matches/sec\n",
	       (int)diff.tv_sec, (int)diff.tv_usec,
	       count_per_sec(i, diff));

	printf("step4.b...\n");
	// hlist test...
	gettimeofday(&start, 0);
	mcount = match_and_add(text_book, end_pattern, MATCH_END);
	gettimeofday(&stop, 0);
	printf("step4.b..Total:%d, matches:%d.\n", i, mcount);
	time_substract(&diff, &start, &stop);
	printf("Hlist. Total time : %d s,%d us. Speed %.2f matches/sec\n",
	       (int)diff.tv_sec, (int)diff.tv_usec,
	       count_per_sec(i, diff));

	printf("step4.c...\n");
	// hlist test...
	gettimeofday(&start, 0);
	mcount = seach_and_match(text_book);
	gettimeofday(&stop,0);
	printf("step4.c..Total:%d, matches:%d.\n", i, mcount);
	time_substract(&diff, &start, &stop);
	printf("Hlist. MATCH  Total time : %d s,%d us. Speed %.2f matches/sec\n",
	       (int)diff.tv_sec, (int)diff.tv_usec,
	       count_per_sec(i, diff));
out:
	printf("step5...\n");
	cleanup_hlist();
	ac_destory_dict(dict);
	if (filter_dict)
		ac_destory_dict(filter_dict);
	printf("End end match test==========\n");
}

static int prepare_func_table(char **func_table)
{
	FILE *fp;
	char buf[100] = {0};
	int i = 0;
	int ret = 0;
	char *find = NULL;

	if (!func_table)
		return -1;

	fp = fopen(func_file, "r");
	if (!fp)
		return -1;

	memset(func_table, 0, sizeof(char *) * MAX_LINES);

	while(fgets(buf, 100, fp)) {
		func_table[i] = malloc(strlen(buf) + 1);
		if (!func_table[i]) {
			ret = -1;
			goto out;
		}
		find = strchr(buf, '\n');
		if(find)
			*find = '\0';
		strncpy(func_table[i], buf, strlen(buf));
		i++;
	}
	printf("Total %d funcs....", i);
out:
	if (ret) {
		while (i >=0)
			free(func_table[i--]);
		free(func_table);
		fclose(fp);
	}
	
	return ret;
}

static void clean_up_func_table(char **func_table)
{
	int i = 0;

	while (i < MAX_LINES) {
		if (func_table[i])
			free(func_table[i]);
		i++;
	}

	free(func_table);
}

int main()
{
	char **func_table = NULL;

	func_table = malloc(sizeof(char *) * MAX_LINES);
	if (!func_table)
		return -1;

	printf("Prepare function tables!\n");
	if (prepare_func_table(func_table))
		return -1;

	printf("Start testing...!\n");
	/* "foo*" */
	test_match_front(func_table);
	/* "foo","*foo*" */
	test_match_mid_full(func_table);
	/* "*foo" */
	test_match_end(func_table);

	printf("Cleanup function tables...!\n");
	clean_up_func_table(func_table);

	return 0;
}
