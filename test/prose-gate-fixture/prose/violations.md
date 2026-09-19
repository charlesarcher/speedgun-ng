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
