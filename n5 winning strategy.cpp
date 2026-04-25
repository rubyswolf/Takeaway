Condition previous_move_was_triple = number_of_elements(picked_on_move(previous_move)) == 3;
MoveTest triple_move = anything & any_from(3, pass);
MoveTest pair_move = anything & any_from(2, pass);
MoveTest compliment_move = all_elements(~picked_on_move(previous_move));

MoveTest singleton_move = anything & any_from(1, pass);
ElementTest covered = picked_in_any(only_from(~are_singleton));
ElementTest exposed = ~are_singleton & ~covered;
Condition full_coverage = !there_is_an_element(exposed);

IF (current_move == 2)
{
    PICK(compliment_move, "Compliment move");
}
IF (previous_move_was_triple) {
    PICK(triple_move, "Any triple");
}
IF (full_coverage.ever_after(singleton_move)) {
    PICK(singleton_move.such_that(full_coverage), "Strong reduction");
}
IF (is_legal(pair_move))
{
    PICK(pair_move, "Any pair");
}
PICK(anything, "Any legal move");