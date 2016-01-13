#include <stdio.h>
#include <string.h>
#include "ac.h"

//#define DEBUG

#define func_file "lustre_runtime_funcs.txt"
#define MAX_LINES 10000
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

static void test_match_front(char **text_book)
{
	struct ac_dict *dict;
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
			mcount++;
		}
		i++;
	}
	printf("step4.a..Total:%d, matches:%d.\n", i, mcount);
	gettimeofday(&stop,0);
	time_substract(&diff, &start, &stop);
	printf("Total time : %d s,%d us. Speed %.2f matches/sec\n",
	       (int)diff.tv_sec, (int)diff.tv_usec,
	       count_per_sec(i, diff));
out:
	printf("step5...\n");
	ac_destory_dict(dict);
	printf("End front match test==========\n");
}

static void test_match_mid_full(char **text_book)
{
	struct ac_dict *dict;
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
			mcount++;
		}
		i++;
	}
	printf("step4.a..Total:%d, matches:%d.\n", i, mcount);
	gettimeofday(&stop, 0);
	time_substract(&diff, &start, &stop);
	printf("Total time : %d s,%d us. Speed %.2f matches/sec\n",
	       (int)diff.tv_sec, (int)diff.tv_usec,
	       count_per_sec(i, diff));
out:
	printf("step5...\n");
	ac_destory_dict(dict);
	printf("End mid/full match test==========\n");
}

static void test_match_end(char **text_book)
{
	struct ac_dict *dict;
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
			mcount++;
		}
		i++;
	}
	printf("step4.a..Total:%d, matches:%d.\n", i, mcount);
	gettimeofday(&stop, 0);
	time_substract(&diff, &start, &stop);
	printf("Total time : %d s,%d us. Speed %.2f matches/sec\n",
	       (int)diff.tv_sec, (int)diff.tv_usec,
	       count_per_sec(i, diff));
out:
	printf("step5...\n");
	ac_destory_dict(dict);
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
