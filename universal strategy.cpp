ElementTest covered = picked_in_any(only_from(~are_singleton));
ElementTest exposed = ~are_singleton & ~covered;
Condition full_coverage = !there_is_an_element(exposed);
MoveTest singletonMove = anything & any_from(1, pass);
Condition safe_full_coverage = full_coverage && (!full_coverage).always_after(singletonMove);

IF (safe_full_coverage.ever_after(singletonMove)) {
   PICK(singletonMove.such_that(safe_full_coverage), "Introduce a new singleton that causes a reduction");
}
IF (!full_coverage) {
   PICK(all_elements(exposed), "Cover all exposed elements to create full coverage");
}