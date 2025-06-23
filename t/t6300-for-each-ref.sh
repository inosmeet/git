#!/bin/sh
#
# Copyright (c) 2007 Andy Parkins
#

test_description='for-each-ref test'

. ./test-lib.sh
GIT_WHICH_TEST='for-each-ref'
. "$TEST_DIRECTORY"/lib-for-each-ref.sh
