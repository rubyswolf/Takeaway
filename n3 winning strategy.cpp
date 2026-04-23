IF (current_move == 2)
{
    PICK(all_elements(~picked_on_move(1)));
}
WHILE_LEGAL {
    PICK(anything);
}
