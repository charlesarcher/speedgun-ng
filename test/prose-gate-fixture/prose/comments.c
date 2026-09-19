/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Comment-extraction fixture for the prose gate, tasks.md T004.
 * The C extractor yields comment text for both comment syntaxes and the
 * matcher scans that text for the Principle XI rules.
 * The harness asserts one XI1.EMDASH finding on the labeled line below
 * and no findings on the silent lines below.
 */

// XI1.EMDASH: The tick loop wakes a worker — the timer never spins idle.

// https://en.wikipedia.org/wiki/Speedgun

static const int kProseFixtureWorkers = 4;

int prose_fixture_add(int left, int right)
{
    return left + right;
}
