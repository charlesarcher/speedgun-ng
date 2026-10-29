# Spec 038 clarification questions, round 2

Feature: `038-report-view-generators`, branch `038-report-view-generators`.
Method: the speckit-clarify discipline, second pass over the amended spec,
no spec edit. Round 1's five answers stand recorded in the spec's
Clarifications section and are not re-asked; the lane rulings R-1 to R-5
and the constitution stay out of scope. Each question below names a spot
where two reasonable readings of the amended spec still diverge with
materially different rendered output, and where no Decision, Requirement,
Assumption, Clarification entry or Edge Case already stands a default.
Three questions; the spec otherwise stands clear.


## Q1: The text view when one of its two figures answers the disclosure

**Spec section (quoted, FR-013 and User Story 6):**

> **FR-013**: The text view SHALL render the same figures the graphic
> views render as terminal text: the histogram as one line per bin
> naming the range and the count with a proportional bar, and the plot
> as one line per recorded value in row order (D-6, D-7).

> A user asks for a view whose record the run left absent: a benchmark
> hidden its repetition rows, a run captured no metric the request
> names, the page's identity field stands absent. The view answers
> `not available` naming the input, the driver prints the disclosure
> on standard output, and the exit status is 0.

**Question:** The round-1 ruling fixed the page's rule: the disclosure
text stands in the figure's place and the page answers the value form.
The text view composes the same two figures without the identity, and
the standing one-repetition column hits it: the histogram's part
discloses (fewer than two values, D-6) while the plot's part renders the
single point. Does the text view compose per part like the page, or does
the whole text view answer the disclosure on the error form? (FR-013,
FR-016)

**Why it matters:** the one-repetition column is a standing fixture
case, so the text view's answer form, its exit status through the
driver, and its fixtures all move with the answer.

| Option | Reading | Implications |
|---|---|---|
| A | The text view composes per part by the rule the page's ruling states: the histogram's lines stand replaced by the disclosure line, the plot's lines render in row order, and the whole answers the value form. | The two composite views (page, text) share one composition rule; the driver exits 0 with the plot's lines on standard output; the fixtures need one text-view gap case. |
| B | The whole text view answers the `not available` disclosure on the error form when either figure discloses. | A one-repetition column's recorded single point stands unreachable in the text view; the driver exits 0 but prints no plot lines; the composite views carry two different composition rules. |
| C | The text view refuses at the driver like a failed pre-check. | The record the run left absent is no request fault; FR-015's refusal list names the benchmark, the column and the view word; an absent figure stands outside it, so the refusal shape does not fit. |

**Recommended:** Option A. FR-013's own wording, the same figures the
graphic views render, puts each part under the rule its figure follows
alone, and the page's settled ruling already states that rule; two
composite views carrying two rules would need a distinction the spec
nowhere draws.


## Q2: The plot when rows stand but no recorded value does

**Spec section (quoted, FR-010 and D-7):**

> **FR-010**: The plot SHALL answer `not available` when the benchmark
> records no repetition row; a single recorded value renders one point
> (D-7).

> **D-7**: ... A row whose value stands unavailable stands out of the
> series, the rule D-6 states for the sample set.

**Question:** Round 1's first ruling removes a row whose value stands
unavailable from the series. When every repetition row of the requested
column stands unavailable (the availability tokens the record carries
instead of values), the series is empty while the rows stand recorded:
FR-010's trigger, no repetition row, is not met literally. Does the plot
answer the disclosure, or does it render the axes over an empty series?
(FR-009, FR-010, D-7)

**Why it matters:** a column the run recorded but never measured is a
standing fixture case for a lane whose records carry availability per
row, so the plot's disclosure rule and the fixtures move with the
answer.

| Option | Reading | Implications |
|---|---|---|
| A | The plot answers `not available` naming the column and the sample count of zero: the series the request names stands absent, the same rule D-6 states for the histogram's sample set. | The two figures share one currency, the recorded values; the empty series renders no shape the run did not measure; the fixtures need one all-gap case. |
| B | The plot renders the labeled axes with no points, the rows standing recorded. | The axes need a value range no recorded value bounds; the figure carries a frame over nothing, and the spec names no shape for an empty series. |
| C | The row joins the series as an absent slot: the plot draws a gap marker. | Round 1's ruling removed the gap marker for the single-gap case; the all-gap case re-introducing it would split one rule in two. |

**Recommended:** Option A. D-6 already fixed the currency for the
histogram, fewer than two recorded values names no shape, and D-7 names
the series over the same recorded values; a plot with no recorded value
has no shape to render, which is the disclosure's whole business.


## Q3: The format the standalone histogram view emits

**Spec section (quoted, FR-007 and US1):**

> **FR-007**: The histogram SHALL render one column's distribution over
> one benchmark from the repetition rows the document records; the bins
> SHALL cover the recorded value range, the bin count SHALL be the
> function's argument with the driver passing `20`, and the bars SHALL
> stand proportional to the counts (D-6).

> The answer carries one bin block per bin over the recorded value
> range, each block naming its range and its count, and no figure the
> document does not record.

**Question:** The spec fixes the plot's bytes, one SVG polyline (FR-009),
the page's bytes, one static HTML document (FR-011), and the text view's
bytes, terminal text (FR-013). The standalone histogram view's format is
unnamed: bin block and proportional bars fix the shape while the bytes stand free.
Does `--view=histogram` emit SVG, an HTML fragment, or text? (FR-007,
D-3)

**Why it matters:** the view's whole output moves with the answer, the
tests assert the bytes, and the page embeds the histogram inline, so the
embedded form and the standalone form must agree.

| Option | Reading | Implications |
|---|---|---|
| A | The histogram emits SVG, one bar per bin beside its range and count label, the figure the page embeds inline beside the plot's SVG. | The two graphic views share one format, the one D-3 names first; the page embeds two inline SVG figures; the standalone bytes and the page's embedded bytes stand equal. |
| B | The histogram emits an HTML fragment, one block element per bin. | The page's format serves a second, non-document output; D-8 reserves the HTML for the one static document, and the fragment's shape needs naming where no requirement carries one. |
| C | The histogram emits text, one line per bin with a character bar. | The output equals FR-013's text histogram, so `--view=histogram` and the histogram part of `--view=text` render the same bytes twice under two view words, and no graphic histogram stands for US4's cross-read. |

**Recommended:** Option A. D-3 names the SVG, the HTML and the text as
the three hand-rolled formats; the HTML is the page's one static
document and the text view's histogram part already owns the terminal
text, so the graphic histogram stands on the SVG, the plot's named
format and the figure the page embeds beside it.


## Considered and dropped

- The histogram of a multipass run pooling the repetition rows of all
  passes: D-7 fixes the pooling for the series and D-6's sample set is
  the same per-repetition values, so the reading stands carried.
- The degenerate all-equal range: the Edge Cases section states the
  outcome, the histogram renders one bin covering the value.
- Bin edges, label precision, axis tick marks, bar widths, the SVG
  document-versus-fragment spelling and the escape character set:
  plan-level details the acceptance text does not split on.
- The view functions' names and signatures: FR-004 fixes the argument
  shape, the public header and the answer type; the spelling is the
  plan's.
- The bin count of the histogram the page embeds: the view request
  carries the view's own arguments, and the driver's standing `20`
  covers every view it drives.
- Formats for `timeUnit`-less documents and other reader-domain gaps:
  the reader's contracts own the document's shape; the views own none of it.
