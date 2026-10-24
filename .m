Docs: Name the Module the Gate Scripts Import

The quickstart hands out the dbc-gate step and lists doxygen, lcov and
gcov as the tools it needs, while the shared gate helper refuses the
run outright when the interpreter the build selected carries no yaml
module. A reader walking the page met that refusal with no warning, so
the prerequisite line now names the module the two gate scripts import
through that helper.

Refs: specs/029-hunter-core

Approved-by: OmO <omo@speedgun.local>
