// The Takeaway Challenge
// This is a two player game where you start with a finite universal set E
// For example with n=4 so we have the universal set E = {1, 2, 3, 4}
// To make a move you chose a subset of E, but you can't pick the empty set or the full set E itself
// So for example you could pick {1, 2} but not {} or {1, 2, 3, 4}
// You may not pick a subset of E that contains ANY of the subsets that have already been picked
// For example player one may pick {1} but then player two may not pick {1, 2} because it contains the subset {1} that player one has already picked
// The player who cannot make a move loses
// For n=2 with E={1,2} player one could play {1}, then player two could play {2}, then player one would have no moves left and would lose while player two would win
// The question is who has the winning strategy for n=3 and n=4 and what is that strategy?
// I aimed to solve this via a brute force search

#include <algorithm> // This lets us reorder and erase collections with helpers like remove_if
#include <iostream> // This library lets us output to the console window that will be attached to the program when we run it
#include <fstream> // This lets us write to output to files
#include <vector> // This allows us to create dynamically sized lists of items called vectors
#include <set> // This lets us define sets of items
#include <string> // This allows us to work with strings of text

// Let's define elements of E to be integers
typedef int Element; // Define elements of E to be integers

// We could represent moves as sets of elements as you might expect
// typedef std::set<Element> Move;

// But there's a much better and more computationally efficient way to represent moves, bitmasks
// The idea is to have a binary number where each digit is either 1 for the element being in the set or 0 for it not being in the set
// For example for n=4 we could have 0101 to represent the set {1, 3} because the first and third digits are 1s so we select the first and third element
typedef int Move; // This way we can define moves to be numbers! (bitmask integers)

// Now we need to define our universal set E
struct UniversalSet {
	int size; // The size of the universal set

	int bitmask; // The bitmask corresponding to the full set E

	// The bitmask for E will look like 1111... (all 1s) because every element of E is in the set
	// So we take a 1, bit shift it to the left |E| binary places and subtract 1 to get |E| 1s in binary, which is exactly the full set bitmask we want
	// This might feel strange because it's binary (base 2) but it's easy to understand if I show the equivalent in decimal (base 10):
	// Take for example 10^3=1000 (10^n rather than 2^n) then when we subtract 1 for 1000-1 we get 999 which is the number with 3 9s

	// When we create a new universal set, we create it from a given size and then construct the full set bitmask
	UniversalSet(int size) : size(size), bitmask((1<<size)-1) {};
};

// Let's define some ways we can manipulate moves
namespace ManipulateMove
{
	// Flip every bit in the mask to get the compliment
	// That is, every item that was in the set is now out of the set and every item that was out of the set is now in the set
	// This is equivalent to the operation E\move in set theory
	inline Move compliment(UniversalSet E, Move move)
	{
		// Doing 1 - x flips a bit because 1 - 0 = 1 and 1 - 1 = 0
		// Assuming we have a full number of 1s like 1111...,
		// if we subtract some number then for each digit it will look like 1-x
		// There is no carry to worry about as a single digit cannot subtract more than 1 and we're taking from 1
		// The full set bit mask is exactly this 1111... we're after
		return E.bitmask - move; // Subtracting the move mask from the full E bitmask gives the compliment
	}

	// Returns 1 if the move set contains the element at the given index, and 0 otherwise
	// Note that computers count in binary starting from zero so the first element is actually element 0, second is 1 ...
	inline Move hasElement(Move move, int elementIndex)
	{
		// To extract a specific bit (element) from the mask
		// we first bit shift to the right by how ever many binary places we need
		// So for example 1011 shifted right once is 101 and twice is 10 and three times is 1
		// So at each shift we simply shave off the right most digit
		// This is represented with >> so 1011 >> 1 = 101 (shave off one digit)
		// Then finally we bitwise and it with 1 to extract the final bit
		// for example abcd & 0001 = 000d = d so we extract the final bit (Because anything AND 0 is 0 and anything AND 1 is that something)
		// So if we shift and then extract the last bit then we can extract any specific bit we'd like:
		return (move >> elementIndex) & 1; // Shift the mask right by n positions and then perform a bitwise AND with 1 to get the value of the bit at position n
	}

	// Convert the move from a bitmask to a text format like "3 = {1, 2}"
	inline std::string toString(UniversalSet E, Move move) {
		std::string result = std::to_string(move) + " = {"; // Start the string with the move number and an opening brace for the set representation
		for (int i = 0; i < E.size; i++) { // For each element in the universal set E
			if (hasElement(move, i)) { // If this element is in the move
				result += std::to_string(i + 1) + ", "; // Add the element (add 1 to convert from 0-indexed to 1-indexed) and a comma and space to separate the elements
			}
		}
		if (result.size() > 2) { // If we added any elements to the string
			result.pop_back(); // Remove the last space
			result.pop_back(); // Remove the last comma
		}
		result += "}"; // Add a closing brace for the set representation
		return result; // Return the final string representation of the move
	}

	// Convert the move to a string of empty circles for not selected items ○ and filled circles for selected items ● for player one
	// and filled or not filled squares for player two □/■
	// with the least significant bit as the first character in the string and the most significant bit as the last character in the string
	inline std::string toSymbols(UniversalSet E, Move move, bool isPlayerOnesTurn) {

		std::string result = ""; // Start with an empty string to build the result
		for (int i = 0; i < E.size; i++) { // For each element in the universal set E
			if (hasElement(move, i)) { // If this element is in the move
				result = result + (isPlayerOnesTurn ? u8"●" : u8"■"); // Add a filled symbol
			}
			else { // Otherwise if this element is not in the move
				result = result + (isPlayerOnesTurn ? u8"○" : u8"□"); // Add an unfilled symbol
			}
		}
		return result; // Return the final string representation of the move
	}

	// Convert the move into a full line of text like "P1: ●○○○ (1 = {1})"
	// This is useful when we want to print a move in a readable way that contains the player number, symbols, bitmask number and set
	inline std::string moveLine(UniversalSet E, Move move, int moveNumber, bool isPlayerOnesTurn) {
		return
			"P" + std::to_string(isPlayerOnesTurn ? 1 : 2) + // Player indicator
			" move " + std::to_string(moveNumber) + ": " + // Move number
			toSymbols(E, move, isPlayerOnesTurn) + // Symbols
			" (" + toString(E, move) + ")"; // Set and bitmask number
	}
};

// This game has a lot of symmetry
// This simply comes from the fact that the names of the elements themselves don't matter
// Only the structure is important
// Say we have E={1, 2}
// Then there's really no difference between making the move {1} or the move {2}
// Since it really just means "player one picks an element"
// And then player two might "pick just the other element"
// And both of those games which are {1}, {2} and {2}, {1} are really just the same thing
// But with elements having different names
// You can get from one game to another just be relabeling the elements
// In this case you rename 1 to 2 and 2 to 1
// So I consider them symmetrically equivalent
// 
// So to eliminate the symmetry here we need a notion of a "principal"
// That is for a move to be considered "the one true move" or a game to be "the one true game"
// And every other move or game to just be considered a symmetric copy of the principal
// Since we're working with binary bitmasks here, moves are just numbers
// a natural choice is simply to choose the smallest number as the principal
// So the principal move will be the smallest move out of a symmetric group
// And the principal game will be the smallest game (make the smallest possible move at each choice)
// To distinguish the principal, we will have the elements being letters
// So out of {1}, {2} and {2}, {1}
// {1}, {2} is the principal game and it is represented as {A}, {B}
// 
// To actually do this in practice, we need a proper definition of what a "symmetry" is
// Here's how I'll define it:
// The "symmetry" of a game state/position is a list of "interchangables"
// An "interchangable" is a set of elements which are interchangable with one another,
// that means you can swap their labels around freely without changing the underlying game
// For example in the inital game state at n=4 then the symmetry is [{1,2,3,4}]
// This is because 1, 2, 3 and 4 are all interchangable through relabelling, doesn't matter which ones you pick
// Say player one picks {1}
// Now that gives meaning to 1, we can fully specify it as "the element that player one picked on their first move"
// And 2, 3 and 4 are all an interchangable group meaning "Any of the three elements player one didn't pick at the start"
// So the symmetry of this new state is now [{1}, {2,3,4}]
//
// If player one were to pick {1,2} from the start, the symmetry would instead be [{1,2}, {3,4}]
// 1 and 2 are interchangable as "one of the two elements player one picked on their first move"
// 3 and 4 are interchangable as "one of the two elements player didn't pick on their first move"

// So we break elements up into interchangables
// based on how you can specify them from what moves they were and weren't selected in

// Let's definine these with code:
typedef std::set<Move> Interchangable;
typedef std::vector<Interchangable> Symmetry;

// Let's define what a game is, it will simply be a list of moves
class Game : public std::vector<Move> // This colon (:) here means extends, so a Game is a vector of moves but with our own extra features
{
public:
	Game(UniversalSet E) : E(E) {
		// Initialize the symmetry of the game
		// At the start of the game all elements are interchangable so we have one interchangable set containing all elements
		Interchangable initialInterchangable; // Create a new interchangable to represent the initial interchangables
		for (int i = 0; i < E.size; i++) { // For each element in the universal set E
			initialInterchangable.insert(i); // Add it to the initial interchangable set
		}
		gameSymmetry.push_back(initialInterchangable); // Add this initial interchangable set to the game's symmetry
	}

	UniversalSet E; // The universal set for the game
	Symmetry gameSymmetry; // The current symmetry of the game

	void playMove(Move move) {

		// DEBUGGING: Print out the move being made
		//std::cout << "Playing move: " << ManipulateMove::toString(move) << std::endl;
		// END DEBUGGING

		this->push_back(move); // Add the move to the list of moves

		// Now we need to update the symmetry
		// The way an element can be "specified"
		// Is through what moves it was or wasn't selected in
		// This new move will categorize the elements depending on whether or not they were selected
		// So if we have elements within the same interchangable that are categorized differently by the new move
		// they are no longer interchangable and their symmetry is broken

		Symmetry newSymmetry; // A new symmetry to store the updated symmetry after making the move

		// Loop over each interchangable in the current symmetry
		for (Interchangable interchangable : gameSymmetry) {
			// We will split this interchangable into two new interchangables based on whether or not the elements were selected in the new move
			Interchangable selected; // A new interchangable to store the elements that were selected in the new move
			Interchangable notSelected; // A new interchangable to store the elements that were not selected in the new move

			for (Move element : interchangable) { // For each element in the current interchangable
				if (ManipulateMove::hasElement(move, element)) { // If this element was selected in the new move
					selected.insert(element); // Add it to the selected interchangable
				}
				else { // Otherwise if this element was not selected in the new move
					notSelected.insert(element); // Add it to the notSelected interchangable
				}
			}

			// Now we have two new interchangables to reintroduce to our symmetry
			// But we only want to add them if they are not empty
			if (!selected.empty())
			{
				newSymmetry.push_back(selected); // Add the selected interchangable to our symmetry
			}
			if (!notSelected.empty())
			{
				newSymmetry.push_back(notSelected); // Add the notSelected interchangable to our symmetry
			}
		}

		// Finally we update the game's symmetry to be the new symmetry we just calculated
		gameSymmetry = newSymmetry;
	}

	// Get ALL moves, including impossible moves such as the empty set and full set E
	std::vector<Move> allMoves() const {
		std::vector<Move> allMoves; // A list (vector) to store all possible moves that we will generate

		for ( // Start a new loop for all possible moves
			int candidateMove = 0; // The loop starts at 0 for the empty set
			candidateMove <= E.bitmask; // Loop ends at the full set E
			candidateMove++) { // At each step we simply add 1 to the candidate move which will iterate through all possible move bitmasks in ascending order
			allMoves.push_back(candidateMove);
		}
		return allMoves;
	}

	// Get all possible moves (legal or not) without taking into account the current position or the rules of the game
	// This is static so we can call it without referencing a specific game
	std::vector<Move> allPossibleMoves() const {
		std::vector<Move> allPossibleMoves; // A list (vector) to store all possible moves that we will generate

		for ( // Start a new loop for all possible moves
			int candidateMove = 1; // The loop starts at 1 (...0001) because we can't pick the empty set which is 0 (...0000)
			// Loop ends at 2^n-2 (...1110)
			candidateMove < E.bitmask; // This condition will fail once we hit E (2^n-1) because then it will be equal and not less than
			// This means the loop terminates without running for E since we can't choose the full set, but it will run for all other possible moves from 1 to E-1
			candidateMove++) { // At each step we simply add 1 to the candidate move which will iterate through all possible move bitmasks in ascending order
			allPossibleMoves.push_back(candidateMove);
		}
		return allPossibleMoves;
	}

	// Get only the principal moves from this position
	std::vector<Move> principalMoves(int interchangableIndex = 0, std::vector<int> elements = {}) const {
		// When creating a principal move we can chose any number of elements from each interchangable
		// We can choose anywere from 0 to all of the elements in each interchangable
		// So we can generate this recursively by first choosing how many elements we want from the first interchangable
		// then how many from the second and so on until we have made a choice for each interchangable
		// And then we combine all possible choices together to get the full list of principal moves
		// This needs to be done depth first so that we generate the moves in ascending order (smallest moves first)

		if (interchangableIndex == gameSymmetry.size()) { // If we've reached the bottom, we have a complete move to return
			Move move = 0; // Start with an empty move

			for (int i = 0; i < gameSymmetry.size(); i++) { // For each interchangable
				for (int j = 0; j < elements[i]; j++) { // For each element we selected from this interchangable
					// Shift 1 to the left by the index of the element in the interchangable to get the bitmask for that element and add it to the move
					move += 1 << *std::next(gameSymmetry[gameSymmetry.size() - i - 1].begin(), j);
				}
			}

			if (move == 0 || move == E.bitmask) { // We can't pick the empty set or the full set E itself
				return {}; // Return an empty vector to indicate that this is not a valid move
			}
			return { move }; // Return this move as a single element vector
		}
		else { // Otherwise we need to select how many elements we want from this interchangable and then recursively generate the moves for the next interchangable
			std::vector<Move> moves; // A vector to store the generated principal moves
			size_t interchangableSize = gameSymmetry[gameSymmetry.size() - interchangableIndex - 1].size(); // The number of elements in this interchangable
			// Note the index here is backwards to make the choice for the last interchangable first and work backwards

			// For each possible number of selected elements from this interchangable (from 0 to all)
			for (int selectedElements = 0; selectedElements <= interchangableSize; selectedElements++) {
				std::vector<int> newElements = elements; // Create a new vector to represent the number of selected elements so far
				newElements.push_back(selectedElements); // Add the number of selected elements from this interchangable to the vector
				std::vector<Move> subMoves = principalMoves(interchangableIndex + 1, newElements); // Recursively generate the moves for the next interchangable with the updated selected elements vector
				moves.insert(moves.end(), subMoves.begin(), subMoves.end()); // Add the generated moves to the main list of moves
			}
			return moves; // Return the generated moves
		}
	}

	// This function checks whether a single candidate move is legal or not
	bool isMoveLegal(Move candidateMove) const {

		// First, we rule out some simple illegal cases

		// The empty set is not allowed
		if (candidateMove == 0) {
			return false;
		}

		// The full set E is also not allowed
		if (candidateMove == E.bitmask) {
			return false;
		}

		// Now we check the main rule of the game

		// A move is illegal if ANY of the previously chosen sets is a subset of the candidate move set
		// So we just loop over all of the previous moves to see if this is the case for any of them:

		for (Move previouslyChosenSet : *this) { // Here "*this" means "this game"

			// We need to check if this previously chosen set is contained inside the candidate move
			// For a set to be a subset of another, every element in the smaller set must also appear in the larger one

			// Since we are using bitmasks, this condition can be checked very simply:
			// If (previouslyChosenSet & candidateMove) == previouslyChosenSet,
			// then every bit set in previouslyChosenSet is also set in candidateMove
			// So previouslyChosenSet is a subset of candidateMove

			if ((previouslyChosenSet & candidateMove) == previouslyChosenSet) {
				// This means the candidate move is illegal
				return false; // We can immediately return since we already found a violation
			}
		}

		// If none of the previous moves were subsets, then the move is legal
		return true;
	}

	// Now we have a list of all possible moves but many of them will be illegal according to the rules of the game
	// So we need a method to filter out the illegal moves and only keep the legal ones
	std::vector<Move> removeIllegalMoves(std::vector<Move> candidateMoves) const {

		std::vector<Move> legalMoves; // A list (vector) to store only the legal moves that make it through the filtering

		// We'll try every candidate move and only keep it if it's legal
		for (Move candidateMove : candidateMoves) {

			// We will check if this candidate move is illegal according to the rules of the game
			// If it is illegal then we will discard it and move on to the next candidate move
			// Otherwise it must be legal so we will add it to our list of legal moves

			if (isMoveLegal(candidateMove)) {
				legalMoves.push_back(candidateMove);
			}
		}

		// Now finally once we have the full list of legal moves, we return it:
		return legalMoves;
	}

	// A method to get the legal moves from the current position
	std::vector<Move> legalMoves() const {
		return removeIllegalMoves(allPossibleMoves()); // We can just get all possible moves and then filter out the illegal ones to get the legal moves
	}

	// A method to get the principal legal moves from the current position
	std::vector<Move> principalLegalMoves() const {
		return removeIllegalMoves(principalMoves()); // We can just get the principal moves and then filter out the illegal ones to get the principal legal moves
	}
};

class MoveNode
{
public:
	UniversalSet E; // The universal set

	MoveNode* parent = nullptr; // The previous game position that led to this one
	Move move; // The move that was made to get to this position from the parent position
	std::vector<MoveNode*> children; // The possible moves from this position that lead to new games
	bool isWinningNode; // A flag to indicate whether this is a winning node or not

	bool playerOneAlwaysWins; // Whether player one always wins from this position (forced)
	bool playerTwoAlwaysWins; // Whether player two always wins from this position (forced)

	bool playerOneCanAlwaysWin; // Whether player one could always win if they play perfectly
	bool playerTwoCanAlwaysWin; // Whether player two could always win if they play perfectly

	// Note that CAN always win is different from ALWAYS wins
	// 
	// Always wins means that the win is forced, no matter what you do
	// 
	// But when we assume perfect play we also want to track cases where a win is always possible but not forced
	// That is, you could lose the game if you played poorly but would always win if you played perfectly

	MoveNode(Game position, int* totalNodes = nullptr, Move move = 0, int depth = 0, bool isPlayerOnesTurn = true, bool isWinningNode = false) : E(position.E), move(move), isWinningNode(isWinningNode) {

		if (totalNodes != nullptr) // If we are tracking how many nodes we are generating
		{
			if (*totalNodes % 10000 == 0) { // Every 10,000 nodes we generate, print out how many nodes we have generated so far to keep track of our progress
				std::cout << "Generated " << *totalNodes << " nodes, currently at: ";
				for (Move positionMove : position) { // Print out the moves that led to this position to see how we got here
					std::cout << std::to_string(positionMove) << ((positionMove != position.back()) ? "," : ""); // Print the move and an arrow to separate the moves except for the last move in the position
				}
				std::cout << std::endl;
			}

			(*totalNodes)++; // Increment our total node count to keep track of how many nodes we have generated in our game tree
		}

		if (isWinningNode) { // If this is a winning node then we don't need to calculate any children since we already know the player can win from this position
			//std::cout << "Found a game where player " << (isPlayerOnesTurn ? "one" : "two") << " wins" << std::endl; // Print out the winning move that leads to this winning node

			// A winning node is a leaf node that has no valid moves that can be made from this position
			// So the other player always wins (or rather they have won):
			playerOneAlwaysWins = !isPlayerOnesTurn;
			playerTwoAlwaysWins = isPlayerOnesTurn;

			// Neither player can always win from this position if they play perfectly
			// This is because a player has already won
			playerOneCanAlwaysWin = false;
			playerTwoCanAlwaysWin = false;
			//playerOneCanAlwaysWin = !isPlayerOnesTurn;
			//playerTwoCanAlwaysWin = isPlayerOnesTurn;

			return; // So we can just return early and not calculate any children
		}

		// Calculate legal moves from this position and create child nodes for each of those moves
		// But we'll exit early if the player has a winning move and we'll have them simply play it
		std::vector<Move> legalMoves = position.principalLegalMoves(); // Get the principal legal moves from this position

		// Assume both players are forced to always win from this position
		// We set this false if there exists any way for the player to not win
		playerOneAlwaysWins = true;
		playerTwoAlwaysWins = true;

		// Assume both players cannot always win from this position by playing a specific move
		// We set this true if it turns out that the current player can always win if one or more specific moves are played
		bool playerOneCanAlwaysWinBySomeMove = false;
		bool playerTwoCanAlwaysWinBySomeMove = false;

		// Assume both players can win by force
		// That means that no matter what their opponent does in response, they still always have a way to win
		// We set this false if there IS a response the other player can make so that we can't always win
		bool playerOneCanAlwaysWinByForce = true;
		bool playerTwoCanAlwaysWinByForce = true;

		// A temporary vector to store the child nodes we create for each legal move so that we can check if any of them are winning nodes before we add them to our main list of children
		std::vector<MoveNode*> collectedChildren;

		for (Move move : legalMoves) { // For each legal move
			Game newPosition = position; // Create a new game position based on the current position
			newPosition.playMove(move); // Play the move to get the new position

			if (newPosition.principalLegalMoves().empty()) { // If there are no legal moves from this new position
				// Then playing that move causes us to win immediately
				// Play it and save it as a winning node
				children.push_back(new MoveNode(newPosition, totalNodes, move, depth + 1, !isPlayerOnesTurn, true));

				// By definition the current player can always win (by playing this move)
				playerOneCanAlwaysWin = isPlayerOnesTurn;
				playerTwoCanAlwaysWin = !isPlayerOnesTurn;

				// And since we're assuming that a player will always play a winning move when they see one
				// We can say that the current player always wins from this position because they always play that winning move
				playerOneAlwaysWins = isPlayerOnesTurn;
				playerTwoAlwaysWins = !isPlayerOnesTurn;

				return; // We can return early since we know the current player will always win from this position by playing this winning move
			}

			// Create a new child node for this new position and add it to our temporary list of collected children
			collectedChildren.push_back(new MoveNode(newPosition, totalNodes, move, depth + 1, !isPlayerOnesTurn));
		}

		// Now we have created child nodes for all of the legal moves from this position and we can add them to our main list of children
		children = collectedChildren; // Set our main list of children to be the list of collected children we just created

		for (MoveNode* child : children) {

			// Always wins

			// If playing this move does not cause player one to always win
			if (!child->playerOneAlwaysWins) {
				// Then this means player one does not always win from this position
				// Because this move could be played
				playerOneAlwaysWins = false;
			}

			// If playing this move does not cause player two to always win
			if (!child->playerTwoAlwaysWins) {
				// Then this means player two does not always win from this position
				// Because this move could be played
				playerTwoAlwaysWins = false;
			}

			// Can always win

			// If playing this move causes player one to always be able to win
			if (child->playerOneCanAlwaysWin) {
				if (isPlayerOnesTurn) { // And it's player one's turn
					// Then player one can play this move to always win
					playerOneCanAlwaysWinBySomeMove = true;
				}

				// Since there exists a way for player one to always win
				// Player two does not win by force
				playerTwoCanAlwaysWinByForce = false;
			}

			// If playing this move causes player two to always be able to win
			if (child->playerTwoCanAlwaysWin) {
				if (!isPlayerOnesTurn) { // And it's player two's turn
					// Then player two can play this move to always win
					playerTwoCanAlwaysWinBySomeMove = true;
				}

				// Since there exists a way for player two to always win
				// Player one does not win by force
				playerOneCanAlwaysWinByForce = false;
			}
		}

		// We assuming the players are playing perfectly
		// If they have a way to always win then they will
		// And therefore the other player will lose

		// If a player is forced to win from this position
		if (playerOneAlwaysWins || playerTwoAlwaysWins)
		{
			// Then obviously they can win from this position
			// Since they are going to
			playerOneCanAlwaysWin = playerOneAlwaysWins;
			playerTwoCanAlwaysWin = playerTwoAlwaysWins;
		}
		else { // Otherwise if neither player is forced to win

			// Then whether or not they can win depends on if they can win either by playing some winning move
			// or by playing some move that the opponent has no winning response to
			playerOneCanAlwaysWin = playerOneCanAlwaysWinBySomeMove || playerOneCanAlwaysWinByForce;
			playerTwoCanAlwaysWin = playerTwoCanAlwaysWinBySomeMove || playerTwoCanAlwaysWinByForce;
		}

		// DEBUGGING: Detect clashes and indeterminates
		//validateNode(position);
		// END DEBUGGING
	}

	// A debugging utility I made to detect whenever there's something wrong with a node and print some useful debugging info
	void validateNode(Game position, bool issue = false) {

		if (issue)
		{
			std::cout << std::endl << "Custom issue triggered";
		}
		if (playerOneAlwaysWins && playerTwoAlwaysWins)
		{
			std::cout << std::endl << "Both players always win (clash)";
			issue = true;
		}
		if (playerOneCanAlwaysWin == playerTwoCanAlwaysWin) {
			std::cout << std::endl << (playerOneCanAlwaysWin ? "Both players can always win (clash)" : "Failed to determine the player who can always win");
			issue = true;
		}

		if (issue) {
			if (position.empty())
			{
				std::cout << " at root node";
			}
			else {
				std::cout << " at: ";
				for (Move positionMove : position) { // Print out the moves that led to this position to see how we got here
					std::cout << std::to_string(positionMove) << ((positionMove != position.back()) ? "," : ""); // Print the move and an arrow to separate the moves except for the last move in the position
				}
			}

			std::cout << std::endl << std::endl << "The children of this node are:" << std::endl;

			for (MoveNode* child : children) { // For each child node we just created
				std::cout << "The move " << child->move << " where " << (child->playerOneCanAlwaysWin ? (child->playerTwoCanAlwaysWin ? "both players can always win (clash)" : "player one can always win") : (child->playerTwoCanAlwaysWin ? "player two can always win" : "neither player is determined to always be able to win")) << " and where " << (child->playerOneAlwaysWins ? (child->playerTwoAlwaysWins ? "both players always win (clash)" : "player one is forced to win") : (child->playerTwoAlwaysWins ? "player two is forced to win" : "neither player is forced to win")) << std::endl;
			}

			exit(1);
		}
	}

	~MoveNode() { // When this node is deleted
		for (MoveNode* child : children) { // For each child node
			delete child; // Delete it too
		}
	}

	// It's useful to be able to prune down the tree and remove nodes based on certain criteria

	// The most natural way to prune the tree is to make one or both players perfect
	// Removing any move they could make where they can't always win

	// If the specified player can always win from this position
	bool canAlwaysWin(bool isPlayerOne)
	{
		return (isPlayerOne && playerOneCanAlwaysWin) || (!isPlayerOne && playerTwoCanAlwaysWin);
	}

	// Perform perfect play pruning to make perfect player never play moves where they can't always win
	void perfectPlayPrune(bool playerOneIsPerfect, bool playerTwoIsPerfect, int* totalPrunedNodes = nullptr, bool isPlayerOnesTurn = true, std::vector<Move> path = {}) {

		if (!playerOneIsPerfect && !playerTwoIsPerfect) { // If we're not making any player play perfectly
			return; // Then we don't need to do anything
		}

		if (playerOneAlwaysWins || playerTwoAlwaysWins) { // If this node already causes one of the players to be forced to win
			return; // Then there's nothing to prune here since you can't play any differently to avoid this forced win
		}

		for (MoveNode* child : children) { // For each child
			// Recursively assume perfect play for the child node with the opposite player's turn
			std::vector<Move> newPath = path;
			newPath.push_back(child->move);
			child->perfectPlayPrune(playerOneIsPerfect, playerTwoIsPerfect, totalPrunedNodes, !isPlayerOnesTurn, newPath);
		}

		// Only if we're pruning this node
		if ((isPlayerOnesTurn && playerOneIsPerfect) || (!isPlayerOnesTurn && playerTwoIsPerfect)) {

			// If there's a garunteed winning move available for the current player from this position
			if (canAlwaysWin(isPlayerOnesTurn)) {

				// Then we can prune all of the child nodes that don't lead to a garunteed win for the current player since we are assuming they will always play the winning move
				children.erase(std::remove_if(children.begin(), children.end(), [isPlayerOnesTurn, totalPrunedNodes](MoveNode* child) { // Remove any child node that does not lead to a forced win for the current player

					bool imperfectMove = !child->canAlwaysWin(isPlayerOnesTurn); // If the move does not garuntee winning then it is imperfect

					if (imperfectMove) {
						if (totalPrunedNodes != nullptr) // If we are tracking how many nodes we prune
						{
							(*totalPrunedNodes)++; // Increment our pruned node count to keep track of how many nodes we have pruned in our game tree
						}
						delete child; // Free the memory used by this child node since we're pruning it from the tree
					}
					return imperfectMove; // Check if this child node does not lead to a win for the current player
					}), children.end());
			}
		}
	}

	// Another way to prune the tree is to assume that one or both players will always choose the first move available to them in the list of legal moves
	// That way if a player has a winning strategy, it doesn't matter which winning move they play since they will always choose the first one listed
	void alwaysChooseFirstMovePrune(bool playerOneAlwaysChoosesFirstMoveAvailable, bool playerTwoAlwaysChoosesFirstMoveAvailable, int* totalPrunedNodes = nullptr, bool isPlayerOnesTurn = true) {
		if (!playerOneAlwaysChoosesFirstMoveAvailable && !playerTwoAlwaysChoosesFirstMoveAvailable) { // If we're not assuming either player always chooses the first move
			return; // Then we don't need to prune any child nodes since we never made that	assumption in the first place
		}

		if ((isPlayerOnesTurn && playerOneAlwaysChoosesFirstMoveAvailable) || (!isPlayerOnesTurn && playerTwoAlwaysChoosesFirstMoveAvailable)) { // If we need to prune at this node
			// Delete all other child nodes except for the first one since we're assuming the current player always chooses the first move

			for (int i = 1; i < children.size(); i++) { // Start from index 1 to skip the first child node
				if (totalPrunedNodes != nullptr) // If we are tracking how many nodes we prune
				{
					(*totalPrunedNodes)++; // Increment our global pruned node count to keep track of how many nodes we have pruned in our game tree
				}
				delete children[i]; // Free the memory used by this child node since we pruned it from the tree
			}

			// Keep only the first child
			children.resize(children.empty() ? 0 : 1);
		}

		for (MoveNode* child : children) { // For each remaining child node
			child->alwaysChooseFirstMovePrune(playerOneAlwaysChoosesFirstMoveAvailable, playerTwoAlwaysChoosesFirstMoveAvailable, totalPrunedNodes, !isPlayerOnesTurn); // Recursively prune the child node with the opposite player's turn
		}
	}

	// Helper method to join a list of strings together with newlines inbetween them
	std::string joinStringsWithNewlines(std::vector<std::string> strings)
	{
		std::string joinedString = ""; // Prepare the joined string

		bool isFirstLine = true; // Start by assuming this is the first like
		for (std::string string : strings)
		{
			if (isFirstLine) // Once the first line passes
			{
				isFirstLine = false; // It is no longer the first line
			}
			else
			{
				joinedString += "\n"; // If it's not the first line then we add a newline character
			}
			joinedString += string; // And then that line of text
		}

		return joinedString;
	}

	// Generate a list of strings that represent every possible full game within this tree
	std::vector<std::string> generateGameStringList(bool printWinNotice = true, std::vector<Move> path = {}) {
		std::vector<std::string> gameStringList; // Create a list of strings for each game string

		if (isWinningNode) { // Print out the full game once we reach a winning node
			std::string gameString = ""; // A string to build up the game string for this winning leaf node
			for (Move pathMove : path) { // Print out the moves that led to this position to see how we got here
				gameString += std::to_string(pathMove) + ((pathMove != path.back()) ? "," : ""); // Print the move and a comma to separate the moves except for the last move in the position
			}
			gameString += (printWinNotice ? " (" + std::to_string(playerOneAlwaysWins ? 1 : 2) + " wins)" : ""); // Print out that this is a winning move for the current player and move to a new line
			return { gameString }; // Return the game string
		}

		for (MoveNode* child : children) { // For each child node
			std::vector<Move> newPath = path; // Create a new path vector to represent the path to this child node
			newPath.push_back(child->move); // Add the move that leads to this child node to the path

			std::vector<std::string> subGameStringList = child->generateGameStringList(printWinNotice, newPath); // Recursively generate the list of game strings for the child node with the opposite player's turn

			gameStringList.insert(gameStringList.end(), subGameStringList.begin(), subGameStringList.end()); // Add the generated moves to the main list of moves

		}

		return gameStringList; // Return the generated list for all of the children of this node
	}

	// Generate a single string to represent the full list of games within this tree
	std::string generateGameList(bool printWinNotice = true) {
		return joinStringsWithNewlines(generateGameStringList(printWinNotice));
	}

	// Generate a text based diagram of the game tree
	// Note that the turns are reversed because we skip printing the root node
	std::vector<std::string> generateTreeDiagramLines(bool isPlayerOnesTurn = false, int indentation = -1, bool isOnlyResponse = false) {

		std::string moveSymbols = ManipulateMove::toSymbols(E, move, isPlayerOnesTurn); // Represent the move in a readable format

		std::string indent = ""; // The string at the start of each line to create indentation for the diagram to show the structure of the tree
		for (int i = 0; i < indentation; i++) { // For each level of indentation we want to add
			indent += "| "; // We can use a vertical bar and a space to create a visual indentation for each level of the tree
		}

		if (playerOneAlwaysWins || playerTwoAlwaysWins) // If either player is forced to win from this position no matter what
		{
			return { indent + "# " + moveSymbols };
		}

		std::vector<std::string> diagramLines; // Create the list of diagram lines

		if (move != 0)
		{
			// If this is not the root node then add the line for this node
			diagramLines = { indent + (isOnlyResponse ? "- " : "+ ") + moveSymbols };
		}

		for (MoveNode* child : children) { // For each child node

			std::vector<std::string> subDiagramLines = child->generateTreeDiagramLines(!isPlayerOnesTurn, indentation + 1, children.size() == 1); // Recursively generate the lines of the tree diagram for this child

			diagramLines.insert(diagramLines.end(), subDiagramLines.begin(), subDiagramLines.end()); // Add the generated moves to the main list of moves
		}

		return diagramLines; // Return the generated diagram for this node and all of its children
	}

	// Generate a single string to represent the full list of games within this tree
	std::string generateTreeDiagram() {
		return joinStringsWithNewlines(generateTreeDiagramLines());
	}
};
