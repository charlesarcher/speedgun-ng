Docs: Mark Three Console Blocks as Shapes

Three fenced blocks of the harness page carry refusal lists and a
worked plan line with no sample-output tag, so the drift gate compares
none of them. That is how one of them carried a plan line missing two
fields until a hand fix. Each block now says in one clause that it
states the shape of the output, which is what a reader needs to know
about a block the gate cannot check.

Refs: specs/029-hunter-core

Approved-by: OmO <omo@speedgun.local>
