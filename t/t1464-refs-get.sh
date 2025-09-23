#!/bin/sh

test_description='git refs get'
GIT_TEST_DEFAULT_INITIAL_BRANCH_NAME=main
export GIT_TEST_DEFAULT_INITIAL_BRANCH_NAME

. ./test-lib.sh

test_expect_success 'setup repository' '
	test_commit one &&
	git tag -a -m "tagging one" my-tag one &&
	git symbolic-ref refs/my-symref refs/heads/main &&
	git symbolic-ref refs/dangling-symref refs/heads/no-such-branch
'

test_expect_success 'fails with no arguments' '
	test_must_fail git refs get >out 2>err &&
	test_grep "refs get requires exactly one reference" err
'

test_expect_success 'fails with too many arguments' '
	test_must_fail git refs get HEAD HEAD >out 2>err &&
	test_grep "refs get requires exactly one reference" err
'

test_expect_success 'get a branch head' '
	git rev-parse main >expect &&
	git refs get refs/heads/main >actual &&
	test_cmp expect actual
'

test_expect_success 'get an annotated tag' '
	git rev-parse my-tag >expect &&
	git refs get refs/tags/my-tag >actual &&
	test_cmp expect actual
'

test_expect_success 'get HEAD (a symbolic ref)' '
	echo "ref: refs/heads/main" >expect &&
	git refs get HEAD >actual &&
	test_cmp expect actual
'

test_expect_success 'get a custom symbolic ref' '
	echo "ref: refs/heads/main" >expect &&
	git refs get refs/my-symref >actual &&
	test_cmp expect actual
'

test_expect_success 'get a dangling symbolic ref' '
	echo "ref: refs/heads/no-such-branch" >expect &&
	git refs get refs/dangling-symref >actual &&
	test_cmp expect actual
'

test_expect_success 'get a non-existent ref' '
	test_must_fail git refs get refs/heads/no-such-branch 2>err &&
	test_grep "not a valid ref" err
'

test_expect_success 'get does not perform DWIM' '
	test_must_fail git refs get main 2>err &&
	test_grep "not a valid ref" err
'

test_done
