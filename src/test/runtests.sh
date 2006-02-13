#!/bin/sh

jed="$1"
if [ ! -x "$jed" ]; then
  echo "Usage: $0 /path/to/jed"
  exit 1
fi

test_dir=`dirname "$0"`
jed_root=$test_dir/../..

export JED_ROOT="$jed_root"
jedscript="$jed -script"

failed=0
for X in $test_dir/test_*.sl
do
   echo Running $jedscript $X
   if ! $jedscript ./$X
   then 
      failed=1
   fi
done
exit $failed
