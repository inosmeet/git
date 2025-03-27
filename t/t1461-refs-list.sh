#!/bin/sh

test_description='refs list'
GIT_TEST_DEFAULT_INITIAL_BRANCH_NAME=main
export GIT_TEST_DEFAULT_INITIAL_BRANCH_NAME

. ./test-lib.sh

test_expect_success setup '
	test_commit --annotate A &&
	git checkout -b side &&
	test_commit --annotate B &&
	git checkout main &&
	test_commit C &&
	git branch B A^0
'
test_expect_success 'refs list --branches, --tags, --head' '
	for branch in B main side
	do
		echo $(git rev-parse refs/heads/$branch) refs/heads/$branch || return 1
	done >expect.branches &&
	git refs list --branches >actual &&
	test_cmp expect.branches actual &&

	for tag in A B C
	do
		echo $(git rev-parse refs/tags/$tag) refs/tags/$tag || return 1
	done >expect.tags &&
	git refs list --tags >actual &&
	test_cmp expect.tags actual &&

	cat expect.branches expect.tags >expect &&
	git refs list --branches --tags >actual &&
	test_cmp expect actual &&

	{
		echo $(git rev-parse HEAD) HEAD &&
		cat expect.branches expect.tags
	} >expect &&
	git refs list --branches --tags --head >actual &&
	test_cmp expect actual
'

test_expect_success 'refs list --heads is deprecated and hidden' '
	test_expect_code 129 git refs list -h >short-help &&
	test_grep ! -e --heads short-help &&
	git refs list --heads >actual 2>warning &&
	test_grep ! deprecated warning &&
	test_cmp expect.branches actual
'

test_done
