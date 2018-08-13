#!/bin/bash

overwrite=${OVERWRITE:="0"}
script_dir=$(dirname $0);

set -euo pipefail

expected=$script_dir/expected.txt;

if [ "$overwrite" != "0" ]; then
    output=$expected;
else
    actual=$(mktemp);
    output=$actual;
fi

echo -n "" > $output;

for mode in 0 1 2 3
do
    for add_seq in true false
    do
        for add_cigar in true false
        do
            if [[ $mode == 2 ]] && [[ $add_cigar ]]; then
                echo "FIXME: skipping extra tests for mode 2 when generating a cigar" >&2;
                continue;
            fi
            for add_header in true false
            do
                args="-M $mode"
                if $add_seq ; then args+=" -s"; fi
                if $add_seq ; then args+=" -s"; fi
                if $add_cigar; then args+=" -c"; fi
                if $add_header; then args+=" -H"; fi
                echo "Testing $args";

                echo "args: $args" >> $output;
                cat $script_dir/inputs.txt | $script_dir/../ksw $args >> $output;
            done
        done
    done
done

# Check test output
if [ "$overwrite" == "0" ]; then
    diff $actual $expected;
fi

echo "Tests completed successfully!";
