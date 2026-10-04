# Prose gate violation fixture

Deliberate violations for the prose gate fixtures, one per line, each
labeled with the rule id it exercises. The harness asserts one finding for
each labeled line.

XI1.EMDASH: The runner drains the ready queue — the executor never blocks.
XI1.DOUBLE-HYHEN: The waiter spins -- forever when every worker slot is busy.
XI2.CONTRASTIVE: The timer wheel is a calendar queue, not a binary heap.
XI2.CONTRASTIVE: The executor prefers spinning rather than sleeping on a condvar.
XI2.CONTRASTIVE: The harness pins worker threads instead of migrating them.
XI3.VOUCHER: Frankly, the measurements drift under sustained thermal load.
XI4.META-EDITORIALIZING: In this section we describe how the harness fires.
XI5.FILLER: The wheel advances in order to fire the nearest bucket.
XI5.MARKETING: The report calls the median estimator robust under tail load.
XI5.FILLER: The gate reports the finding in addition to the summary line.
XI5.FILLER: The parser keeps the token as well as its rule id.
XI5.FILLER: The summary lists the families and so on.
XI5.FILLER: Two fixtures share one harness file in the same way.
XI5.FILLER: The loader stops due to the fact that a probe fails.
XI5.FILLER: The gate exits early as a result of a bad token.
XI5.FILLER: The harness checks two compilers similarly.
