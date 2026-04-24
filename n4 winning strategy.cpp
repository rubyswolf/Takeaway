ElementTest both    = picked_on_move(1) & picked_on_move(2);
ElementTest only1   = picked_on_move(1) & ~picked_on_move(2);
ElementTest only2   = ~picked_on_move(1) & picked_on_move(2);
ElementTest neither = ~picked_on_move(1) & ~picked_on_move(2);

Condition move1HasSize1 = (number_of_elements(picked_on_move(1)) == 1);
Condition move1HasSize2 = (number_of_elements(picked_on_move(1)) == 2);
Condition move1HasSize3 = (number_of_elements(picked_on_move(1)) == 3);

Condition move3HasSize1 = (number_of_elements(picked_on_move(3)) == 1);
Condition move3HasSize3 = (number_of_elements(picked_on_move(3)) == 3);

Condition move3HasBoth    = there_is_an_element(picked_on_move(3) & both);
Condition move3HasOnly1   = there_is_an_element(picked_on_move(3) & only1);
Condition move3HasOnly2   = there_is_an_element(picked_on_move(3) & only2);
Condition move3HasNeither = there_is_an_element(picked_on_move(3) & neither);

Condition move3IsThreeOnePair =
    (move3HasBoth && !move3HasNeither)
    || move3HasSize3;

Condition move3OnlysDifferent = (move3HasOnly1 != move3HasOnly2);

ElementTest swap1 =
    both.when(move3OnlysDifferent)
    .otherwise(only2);

ElementTest swap2 =
    only2.when(move3OnlysDifferent && move3HasOnly1)
    .otherwise(
        only1.when(move3OnlysDifferent && move3HasOnly2)
        .otherwise(only1)
    );

ElementTest offOr =
    only1.when(move3OnlysDifferent && move3HasOnly1)
    .otherwise(
        only2.when(move3OnlysDifferent && move3HasOnly2)
        .otherwise(both)
    );

Condition needMove6 = !(number_of_elements(picked_on_move(5)) == 1 && there_is_an_element(picked_on_move(5) & neither));

Condition move5HasSwap1 = there_is_an_element(picked_on_move(5) & swap1);
Condition move5HasSwap2 = there_is_an_element(picked_on_move(5) & swap2);
Condition move5HasOr = there_is_an_element(picked_on_move(5) & (offOr | neither));

ElementTest move6Elements =
    swap1.when(move5HasSwap2).otherwise(fail)
    |
    swap2.when(move5HasSwap1).otherwise(fail)
    |
    neither.when(move5HasOr).otherwise(fail);

MoveTest move6 = all_elements(move6Elements);

IF (move1HasSize1) {
    IF (current_move == 2)
    {
        PICK(all_elements(~picked_on_move(1)), "Move 2 for size 1");
    }
    IF (current_move == 4)
    {
        PICK(all_elements(~picked_on_move(1) & ~picked_on_move(3)), "Move 4 for size 1");
    }
}

IF (move1HasSize3) {
    IF (current_move == 2)
    {
        PICK(all_elements(~picked_on_move(1)), "Move 2 for size 3");
    }
    IF (current_move == 4)
    {
        PICK(all_elements(picked_on_move(1) & ~picked_on_move(3)), "Move 4 size 3");
    }
}

IF (move1HasSize2) {
    IF (current_move == 2) {
        PICK(
            any_from(1, picked_on_move(1))
            &
            any_from(1, ~picked_on_move(1)),
            "Crossing move"
        );
    }

    IF (move3IsThreeOnePair) {
        IF (current_move == 4)
        {
            PICK(all_elements(~picked_on_move(3)), "3 1 Pair 3s Compliment");
        }
        IF (current_move == 6)
        {
            PICK(all_elements(~both) & all_elements(~picked_on_move(5)), "3 1 Pair 5s Compliment");
        }
    }
    ELSE {
        IF (move3HasSize1) {
            IF (current_move == 4)
            {
                PICK(
                    all_elements(only1).when(move3HasNeither)
                    .otherwise(all_elements(neither)),
                    "Move 3 size 1"
                );
            }
        }
        ELSE {
            IF (current_move == 4)
            {
                PICK(
                    (all_elements(only1) & all_elements(only2))
                    .when(move3OnlysDifferent)
                    .otherwise(all_elements(~picked_on_move(3))),
                    "Move for when move 3 not size 1"
                );
            }

            IF (current_move == 6 && needMove6) {
                PICK(move6, "Move 6");
            }
        }
    }
}

WHILE_LEGAL {
    PICK(anything, "Arbitrary legal move");
}