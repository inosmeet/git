#!/bin/sh

test_description='git refs list tests'

. ./test-lib.sh

GIT_REFS_LIST_CMD='refs list'
. "$TEST_DIRECTORY"/t6300-for-each-ref.sh
