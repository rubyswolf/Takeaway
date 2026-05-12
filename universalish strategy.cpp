MoveTest cover = only_from(~are_singleton);
ElementTest covered = picked_in_any(cover);
ElementTest exposed = ~are_singleton & ~covered;
Condition full_coverage = !there_is_an_element(exposed);
MoveTest singletonMove = legal & any_from(1, pass);
MoveTest subgame_move = cover & ~all_elements(~are_singleton);
Condition even_subgame = (number_of_moves(subgame_move) % 2) == 0;
Condition no_even_counter_reduction_possible = (!(full_coverage && even_subgame)).always_after(singletonMove);
Condition even_reduction = full_coverage && even_subgame;
Condition safe_even_reduction = even_reduction && no_even_counter_reduction_possible;
ElementIntExpr coverage = cover.times_picked;
ElementTest leastCovered = ~are_singleton & (coverage == min(coverage, ~are_singleton));
MoveTest coveringMove = all_elements(leastCovered);
MoveTest non_singletons_compliment = all_elements(~are_singleton & ~picked_on_move(previous_move));


IF (safe_even_reduction.ever_after(singletonMove)) {
   PICK(singletonMove & causes(safe_even_reduction), "Introduce a new singleton that causes a reduction");
}
IF (safe_even_reduction.ever_after(coveringMove))
{
   PICK(coveringMove & causes(safe_even_reduction), "Cover all elements that are the least covered");
}
IF (is_legal(non_singletons_compliment))
{
   PICK(non_singletons_compliment, "Non singleton's compliment");
}
IF (no_even_counter_reduction_possible.ever_after(legal))
{
   PICK(legal & causes(no_even_counter_reduction_possible), "Prevent counter reduction");
}
PICK(legal, "Any legal move");
