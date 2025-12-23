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

for library in 0 1 2
do
    # set the alignment mode
    if [ $library == 0 ]; then
        # auto
        alignment_modes=(0 1 2 3)
    elif [ $library == 1 ]; then
        # ksw2
        alignment_modes=(2 3)
    else
        # parasail
        alignment_modes=(0 1 3)
    fi

    for alignment_mode in "${alignment_modes[@]}"
    do
        for add_seq in true false
        do
            for add_cigar in true false
            do
                for add_header in true false
                do
                    for offset_and_length in true false
                    do
                        for gap_open in 5 0
                        do
                            args="-l $library -M $alignment_mode";
                            if $add_seq ; then args+=" -s"; fi
                            if $add_cigar; then args+=" -c"; fi
                            if $add_header; then args+=" -H"; fi
                            if $offset_and_length; then args+=" -O"; fi
                            args+=" -q $gap_open";
                            echo "Testing $args";

                            echo "args: $args" >> $output;
                            cat $script_dir/inputs.txt | $script_dir/../ksw $args >> $output;
                        done
                    done
                done
            done
        done
    done
done

# Test matrix loading (Issue #16: matrix validation logic fix)
echo "Testing matrix loading with 4x4 matrix (-m)";
echo "args: -m tests/matrix_4x4.txt" >> $output;
echo -e "GATTAC\nGATTAC" | $script_dir/../ksw -m $script_dir/matrix_4x4.txt >> $output;

echo "Testing matrix loading with 5x5 matrix (-m)";
echo "args: -m tests/matrix_5x5.txt" >> $output;
echo -e "GATTAC\nGATTAC" | $script_dir/../ksw -m $script_dir/matrix_5x5.txt >> $output;

# Check test output
if [ "$overwrite" == "0" ]; then
    diff $actual $expected;
fi

echo "Tests completed successfully!";
