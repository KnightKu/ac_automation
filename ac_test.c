#include <stdio.h>
#include <string.h>
#include "ac.h"

#define DEBUG

char *text_book[] = {
"ll_file_open",
"ll_layout_restore",
"ll_hsm_state_set",
"ll_lov_getstripe",
"ll_layout_refresh",
"ll_file_aio_read",
"ll_file_read",
"ll_fid2path",
"ll_lov_setstripe",
"ll_file_ioctl",
"ll_close_thread_shutdown",
"ll_close_next_lli",
"ll_done_writing_attr",
"ll_ioepoch_close",
"ll_queue_done_writing",
"vvp_write_complete",
"vvp_write_pending",
"ll_close_thread_start",
"ll_som_update",
"ll_close_thread",
"ll_get_fsname",
"get_fsname",
"ll_dirty_page_discard_warn",
"ll_get_obd_name",
"cksum_obd2cfs",
"ll_dump_inode",
"lustre_dump_dentry",
"ll_show_options",
"obd_statfs_rqset",
"obd_set_info_async.clone.3",
"ll_clear_inode",
"ll_statfs_internal",
"ll_statfs",
"ll_md_setattr",
"ll_get_default_mdsize",
NULL
};

static void test_match_front()
{
	struct ac_dict *dict;
	int i, ret;

	printf("Start front match test==========\n");
	printf("step1...\n");
	dict = ac_create_dict();
	if (!dict)
		return;

	printf("step2...\n");
	ret = ac_add_word(dict, "ll_file", strlen("ll_file"));
	if (ret)
		goto out;
	ret = ac_add_word(dict, "obd_", strlen("obd_"));
	if (ret)
		goto out;
	printf("step3...\n");
    	ret = ac_build_automation(dict);
	if (ret)
		goto out;
	i = 0;
#ifdef DEBUG
	ac_verify(dict->root);
	ac_dump_words(dict->root);
#endif
	printf("step4...\n");
	while (text_book[i]) {
		ret = ac_match(dict, text_book[i], MATCH_FRONT);
		if (!ret)
			printf("matched text:%s\n", text_book[i]);
		i++;
	}
out:
	printf("step5...\n");
	ac_destory_dict(dict);
	printf("End front match test==========\n");
}

static void test_match_mid_full()
{
	struct ac_dict *dict;
	int i, ret;

	printf("Start mid/full match test==========\n");
	printf("step1...\n");
	dict = ac_create_dict();
	if (!dict)
		return;

	printf("step2...\n");
	ret = ac_add_word(dict, "_file_", strlen("_file_"));
	if (ret)
		goto out;
	ret = ac_add_word(dict, "ll_dirty_page_discard_warn", strlen("ll_dirty_page_discard_warn"));
	if (ret)
		goto out;
	printf("step3...\n");
    	ret = ac_build_automation(dict);
	if (ret)
		goto out;
#ifdef DEBUG
	ac_verify(dict->root);
	ac_dump_words(dict->root);
#endif
	i = 0;
	printf("step4...\n");
	while (text_book[i]) {
		ret = ac_match(dict, text_book[i], MATCH_MIDDLE);
		if (!ret)
			printf("matched text:%s\n", text_book[i]);
		i++;
	}
out:
	printf("step5...\n");
	ac_destory_dict(dict);
	printf("End mid/full match test==========\n");
}

static void test_match_end()
{
	struct ac_dict *dict;
	int i, ret;

	printf("Start front match test==========\n");
	printf("step1...\n");
	dict = ac_create_dict();
	if (!dict)
		return;
	
	printf("step2...\n");
	ret = ac_add_word_reverse(dict, "_read", strlen("_read"));
	if (ret)
		goto out;
	ret = ac_add_word_reverse(dict, "_inode", strlen("_inode"));
	if (ret)
		goto out;
	printf("step3...\n");
    	ret = ac_build_automation(dict);
	if (ret)
		goto out;
	i = 0;
#ifdef DEBUG
	ac_verify(dict->root);
	ac_dump_words(dict->root);
#endif
	printf("step4...\n");
	while (text_book[i]) {
		ret = ac_match(dict, text_book[i], MATCH_END);
		if (!ret)
			printf("matched text:%s\n", text_book[i]);
		i++;
	}
out:
	printf("step5...\n");
	ac_destory_dict(dict);
	printf("End front match test==========\n");
}

int main()
{
	/* "foo*" */
	test_match_front();
	/* "foo","*foo*" */
	test_match_mid_full();
	/* "*foo" */
	test_match_end();

	return 0;
}
