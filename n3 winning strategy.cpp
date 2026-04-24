IF (current_move == 2)
{
    PICK(all_elements(~picked_on_move(1)), "Compliment move");
}
WHILE_LEGAL {
    PICK(anything, "Arbitrary legal move");
}
