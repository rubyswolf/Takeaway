Condition previous_move_was_singleton = number_of_elements(picked_on_move(previous_move)) == 1;
MoveTest all_but_one_non_singleton = all_but(1, ~are_singleton);

IF (number_of_elements(are_singleton) == 0) {
  // Always the compliment seems to force player one to eventually play a singleton
  PICK(all_elements(~picked_on_move(previous_move)), "Play the compliment");
}

IF (has_been_played(all_but_one_non_singleton))
{
  PICK(all_elements(~are_singleton & ~picked_on_move(move_where(all_but_one_non_singleton))), "Pick last singleton to reduce");
}

PICK(all_elements(~are_singleton), "Cover all non-singletons to reduce");