#!/bin/sh

test_description='git refs list tests'

. ./test-lib.sh
GIT_WHICH_TEST='refs list'
. "$TEST_DIRECTORY"/lib-for-each-ref.sh
