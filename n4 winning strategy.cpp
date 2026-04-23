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

Condition bigCase =
    (move3HasBoth && !move3HasNeither)
    || move3HasSize3;

Condition swappedCase = (move3HasOnly1 != move3HasOnly2);

ElementTest bit1 =
    both.when(swappedCase)
    .otherwise(only2);

ElementTest bit2 =
    only2.when(swappedCase && move3HasOnly1)
    .otherwise(
        only1.when(swappedCase && move3HasOnly2)
        .otherwise(only1)
    );

ElementTest bit3 =
    only1.when(swappedCase && move3HasOnly1)
    .otherwise(
        only2.when(swappedCase && move3HasOnly2)
        .otherwise(both)
    );

ElementTest bit4 = neither;

Condition needMove6 =
    (number_of_elements(picked_on_move(5)) != 1)
    || !there_is_an_element(picked_on_move(5) & neither);

Condition move5HasBit1 = there_is_an_element(picked_on_move(5) & bit1);
Condition move5HasBit2 = there_is_an_element(picked_on_move(5) & bit2);
Condition move5HasBit3OrBit4 = there_is_an_element(picked_on_move(5) & (bit3 | bit4));

MoveTest move6 =
    all_elements(bit4).when(move5HasBit3OrBit4).otherwise(nothing)
    |
    all_elements(bit2).when(move5HasBit1).otherwise(nothing)
    |
    all_elements(bit1).when(move5HasBit2).otherwise(nothing)
    |
    (~everything).when(!move5HasBit3OrBit4).otherwise(nothing);

IF (move1HasSize1) {
    IF (current_move == 2)
    {
        PICK(all_elements(~picked_on_move(1)));
    }
    IF (current_move == 4)
    {
        PICK(all_elements(~picked_on_move(1) & ~picked_on_move(3)));
    }
}

IF (move1HasSize3) {
    IF (current_move == 2)
    {
        PICK(all_elements(~picked_on_move(1)));
    }
    IF (current_move == 4)
    {
        PICK(all_elements(picked_on_move(1) & ~picked_on_move(3)));
    }
}

IF (move1HasSize2) {
    IF (current_move == 2) {
        PICK(
            any_from(1, picked_on_move(1))
            &
            any_from(1, ~picked_on_move(1))
        );
    }

    IF (bigCase) {
        IF (current_move == 4)
        {
            PICK(all_elements(~picked_on_move(3)));
        }
        PICK(all_elements(~both) & all_elements(~picked_on_move(5)));
    }
    ELSE {
        IF (move3HasSize1) {
            PICK(
                all_elements(only1).when(move3HasNeither)
                .otherwise(all_elements(neither))
            );
        }
        ELSE {
            PICK(
                (all_elements(only1) | all_elements(only2))
                .when(swappedCase)
                .otherwise(all_elements(~picked_on_move(3)))
            );

            IF (needMove6) {
                PICK(move6);
            }
        }
    }
}

WHILE_LEGAL {
    PICK(anything);
}
