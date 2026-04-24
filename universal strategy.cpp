ElementIntExpr coverage = only_from(~are_singleton).times_picked;
ElementTest leastCovered = ~are_singleton & (coverage == min(coverage, ~are_singleton));

PICK(all_elements(leastCovered), "Cover all elements that are the least covered to try to reduce the game");
