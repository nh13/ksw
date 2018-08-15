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

for alignment_mode in 0 1 2 3
do
    for add_seq in true false
    do
        for add_cigar in true false
        do
            for add_header in true false
            do
                for right_align in true false
                do
                    for offset_and_length in true false
                    do
                        args="-M $alignment_mode"
                        if $add_seq ; then args+=" -s"; fi
                        if $add_cigar; then args+=" -c"; fi
                        if $add_header; then args+=" -H"; fi
                        if $right_align; then args+=" -R"; fi
                        if $offset_and_length; then args+=" -O"; fi
                        echo "Testing $args";

                        echo "args: $args" >> $output;
                        cat $script_dir/inputs.txt | $script_dir/../ksw $args >> $output;
                    done
                done
            done
        done
    done
done

# Check test output
if [ "$overwrite" == "0" ]; then
    diff $actual $expected;
fi

echo "Tests completed successfully!";
