# Spec 038 clarification questions, round 1

Feature: `038-report-view-generators`, branch `038-report-view-generators`.
Method: the speckit-clarify discipline, applied once, no spec edit. Each
question below names a spot where two reasonable readings of the spec
diverge with materially different rendered output or API surface, and
where no Decision, Requirement or Assumption already stands a default.
Lane rulings R-1 to R-5 bound every option: the standing reader, library
first, no new dependency, the report unchanged, the evidence recorded.


## Q1: A repetition row whose column value stands unavailable

**Spec section (quoted, D-6):**

> The histogram's sample set: the per-repetition values of one column,
> the distribution the `Statistics` of specs/019 summarizes. Fewer than
> two recorded values names no shape, so the histogram answers `not
> available` with the sample count named.

**Question:** When one repetition row's value for the requested column
stands unavailable (an `Availability` other than `COUNTABLE`, the fields
`MetricValue::availability` at `include/speedgun-ng/benchmark.hpp:244`
and `ResultRow::timeAvailability` at `:404`), does that row join the
histogram's sample set and the plot's series, and does the sample count
of the disclosure name every repetition row or only the rows a value
stands for? (FR-007, FR-008)

**Why it matters:** a gap is a state the record carries for every
column, so the fixtures, the sample count and the disclosure text all
move with the answer.

| Option | Reading | Implications |
|---|---|---|
| A | A row whose value stands unavailable stands out of the sample set and the series; the disclosure's sample count names the rows a value stands for. | The figure carries measured values alone; the sample count can sit under the row count; the fixtures need one gap case. |
| B | The row joins the series as an absent slot: the plot draws a gap marker, the histogram counts the row at zero. | The figure carries a shape the run did not measure; D-4 bars that, and the marker grammar needs naming the spec nowhere carries. |
| C | One unavailable value makes the whole column absent: the view answers `not available` naming the availability token. | The strongest reading; one poisoned repetition hides every measured repetition, the least figure the record supports. |

**Recommended:** Option A. The standing disclosure rule keeps a figure
the record does not carry out of the view, and a row whose value stands
absent carries no measurement to render.


## Q2: The driver's choice between exit 2 and exit 0

**Spec section (quoted, FR-005, FR-015, FR-016):**

> **FR-005**: Each view function SHALL answer
> `std::expected<std::string, std::string>`; the value carries the view
> bytes, the error names the input the document does not carry or the
> record the run left absent, the standing `not available` shape
> (D-10).

> **FR-015**: The driver SHALL refuse one line on standard error with
> exit status 2 before any view output: a document `loadReport`
> refuses, the reader's error text; a name the document does not carry,
> the benchmark, the column or the view word; a missing or repeated
> required argument (the standing refusal shape).

> **FR-016**: A view that answers the `not available` disclosure SHALL
> print the disclosure on standard output and exit 0 (D-9).

**Question:** FR-005 puts the absent-name case and the absent-record
case on one error form, and FR-015 and FR-016 give them two exit
statuses; how does the driver decide which status an error form
deserves? (FR-015, FR-016)

**Why it matters:** a script's pipeline reads the exit status, and the
library caller needs one rule for the same split.

| Option | Reading | Implications |
|---|---|---|
| A | The driver checks the request against the loaded `ReportData` (the benchmark name, the column selector, the view word) before it calls the view function; a failed check refuses with status 2, and every error the view function answers is a disclosure with status 0. | The library keeps one error shape (D-10); the name checks live in the driver, thin argument handling under D-2; the classification is testable as two driver cases. |
| B | The library returns two distinguishable error shapes (a tagged error value, or a reserved prefix in the error text), and the driver classifies on the shape. | The public answer type widens or the error text gains a machine contract; a larger API surface for one driver decision. |
| C | The driver re-answers the request after the view error arrives and decides disclosure against refusal from its own inspection of the rows. | The classification rule lives twice, once in the library's error path and once in the driver, two sources of one fact. |

**Recommended:** Option A. The driver already holds the loaded value
and the request, so the classification runs on data the standing reader
answers and the library keeps the one error shape D-10 names.


## Q3: The column selector and the user counter columns

**Spec section (quoted, FR-006):**

> **FR-006**: The column selector SHALL read `time` for the primary
> time column, `cpu` for the CPU-time column, and a metric label of the
> benchmark for a metric column; the views SHALL render no figure
> outside the columns the document records (R-4).

**Question:** Does the column selector of FR-006 resolve a user counter
column (a name in `BenchmarkResult::counterLabels`, recorded at
`include/speedgun-ng/benchmark.hpp:647` with the per-row `counters`
values beside the metric columns), or does the selector end at `time`,
`cpu` and the metric labels? (FR-006)

**Why it matters:** the document records the counter columns, so the
selector's three-case enumeration either names the whole selector or
leaves a recorded column class with no answer.

| Option | Reading | Implications |
|---|---|---|
| A | A counter label resolves like a metric label, and the histogram, the plot, the text view and the page render it. | The figure set covers every recorded column class; FR-006, the user stories, the refusal table and the fixtures gain a counter case. |
| B | Counter columns stand out of scope for this spec: a counter label refuses one line, and a later spec extends the selector. | The smallest surface X.2 asks for; the exclusion needs an Out of scope row, and no view this lane ships renders a recorded counter column. |
| C | The page renders the counter columns as a table beside the two figures, the figures skip them. | A second page shape beside FR-011; the largest surface of the three, for a requirement no user story names. |

**Recommended:** Option B. No user story, fixture or evidence run names
a counter column, so the three-case enumeration reads as the whole
selector and X.2 keeps the lane at the shapes the stories name.


## Q4: The page when one embedded figure answers the disclosure

**Spec section (quoted, FR-011, FR-012):**

> **FR-011**: The page SHALL render one static HTML document carrying
> the run's identity as the document records it (the `speedgun`
> version, the `schemaVersion`, the benchmark name, the column) and the
> histogram and the plot views of the request, the text escaped, and no
> script (D-8).

> **FR-012**: An identity field the document does not carry SHALL
> render `not available`; the page SHALL invent no field (R-4).

**Question:** When one of the two figures the page embeds answers the
`not available` disclosure (a single-repetition column renders the plot
and leaves the histogram under two samples, per D-6), what does the
page carry in that figure's place, and does the page still answer the
value form? (FR-011)

**Why it matters:** the one-repetition column is a standing fixture
case, so the page's success rule and its escape surface both move with
the answer.

| Option | Reading | Implications |
|---|---|---|
| A | The page embeds the disclosure text in the figure's place (the standing token naming the input) and answers the value form. | The page stays one artifact per request; the reader sees which figure stands absent and why; FR-012's rule extends from identity fields to figures by the same wording. |
| B | The page answers the error form when either figure discloses. | A one-repetition column renders no page at all, the plot the run did record stands unreachable in the page view, and FR-016's exit mapping needs a second rule for a partial page. |
| C | The page omits the disclosed figure and carries no token. | The page hides the absence and invents a shape the request did not name; the disclosure rule loses its reader. |

**Recommended:** Option A. The page composes the two figures plus the
identity, so each part renders by the rule the part follows alone, and
the disclosure token names the absence in place.


## Q5: The unit the time figures print

**Spec section (quoted, D-6 and the standing facts):**

> The histogram's sample set: the per-repetition values of one column,
> the distribution the `Statistics` of specs/019 summarizes.

> A benchmark result carries its metric columns in `metricLabels`,
> one `MetricValue` per label per row (`ResultRow::metrics`); the
> primary time column prints in the unit `timeUnit` names.

**Question:** Do the histogram's bin ranges, the plot's axis values and
the text view's lines print the stored nanosecond values, or do they
scale the time column by `BenchmarkResult::timeUnit` (the print scale
of `include/speedgun-ng/benchmark.hpp:667`, the field whose contract
reads "The stored row values stay nanoseconds; only the printed scale
follows this field")? (FR-006, FR-007, FR-013)

**Why it matters:** every number the figures print moves with the
answer, and the views disagree with the report column a reader
cross-reads under the wrong choice.

| Option | Reading | Implications |
|---|---|---|
| A | The views scale the time column by `timeUnit` and name the unit beside the values; the `cpu` column scales by the same field; metric and counter values print as stored. | The figures agree with the report's printed columns; the selector's two time columns share one rule; the fixtures record scaled values. |
| B | The views print the stored values (nanoseconds) with the unit named. | The figure matches the document's stored numbers and disagrees in scale with every printed column of the run. |
| C | The unit rides the request (a driver argument). | A new argument the grammar of D-9 does not name; the driver grows a surface no user story exercises. |

**Recommended:** Option A. `timeUnit` exists as the print scale of the
run's time values, so a view that prints another scale renders the same
numbers twice under two names, and the spec's own standing fact points
the printed column at `timeUnit`.


## Considered and dropped

- Bin boundary convention and label precision: FR-007 requires the bins
  to cover the recorded value range, so the reading that drops the
  maximum fails the requirement already; the label precision is a
  plan-level detail.
- Tick marks on the plot axes: the acceptance text of User Story 2
  names labels alone, so labels-only stands as the default reading.
- The width of the text view's proportional bar: a plan-level
  parameter, fixture-visible yet free of a correctness split.
- Schema minors `1.5` through `1.8`: the Assumptions section covers the
  variance, and each evidence record names the schema its document
  carries.
