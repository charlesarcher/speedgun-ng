#!/usr/bin/env bash
# The evidence runs of specs/038 (FR-021, D-5): the placement report of
# specs/035 and the tracer reports of specs/036 render through the
# driver, one record per run beside the views. The script runs on the
# lane host from the repository root, the release build at build/ stood
# first (the ci-rocky preset configure, then the build).
set -u
cd "$(dirname "$0")/../.." || exit 1
R=specs/038-report-view-generators/results
DRIVER=build/tools/report_view

# Run A: the placement run of specs/035, five repetitions, the report
# beside the records.
./build/example/docs_numa_placement --placement=node --repetitions=5 \
    --json=$R/reports/placement.json > /tmp/placement_run.out 2>&1
echo "placement run exit $?"

# Run B: the tracer reports of specs/036 stand recorded at
# reports/osnoise.json and reports/timerlat.json (schema 1.7).

# The views of run A.
V=$R/reports/placement.json
$DRIVER $V --benchmark='bmSharded/threads:2' --metric=time --view=histogram > $R/views/placement-histogram.svg
echo "placement histogram exit $?"
$DRIVER $V --benchmark='bmSharded/threads:2' --metric=time --view=plot > $R/views/placement-plot.svg
echo "placement plot exit $?"
$DRIVER $V --benchmark='bmSharded/threads:2' --metric=time --view=page > $R/views/placement-page.html
echo "placement page exit $?"
$DRIVER $V --benchmark='bmSharded/threads:2' --metric=time --view=text > $R/views/placement-text.txt
echo "placement text exit $?"

# The views of run B, one block per report.
for engine in osnoise timerlat; do
  V=$R/reports/$engine.json
  $DRIVER $V --benchmark='bmTouch/threads:2' --metric=time --view=histogram > $R/views/$engine-histogram.txt
  echo "$engine histogram exit $?"
  $DRIVER $V --benchmark='bmTouch/threads:2' --metric=time --view=plot > $R/views/$engine-plot.svg
  echo "$engine plot exit $?"
  $DRIVER $V --benchmark='bmTouch/threads:2' --metric=time --view=page > $R/views/$engine-page.html
  echo "$engine page exit $?"
  $DRIVER $V --benchmark='bmTouch/threads:2' --metric=time --view=text > $R/views/$engine-text.txt
  echo "$engine text exit $?"
done
