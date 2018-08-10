#!/bin/bash

overwrite=${OVERWRITE:="0"}
script_dir=$(dirname $0);

set -euo pipefail

for mode in 0 1 2 3
do
	echo "Testing mode $mode";
	expected=$script_dir/expected.${mode}.txt
	if [ "$overwrite" != "0" ]; then
		cat $script_dir/inputs.txt | $script_dir/../ksw -M $mode > $expected;
	else
		actual=$(mktemp);
		cat $script_dir/inputs.txt | $script_dir/../ksw -M $mode > $actual;
		diff -q $actual $expected;
	fi
done

echo "Tests completed successfully!";
