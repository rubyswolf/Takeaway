MoveTest cover = only_from(~are_singleton);
ElementTest covered = picked_in_any(cover);
ElementTest exposed = ~are_singleton & ~covered;
Condition full_coverage = !there_is_an_element(exposed);
MoveTest singletonMove = anything & any_from(1, pass);
Condition safe_full_coverage = full_coverage && (!full_coverage).always_after(singletonMove);
ElementIntExpr coverage = cover.times_picked;
ElementTest leastCovered = ~are_singleton & (coverage == min(coverage, ~are_singleton));
MoveTest coveringMove = all_elements(leastCovered);

IF (safe_full_coverage.ever_after(singletonMove)) {
   PICK(singletonMove.such_that(safe_full_coverage), "Introduce a new singleton that causes a reduction");
}
IF (safe_full_coverage.ever_after(coveringMove))
{
   PICK(coveringMove.such_that(safe_full_coverage), "Cover all elements that are the least covered");
}
PICK(anything, "Pick any legal move as a last resort");