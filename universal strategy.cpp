WHILE_LEGAL {
  IF ((number_of_elements(picked_on_move(previous_move)) == 1) && has_been_played(all_elements(~is_singleton)))
  {

  }
  ELSE {
    PICK(all_elements(~picked_on_move(previous_move)));
  }
}