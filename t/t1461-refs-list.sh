#!/bin/sh

test_description='Verify git refs list functionality and compatibility with git show-ref'
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

test_expect_success 'refs list --branches, --tags, --head, pattern' '
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
	test_cmp expect actual &&

	{
		echo $(git rev-parse HEAD) HEAD &&
		echo $(git rev-parse refs/heads/B) refs/heads/B &&
		echo $(git rev-parse refs/tags/B) refs/tags/B
	} >expect &&
	git refs list --head B >actual &&
	test_cmp expect actual &&

	{
		echo $(git rev-parse refs/heads/B) refs/heads/B &&
		echo $(git rev-parse refs/tags/A) refs/tags/A &&
		echo $(git rev-parse refs/tags/B) refs/tags/B
	} >expect &&
	git refs list A B >actual &&
	test_cmp expect actual
'

test_expect_success 'Backward compatibility with show-ref' '
	git show-ref >expect&&
	git refs list >actual&&
	test_cmp expect actual &&

	git show-ref --branches >expect &&
	git refs list --branches >actual &&
	test_cmp expect actual &&

	git show-ref --tags >expect &&
	git refs list --tags >actual &&
	test_cmp expect actual &&

	git show-ref --head >expect &&
	git refs list --head >actual &&
	test_cmp expect actual &&

	git show-ref --branches --tags --head >expect &&
	git refs list --branches --tags --head >actual &&
	test_cmp expect actual &&

	git show-ref B >expect &&
	git refs list B >actual &&
	test_cmp expect actual &&

	git show-ref --head B >expect &&
	git refs list --head B >actual &&
	test_cmp expect actual &&

	git show-ref A B >expect &&
	git refs list A B >actual &&
	test_cmp expect actual
'

test_done
