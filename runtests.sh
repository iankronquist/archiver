#!/bin/bash

for file in $(ls tests/bin); do
	if valgrind --error-exitcode=0 tests/bin/$file $TEST_ARGS; then
		echo "Error: $file leaking memory"
	fi
	tests/bin/$file
done

for file in $(ls build/bin); do
	if valgrind --error-exitcode=0 $file $ARGS; then 
		echo "Error: leaking memory"
	fi
done
