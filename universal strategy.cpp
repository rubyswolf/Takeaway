Condition previous_move_was_singleton = number_of_elements(picked_on_move(previous_move)) == 1;
IntExpr number_of_singletons = number_of_elements(all_elements(are_singleton));
MoveTest all_but_one_singleton = any_from(number_of_singletons-1, ~are_singleton);
IntExpr move_with_all_but_one_singleton = move_where(all_but_one_singleton);

WHILE_LEGAL {
  IF (previousMoveWasSingleton && has_been_played(all_but_one_singleton))
  {
    PICK(all_elements(~are_singleton & ~picked_on_move(move_with_all_but_one_singleton)));
  }
  ELSE {
    PICK(all_elements(~picked_on_move(previous_move)));
  }
}