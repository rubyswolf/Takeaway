// The Takeaway Challenge
// This is a two player game where you start with a universal set E of a certain size we'll call n:
int n = 4; 
// For example with n=4 so we have the universal set E = {1, 2, 3, 4}
// To make a move you chose a subset of E, but you can't pick the empty set or the full set E itself
// So for example you could pick {1, 2} but not {} or {1, 2, 3, 4}
// You may not pick a subset of E that contains ANY of the subsets that have already been picked
// For example player one may pick {1} but then player two may not pick {1, 2} because it contains the subset {1} that player one has already picked
// The player who cannot make a move loses
// For n=2 with E={1,2} player one could play {1}, then player two could play {2}, then player one would have no moves left and would lose while player two would win
// The question is who has the winning strategy for n=3 and n=4 and what is that strategy?
//
// I aimed to solve this via a brute force search
// Then my final winning strategy will be in the form of a lookup table that garuntees one player a win

#include <iostream> // This library lets us output to the console window that will be attached to the program when we run it
#include <fstream> // This lets us write to output to files
#include <vector> // This allows us to create dynamically sized lists of items called vectors
#include <set> // This lets us define sets of items
#include <string> // This allows us to work with strings of text

// The way we'll represent moves though will not be sets but rather bitmasks
// The idea is to have a binary number where each digit is either 1 for the element being in the set or 0 for it not being in the set
// For example for n=4 we could have 0101 to represent the set {1, 3} because the first and third digits are 1s so we select the first and third element
typedef int Move; // Define moves to be bitmask integers

// Now it will be useful to save a copy of the number corresponding to the full set E
// It will look like 1111... (all 1s) because every element that could be chosen is in the set
Move E = std::pow(2, n) - 1; // We do this through 2^n-1
// This might feel strange because it's binary (base 2) but it's easy to understand if I show the equivalent in decimal (base 10):
// Take for example 10^3=1000 (10^n rather than 2^n) then when we subtract 1 for 1000-1 we get 999 which is the number with 3 9s and we used n=3
// So when we do 2^n-1 we get a number with n 1s in binary, which is exactly the full set bitmask we want

// Permutations will come in later
// In C++ you're supposed to define things above where you use them so this needs to be up here as we're about to use it

// An abstraction to store a permutation
class Permutation : public std::vector<int> // Now you're really not meant to extend the vector class but I've done it before and she'll be right
{
public:
	// Generate all permutations of a bag of numbers
	static std::vector<Permutation> allPermutations(std::set<int> bag, Permutation chosen = {}) {
		if (bag.empty()) { // Once the bag is empty we have pulled out a complete permutation and we can return it
			return { chosen }; // Return the chosen items as a complete permutation
		}
		else { //Otherwise if there's still items in the bag
			// Try pulling out each item in the bag and then recursively generate all permutations of the remaining items
			std::vector<Permutation> permutations; // A vector to store the generated permutations
			for (int item : bag) { // For each item in the bag
				std::set<int> newBag = bag; // Create a new bag to represent the remaining items after pulling out the current item
				newBag.erase(item); // Remove the current item from the new bag
				Permutation newChosen = chosen; // Create a new vector to represent the chosen items so far
				newChosen.push_back(item); // Add the current item to the chosen vector
				std::vector<Permutation> subPermutations = allPermutations(newBag, newChosen); // Recursively generate all permutations of the remaining items with the new bag and chosen items
				permutations.insert(permutations.end(), subPermutations.begin(), subPermutations.end()); // Add the generated permutations to the main list of permutations
			}
			return permutations; // Added return statement to ensure function returns a value
		}
	}

	// Return all permutations of the bag except for the identity permutation where they're in order
	static std::vector<Permutation> allNonIdentityPermutations(std::set<int> bag) {
		std::vector<Permutation> all = allPermutations(bag); // First get all permutations
		std::vector<Permutation> nonIdentityPermutations; // Create a vector to store only the non-identity permutations

		for (const Permutation& permutation : all) { // For each permutation
			bool isIdentity = true; // Start by assuming it is the identity permutation

			for (int i = 0; i < n; i++) { // Loop over each element
				if (permutation[i] != i) { // If we find something out of order
					isIdentity = false; // It's not not the identity permutation
					break; // Break early on the first item out of order
				}
			}

			if (!isIdentity) { // If it wasn't an identity permutation
				nonIdentityPermutations.push_back(permutation); // Add it to the collection
			}
		}

		return nonIdentityPermutations; // Return the final list of non-identity permutations
	}
};

// Let's define some simple ways we can manipulate a move
// Classes let us create our own custom data types and give them functionality
class MoveManipulator // Let's create an object that is able to manipulate moves
{
public: // These are the properties and methods (functions) that are accessible from outside the class
	// We'll use the keyword "static" which means we can call these methods (owned functions) without a move manipulator instance
	// We can just call them directly on the class itself like MoveManipulator::compliment(move)
	// Otherwise we'd have to actually create a move manipulator as a new object

	// Flip every bit in the mask to get the compliment
	// That is, every item that was in the set is now out of the set and every item that was out of the set is now in the set
	// This is equivalent to the operation E\move in set theory
	static Move compliment(Move move)
	{
		// Doing 1 - x flips a bit because 1 - 0 = 1 and 1 - 1 = 0
		// Assuming we have a full number of 1s like 1111...,
		// if we subtract some number then for each digit it will look like 1-x
		// There is no carry to worry about as a single digit cannot subtract more than 1 and we're taking from 1
		// The full set bit mask is exactly this 1111... we're after
		return E - move; // Subtracting the move mask from the full E bitmask gives the compliment
	}

	// Returns 1 if the move set contains the element at the given index, and 0 otherwise
	// Note that computers count in binary starting from zero so the first element is actually element 0, second is 1 ...
	static Move hasElement(Move move, int elementIndex)
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

	// Relabel the elements by rearranging them
	static Move relabel(Move move, Permutation permutation)
	{
		int sum = 0; // Initialize the sum to store the permuted game
		for (int i = 0; i < n; i++) { // For each bit position (assuming we're working in size n)
			int bitValue = hasElement(move, i); // Get the value of the bit at position i
			sum += bitValue << permutation[i]; // Shift the bit value to its new position according to the permutation and add it to the sum
		}
		return sum; // Return the permuted game
	}

	// Convert the move from a bitmask to a text format like "3 = {1, 2}"
	static std::string toString(Move move) {
		std::string result = std::to_string(move) + " = {"; // Start the string with the move number and an opening brace for the set representation
		for (int i = 0; i < n; i++) { // For each element in the universal set E
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
	static std::string toPrincipalSymbols(Move move, bool isPlayerOnesTurn) {

		std::string result = ""; // Start with an empty string to build the result
		for (int i = 0; i < n; i++) { // For each element in the universal set E
			if (hasElement(move, i)) { // If this element is in the move
				result = result + (isPlayerOnesTurn ? u8"●" : u8"■"); // Add a filled symbol
			}
			else { // Otherwise if this element is not in the move
				result = result + (isPlayerOnesTurn ? u8"○" : u8"□"); // Add an unfilled symbol
			}
		}
		return result; // Return the final string representation of the move
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
	Game() {
		// Initialize the symmetry of the game
		// At the start of the game all elements are interchangable so we have one interchangable set containing all elements
		Interchangable initialInterchangable; // Create a new interchangable to represent the initial interchangables
		for (int i = 0; i < n; i++) { // For each element in the universal set E
			initialInterchangable.insert(i); // Add it to the initial interchangable set
		}
		gameSymmetry.push_back(initialInterchangable); // Add this initial interchangable set to the game's symmetry
	}

	Symmetry gameSymmetry; // The current symmetry of the game

	void playMove(Move move) {

		// DEBUGGING: Print out the move being made
		//std::cout << "Playing move: " << MoveManipulator::toString(move) << std::endl;
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
				if (MoveManipulator::hasElement(move, element)) { // If this element was selected in the new move
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

	// Relabel the elements of this game via some permutation
	void relabel(Permutation permutation) {
		for (Move& move : *this) { // Get a REFERENCE to each move (not just a copy)
			move = MoveManipulator::relabel(move, permutation); // Relabel that move using the reference
		}
	}

	// Get all possible moves (legal or not) without taking into account the current position or the rules of the game
	// This is static so we can call it without referencing a specific game
	static vector<Move> allMoves(int n) {
		vector<Move> allMoves; // A list (vector) to store all possible moves that we will generate

		for ( // Start a new loop for all possible moves
			int candidateMove = 1; // The loop starts at 1 (...0001) because we can't pick the empty set which is 0 (...0000)
			// Loop ends at 2^n-2 (...1110)
			candidateMove < std::pow(2, n) - 1; // This condition will fail once we hit E (2^n-1) because then it will be equal and not less than
			// This means the loop terminates without running for E since we can't choose the full set, but it will run for all other possible moves from 1 to E-1
			candidateMove++) { // At each step we simply add 1 to the candidate move which will iterate through all possible move bitmasks in ascending order
			allMoves.push_back(candidateMove);
		}
		return allMoves;
	}

	// Get only the principal moves from this position
	vector<Move> principalMoves(int interchangableIndex = 0, vector<int> elements = {}) {
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

			if (move == 0 || move == E) { // We can't pick the empty set or the full set E itself
				return {}; // Return an empty vector to indicate that this is not a valid move
			}
			return { move }; // Return this move as a single element vector
		}
		else { // Otherwise we need to select how many elements we want from this interchangable and then recursively generate the moves for the next interchangable
			vector<Move> moves; // A vector to store the generated principal moves
			int interchangableSize = gameSymmetry[gameSymmetry.size()-interchangableIndex-1].size(); // The number of elements in this interchangable
			// Note the index here is backwards to make the choice for the last interchangable first and work backwards

			// For each possible number of selected elements from this interchangable (from 0 to all)
			for (int selectedElements = 0; selectedElements <= interchangableSize; selectedElements++) {
				vector<int> newElements = elements; // Create a new vector to represent the number of selected elements so far
				newElements.push_back(selectedElements); // Add the number of selected elements from this interchangable to the vector
				vector<Move> subMoves = principalMoves(interchangableIndex + 1, newElements); // Recursively generate the moves for the next interchangable with the updated selected elements vector
				moves.insert(moves.end(), subMoves.begin(), subMoves.end()); // Add the generated moves to the main list of moves
			}
			return moves; // Return the generated moves
		}
	}

	// Now we have a list of all possible moves but many of them will be illegal according to the rules of the game
	// So we need a method to filter out the illegal moves and only keep the legal ones
	vector<Move> removeIllegalMoves(vector<Move> candidateMoves) {
		vector<Move> legalMoves; // A list (vector) to store only the legal moves that make it through the filtering

		// We'll try every candidate move and only keep it if it's legal
		for (Move candidateMove : candidateMoves) {

			// We will check if this candidate move is illegal according to the rules of the game
			// If it is illegal then we will discard it and move on to the next candidate move
			// Otherwise it must be legal so we will add it to our list of legal moves

			bool isLegal = true; // We will assume the move is legal until we find a violation that makes it illegal

			// So what makes a move illegal?
			// A move is illegal if ANY of the previously chosen sets as subsets of the candidate move set
			// So we just loop over all of the previous moves to see if this is the case for any of them:
			for (Move previouslyChosenSet : *this) { // Here "*this" means "this game"

				if ( // We need to check if this previously chosen set makes our candidate move illegal
					// IE: if it is a subset of the candidate move set
					// For a set to be contained in another set, then for ALL elements in the universal set E, either:
					// It's in the previously chosen set and therefore it must also in the candidate move set
					// Or
					// It's not in the previously chosen set at all (so it doesn't violate the rules of being a subset)
					// 
					// So let's check exactly that for each element (bit):
					
					((previouslyChosenSet // If that element is in the previously chosen set
					& candidateMove) // Then it must also be in the candidate move set
					| // Or otherwise
					// The element must not belong to the previously chosen set at all
					(MoveManipulator::compliment(previouslyChosenSet)))

					// Now this will give us a new bit mask where:
					// The bit will be 1 if the condition is satisfied
					// Or the bit will be 0 if the condition is not satisfied

					// We need to check that this is satisfied for ALL elements in the universal set E
					// So we want this bitmask to look like 1111... (all 1s)
					// That that is exactly the full set E bitmask we defined earlier, so we can just check if it's equal to E:
					== E
				) {
					// Now any code in here will run whenever the candidate move is illegal
					isLegal = false; // We our legal flag to false to indicate that this candidate move is illegal

					// We can also use the "break" command for efficiency
					// This tells the program to give up immediately and stop checking any more previous moves
					// Since at this point we already know the move is illegal
					// So continueing to check more previous moves would be a waste of time
					break;
				}
			}

			// Alright, we now know whether this candidate move is legal or not
			if (isLegal) { // If the move is legal then we add it to our list of legal moves
				legalMoves.push_back(candidateMove);
			}
		}

		// Now finally once we have the full list of legal moves, we return it:
		return legalMoves;
	}

	// A method to get the legal moves from the current position
	vector<Move> legalMoves() {
		return removeIllegalMoves(allMoves(n)); // We can just get all possible moves and then filter out the illegal ones to get the legal moves
	}

	// A method to get the principal legal moves from the current position
	vector<Move> principalLegalMoves() {
		return removeIllegalMoves(principalMoves()); // We can just get the principal moves and then filter out the illegal ones to get the principal legal moves
	}
};

int totalNodes = 0; // A global variable to keep track of how many nodes we have generated in our game tree
int totalPrunedNodes = 0; // A global variable to keep track of how many nodes we have pruned in our game tree

class MoveNode
{
public:
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

	MoveNode(Game position, Move move = 0, int depth = 0, bool isPlayerOnesTurn = true, bool isWinningNode = false) : move(move), isWinningNode(isWinningNode) {

		if (totalNodes%10000 == 0) { // Every 10,000 nodes we generate, print out how many nodes we have generated so far to keep track of our progress
			std::cout << "Generated " << totalNodes << " nodes, currently at: ";
			for (Move positionMove : position) { // Print out the moves that led to this position to see how we got here
				std::cout << std::to_string(positionMove) << ((positionMove != position.back()) ? "," : ""); // Print the move and an arrow to separate the moves except for the last move in the position
			}
			std::cout << std::endl;
		}

		totalNodes++; // Increment our global node count to keep track of how many nodes we have generated in our game tree

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
		// We set this false if there exists a way for the player to not win
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
				children.push_back(new MoveNode(newPosition, move, depth+1, !isPlayerOnesTurn, true));

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
			collectedChildren.push_back(new MoveNode(newPosition, move, depth + 1, !isPlayerOnesTurn));
		}

		// Now we have created child nodes for all of the legal moves from this position and we can add them to our main list of children
		children = collectedChildren; // Set our main list of children to be the list of collected children we just created

		for (MoveNode* child : children) {

			// Always wins

			// If this node could still go either way (no player is forced a win due to this move)
			if (!child->playerOneAlwaysWins && !child->playerTwoAlwaysWins) {
				// Then we can't say that either player is forced a win from this position
				// since if this move were played then no win would be forced
				// So we can't possibly have a forced win from here either

				// So we set both players to not always win from this position
				playerOneAlwaysWins = false;
				playerTwoAlwaysWins = false;
			}

			// If this there is a way player one can always win from this position
			if (child->playerOneCanAlwaysWin) {
				// Then there is a continuation where player two loses
				// So player two does not always win
				playerTwoAlwaysWins = false;
			}

			// If this there is a way player two can always win from this position
			if (child->playerTwoCanAlwaysWin) {
				// Then there is a continuation where player one loses
				// So player one does not always win
				playerOneAlwaysWins = false;
			}

			// Can always win

			// If playing this move causes player one to always be able to win
			if (child->playerOneCanAlwaysWin) {
				if (isPlayerOnesTurn) { // And it's player one's turn
					// Then player one can play this move to always win
					playerOneCanAlwaysWinBySomeMove = true;
				}

				// Since there exists a move that causes player one to win
				// Player two does not win by force
				playerTwoCanAlwaysWinByForce = false;
			}

			// If playing this move causes player two to always be able to win
			if (child->playerTwoCanAlwaysWin) {
				if (!isPlayerOnesTurn) { // And it's player two's turn
					// Then player two can play this move to always win
					playerTwoCanAlwaysWinBySomeMove = true;
				}

				// Since there exists a move that causes player two to win
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

	// A debugging utility I made to detect whenever there's something wrong with a node
	void validateNode(Game position) {
		bool issue = false;

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

	// It's useful to be able to prune down the tree and remove nodes based on certain criteria

	// The most natural way to prune the tree is to make one or both players perfect
	// Removing any move they could make where they can't always win

	// If the specified player can always win from this position
	bool canAlwaysWin(bool isPlayerOne)
	{
		return (isPlayerOne && playerOneCanAlwaysWin) || (!isPlayerOne && playerTwoCanAlwaysWin);
	}

	// Perform perfect play pruning to make perfect player never play moves where they can't always win
	void perfectPlayPrune(bool playerOneIsPerfect, bool playerTwoIsPerfect, bool isPlayerOnesTurn = true, std::vector<Move> path = {}) {

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
			child->perfectPlayPrune(playerOneIsPerfect, playerTwoIsPerfect, !isPlayerOnesTurn, newPath);
		}

		// Only if we're pruning this node
		if ((isPlayerOnesTurn && playerOneIsPerfect) || (!isPlayerOnesTurn && playerTwoIsPerfect)) {

			// If there's a garunteed winning move available for the current player from this position
			if (canAlwaysWin(isPlayerOnesTurn)) {

				// Then we can prune all of the child nodes that don't lead to a garunteed win for the current player since we are assuming they will always play the winning move
				children.erase(std::remove_if(children.begin(), children.end(), [isPlayerOnesTurn](MoveNode* child) { // Remove any child node that does not lead to a forced win for the current player

					bool imperfectMove = !child->canAlwaysWin(isPlayerOnesTurn); // If the move does not garuntee winning then it is imperfect

					if (imperfectMove) {
						totalPrunedNodes++; // Increment our global pruned node count to keep track of how many nodes we have pruned in our game tree
						delete child; // Free the memory used by this child node since we're pruning it from the tree
						if (totalPrunedNodes % 10000 == 0) { // Every 10,000 nodes we prune, print out how many nodes we have pruned so far to keep track of our progress
							std::cout << "Pruned " << totalPrunedNodes << " nodes" << std::endl;
						}
					}
					return imperfectMove; // Check if this child node does not lead to a win for the current player
				}), children.end());
			}
		}
	}

	// We also want to be able to prune the tree so that the player that can always win will do so as fast as possible
	// The natural choice here is to use the minimax algorithm
	// The idea behind minimax is we want to minimize some score for the worst case scenario
	// The score here will be the length of a game
	// So the worst case scenario for a player is the longest game that can be played from this position
	// And we want to minimize that worst case scenario by pruning any moves that lead to games longer than strictly necessary for the player to win

	// Find the length of the longest game FROM THIS POSITION
	// This does not count previous nodes, only the most moves needed to end the game from here
	int lengthOfLongestGame() {
		int longest = 0; // A variable to keep track of the longest game we find so far
		for (MoveNode* child : children) { // For each child node
			int childLongest = child->lengthOfLongestGame(); // Get the length of the longest game from this child node
			if (childLongest > longest) { // If this is the longest game we've found so far
				longest = childLongest; // Update our longest game variable to be this new longest game
			}
		}
		return longest + 1; // Add one to include the current node itself
	}

	void minimaxPrune(bool forPlayerOne, bool forPlayerTwo, bool isPlayerOnesTurn = true) {

		if (!forPlayerOne && !forPlayerTwo) { // If we're not pruning for either player
			return; // There is nothing to prune
		}

		for (MoveNode* child : children) { // For each child node
			child->minimaxPrune(forPlayerOne, forPlayerTwo, !isPlayerOnesTurn); // Recursively prune the child node with the opposite player's turn
		}

		if ((isPlayerOnesTurn && forPlayerOne) || (!isPlayerOnesTurn && forPlayerTwo)) // If we're pruning this node
		{

			int shortestLongestGame = INT_MAX; // A variable to keep track of the shortest longest game we find so far, initialized to infinity for minimization
			std::vector<int> longestGames; // A vector to store the length of the longest game from each child node so that we can check if there are multiple equally good moves

			for (int i = 0; i < children.size(); i++) { // For each child node
				// Check the length of the longest game from this child node
				longestGames.push_back(children[i]->lengthOfLongestGame()); // Add the length of the longest game from this child node to our list of longest games

				//std::cout << "Move " << std::to_string(children[i]->move) << " has longest game of length " << std::to_string(longestGames[i]) << std::endl;

				if (longestGames[i] < shortestLongestGame) { // If this is the shortest longest game we've found so far
					shortestLongestGame = longestGames[i]; // Update our shortest longest game variable to be this new shortest longest game
				}
			}

			std::vector<MoveNode*> bestChildren; // Initalize a list for only the optimal children

			// Remove any child node that does not lead to the shortest longest game
			for (int i = 0; i < children.size(); i++) { // Start from index 1 to skip the first child node
				if (longestGames[i] == shortestLongestGame) { // Check if this child node is optimal and leads to the shortest possible longest game

					bestChildren.push_back(children[i]); // Add this child to the list of optimal children
				} else { // Otherwise we are getting rid of it
					totalPrunedNodes++; // Increment our global pruned node count to keep track of how many nodes we have pruned in our game tree

					std::cout << "Pruned the move " << std::to_string(children[i]->move) << " because it has longest game of length " << std::to_string(longestGames[i]) << " which is longer than the optimal length " << std::to_string(shortestLongestGame) << std::endl;

					if (totalPrunedNodes % 100 == 0) { // Every 100 nodes we prune, print out how many nodes we have pruned so far to keep track of our progress
						std::cout << "Pruned " << totalPrunedNodes << " nodes" << std::endl;
					}

					delete children[i]; // Free the memory used by this child node since we're pruning it from the tree
				}
			}

			children = bestChildren; // Only keep the best children
		}
	}

	// Another way to prune the tree is to assume that one or both players will always choose the first move available to them in the list of legal moves
	// That way if a player has a winning strategy, it doesn't matter which winning move they play since they will always choose the first one listed
	void alwaysChooseFirstMovePrune(bool playerOneAlwaysChoosesFirst, bool playerTwoAlwaysChoosesFirst, bool isPlayerOnesTurn = true) {
		if (!playerOneAlwaysChoosesFirst && !playerTwoAlwaysChoosesFirst) { // If we're not assuming either player always chooses the first move
			return; // Then we don't need to prune any child nodes since we never made that	assumption in the first place
		}

		if ((isPlayerOnesTurn && playerOneAlwaysChoosesFirst) || (!isPlayerOnesTurn && playerTwoAlwaysChoosesFirst)) { // If we need to prune at this node
			// Delete all other child nodes except for the first one since we're assuming the current player always chooses the first move
			
			// Keep only the first child
			children.resize(children.empty() ? 0 : 1);

			for (int i = 1; i < children.size(); i++) { // Start from index 1 to skip the first child node
				totalPrunedNodes++; // Increment our global pruned node count to keep track of how many nodes we have pruned in our game tree
				delete children[i]; // Free the memory used by this child node since we pruned it from the tree
			}
		}

		for (MoveNode* child : children) { // For each remaining child node
			child->alwaysChooseFirstMovePrune(playerOneAlwaysChoosesFirst, playerTwoAlwaysChoosesFirst, !isPlayerOnesTurn); // Recursively prune the child node with the opposite player's turn
		}
	}

	// While we only make principal moves at each node
	// This unfortunately doesn't stop us from having duplicate games
	// For example at n=4 we have cases like:
	// 3, 5, 2, 8, 1, 4
	// 3, 5, 2, 8, 4, 1
	// 
	// Which means for E={1,2,3,4}
	// The first game is {1, 2}, {1, 3}, {2}, {4}, {1}, {3}
	// The second game is {1, 2}, {1, 3}, {2}, {4}, {3}, {1}
	//
	// {1} is the element picked that was picked in both the first move and second move
	// {3} is the element that was not picked in the first move but was picked in the second move
	// So both moves ARE principal (we can specify them without naming the elements)
	// 
	// But these games are symmetrically equivalent through swapping the labels of 1 and 3
	// So we will still need a final pruning algorithm to deduplicate these games
	//
	// The way I choose to do this is to create all possible relabellings of each game
	// And then for each relabeling we check to see if there exists a node corresponding to it
	// If there is then we simply delete it

	// Searches for a node that can be reached by playing a specific list of moves (a game) from the current node
	MoveNode* getNodeByGame(Game game) {

		if (game.empty()) // If there are no moves we need to make to get to the desired node
		{
			return this; // THIS IS the desired node, return it
		}

		for (MoveNode* child : children) { // For each child
			if (child->move == game.front()) // If the first move in the game corresponds to this child
			{
				game.erase(game.begin()); // We've completed this move, so remove it
				// Not continue the search from this node with the first move of the game removed
				return child->getNodeByGame(game);

				// Break since we assume this is the only child node with this move
				// And we've already found it
				break; 
			}
		}

		// If we reached here then that means the game we are looking for doesn't exist
		return nullptr; // Return a pointer that leads nowhere since it doesn't exist
	}

	void symmetryDeduplicationPrune() {
		Interchangable elementBag; // Initalize an empty bag to put elements into

		for (int i = 0; i < n; i++) // For each element
		{
			elementBag.insert(i); // Add it to the bag
		}

		std::vector<Permutation> allPermutations = Permutation::allNonIdentityPermutations(elementBag); // Create all permutations of the elements

		symmetryDeduplicate(allPermutations, this); // Run the pruning, feeding this node as the root node
	}

	void symmetryDeduplicate(std::vector<Permutation> allPermutations, MoveNode* root, Game position = {}) {
		if (isWinningNode) // If we reach a winning leaf node
		{

			for (Permutation permutation : allPermutations) { // For each permutation of the elements
				Game relabeledGame = position; // Create a copy of the game position
				relabeledGame.relabel(permutation); // Relabel the game using this permutation

				// Try to find if this relabeledGame exists
				MoveNode* relabeledNode = root->getNodeByGame(relabeledGame);

				if (relabeledNode != nullptr) { // If it does exist

					// Then we need to remove this child from the parent's list of children
					MoveNode* relabeledNodeParent = relabeledNode->parent; // Find the node's parent

					if (relabeledNodeParent != nullptr) { // If this parent exists (it should)

						std::vector<MoveNode*>& relabeledNodeSiblings = relabeledNodeParent->children; // Get a reference to the relabeled node's siblings

						relabeledNodeSiblings.erase(std::remove_if(relabeledNodeSiblings.begin(), relabeledNodeSiblings.end(), [relabeledNode](MoveNode* relabeledNodeSibling) { // Remove any child node that does not lead to a forced win for the current player

							bool isTheRelabeledNode = relabeledNodeSibling->move == relabeledNode->move;

							if (isTheRelabeledNode) {
								totalPrunedNodes++; // Increment our global pruned node count to keep track of how many nodes we have pruned in our game tree
								if (totalPrunedNodes % 10 == 0) { // Every 10,000 nodes we prune, print out how many nodes we have pruned so far to keep track of our progress
									std::cout << "Pruned " << totalPrunedNodes << " nodes" << std::endl;
								}
							}
							return isTheRelabeledNode; // Check if this child node does not lead to a win for the current player
						}), relabeledNodeSiblings.end());
					}

					delete relabeledNode; // Free the memory used by this relabeled node since we're pruning it from the tree
				}
			}

		}
		else { // Otherwise if this isn't a winning node

			// We need to recurse to all children
			for (MoveNode* child : children) { // Loop over all children
				Game newPosition = position; // Copy the game position
				newPosition.playMove(child->move); // Play the child's move
				child->symmetryDeduplicate(allPermutations, root, newPosition);
			}
		}
	}

	// Generate a text based diagram of the game tree
	std::string generateTreeDiagram(bool isPlayerOnesTurn = true, int indentation = -1) {
		std::string diagram = ""; // A string to build up the diagram of the game tree

		if (move != 0) { // If this is not the root node then we want to print this node

			std::string indent = ""; // The string at the start of each line to create indentation for the diagram to show the structure of the tree
			for (int i = 0; i < indentation; i++) { // For each level of indentation we want to add
				indent += "| "; // We can use a vertical bar and a space to create a visual indentation for each level of the tree
			}

			// A prefix to indicate whether this is a winning node or not and show if there are multiple moves available from this position or not
			std::string prefix = isWinningNode ? "# " : (children.size() > 1 ? "+ " : "- ");

			std::string principalSymbols = MoveManipulator::toPrincipalSymbols(move, isPlayerOnesTurn); // Represent the move in a readable format

			diagram += indent + prefix + principalSymbols + "\n"; // Add this move to our diagram with the appropriate indentation and prefix
		}

		for (MoveNode* child : children) { // For each child node
			// Check if this node is an early child
			bool onlyChild = true;

			// Check if this node has a parent
			if (parent != nullptr) {
				// Check if this node is an only child of its parent
				onlyChild = parent->children.size() == 1;
			}

			// Don't indent if this node is an only child and this child of it is an only child
			int newIndentation = onlyChild && children.size() == 1 ? indentation : indentation + 1;
			
			// This occurs for situations like:
			// 
			// + 1
			// | - 2
			// | | - 3
			// | | | # 4
			// + 5
			// 
			// Which will now look like:
			// + 1
			// | - 2
			// | - 3
			// | # 4
			// + 5
			//
			// Greatly improving the readability of the diagram by reducing unnecessary indentation for long chains of only children

			diagram += child->generateTreeDiagram(!isPlayerOnesTurn, newIndentation); // Recursively generate the diagram for the child node with the opposite player's turn and increased indentation
		}

		return diagram; // Return the generated diagram for this node and all of its children
	}

	std::vector<std::string> generateGameStringList(bool printWinNotice = true, std::vector<Move> path = {}) {
		std::vector<std::string> gameStringList; // Create a list of strings for each game string

		if (isWinningNode) { // Print out the full game once we reach a winning node
			std::string gameString = ""; // A string to build up the game string for this winning leaf node
			for (Move pathMove : path) { // Print out the moves that led to this position to see how we got here
				gameString += std::to_string(pathMove) + ((pathMove != path.back()) ? "," : ""); // Print the move and a comma to separate the moves except for the last move in the position
			}
			gameString += (printWinNotice ? " (" + std::to_string(playerOneAlwaysWins ? 1 : 2) + " wins)" : ""); // Print out that this is a winning move for the current player and move to a new line
			return { gameString } ; // Return the game string
		}

		for (MoveNode* child : children) { // For each child node
			std::vector<Move> newPath = path; // Create a new path vector to represent the path to this child node
			newPath.push_back(child->move); // Add the move that leads to this child node to the path

			std::vector<std::string> subGameStringList = child->generateGameStringList(printWinNotice, newPath); // Recursively generate the list of game strings for the child node with the opposite player's turn

			gameStringList.insert(gameStringList.end(), subGameStringList.begin(), subGameStringList.end()); // Add the generated moves to the main list of moves
			
		}

		return gameStringList; // Return the generated list for all of the children of this node
	}

	std::string generateGameList(bool printWinNotice = true) {
		std::string gameList = "";
		std::vector<std::string> gameStringList = generateGameStringList(printWinNotice);

		bool isFirstLine = true;
		for (std::string gameString : gameStringList)
		{
			if (isFirstLine)
			{
				isFirstLine = false;
			}
			else
			{
				gameList += "\n";
			}
			gameList += gameString;
		}

		return gameList;
	}
};

int main()
{
	//--------------------------------------------------------
	// Example usage of the game and move manipulator classes
	//--------------------------------------------------------

	/*
	// Print the legal moves from the position 1 (0001), 2 (0010) for n=4
	Game game; // Create a new game with the moves 1 and 2 already chosen
	std::cout << "Starting game from: " << MoveManipulator::toString(E) << std::endl; // Print the starting position of the game which is the full set E
	game.playMove(1);
	game.playMove(6);
	// Print out the symmetry of this position to see how the elements are categorized
	std::cout << "Symmetry of position: ["; // Print a header for the symmetry
	for (Interchangable interchangable : game.gameSymmetry) { // For each interchangable in the game's symmetry
		std::cout << "{"; // Print the opening brace for the interchangable set
		for (Move element : interchangable) { // For each element in this interchangable
			// Print the element (add 1 to convert from 0-indexed to 1-indexed) and a comma and space to separate the elements except for the last element in the set
			std::cout << element + 1 << ((element != *interchangable.rbegin()) ? ", " : "");
		}
		std::cout << "}" << ((interchangable != game.gameSymmetry.back()) ? ", " : ""); // Print the closing brace for the interchangable set and a comma and space to separate the interchangables except for the last interchangable in the symmetry
	}
	std::cout << "]" << std::endl; // Print a closing bracket for the symmetry and move to a new line
	std::vector<Move> legalMoves = game.principalLegalMoves(); // Get the principal legal moves from this position
	std::cout << "Principal legal moves from this position:" << std::endl; // Print a header for the principal legal moves
	for (Move move : legalMoves) { // For each principal legal move
		std::cout << MoveManipulator::toString(move) << std::endl; // Print the move in a readable format
	}
	*/


	//------------------------------------------------------------------------------------------------------
	// Generate a game tree, assuming each player will play a winning move whenever they have one available
	//------------------------------------------------------------------------------------------------------

	std::cout << "Generating game tree..." << std::endl;
	MoveNode gameTree = MoveNode(Game()); // Create a move node for the initial game position to generate the game tree
	std::cout << "Game tree generated, generated a total of " << totalNodes << " nodes" << std::endl;

	// Save the full list of games from the initial position
	//std::ofstream(std::to_string(n) + "_all.txt") << gameTree.generateGameList(); // Generate a list of all games from the initial position and save it to a text file

	std::cout << "Player " << (gameTree.playerOneCanAlwaysWin ? "one" : "two") << " can always win!" << std::endl << std::endl;


	//----------------------------------------------------------------------------
	// Example of generating all possible games where both players play perfectly
	//----------------------------------------------------------------------------

	/*
	std::cout << "Pruning for perfect play for both players..." << std::endl;
	gameTree.perfectPlayPrune(true, true); // Assume perfect play for both players to prune the game tree and find the winning strategy for both players
	std::cout << "Pruning complete, pruned a total of " << totalPrunedNodes << " nodes" << std::endl;

	// Save the full list of games where both players play perfectly from the initial position
	std::ofstream(std::to_string(n) + "_perfect.txt") << gameTree.generateGameList();
	*/

	//-----------------------------------------------------------------------------------
	// Otherwise we can generate the winning strategy for the player than can always win
	// Without assuming the other player plays perfectly
	//-----------------------------------------------------------------------------------

	std::cout << "Pruning for perfect play for player " << (gameTree.playerOneCanAlwaysWin ? "one" : "two") << " only..." << std::endl;
	//gameTree.perfectPlayPrune(playerOneAlwaysWins, !playerOneAlwaysWins); // Prune only for the player that always wins, not assuming perfect play for the other player
	gameTree.perfectPlayPrune(gameTree.playerOneCanAlwaysWin, !gameTree.playerOneCanAlwaysWin);
	std::cout << "Perfect play pruning complete, pruned a total of " << totalPrunedNodes << " nodes" << std::endl;

	//std::ofstream(std::to_string(n) + "_winning_strategy.txt") << gameTree.generateGameList(false);

	totalPrunedNodes = 0; // Reset the pruned nodes counter

	std::cout << "Pruning with minimax pruning so player " << (gameTree.playerOneCanAlwaysWin ? "one" : "two") << " always wins as fast as possible..." << std::endl;
	gameTree.minimaxPrune(gameTree.playerOneCanAlwaysWin, !gameTree.playerOneCanAlwaysWin); // Now we can also prune for the winning player to win as fast as possible since we know they always win from the initial position
	std::cout << "Minimax pruning complete, pruned a total of " << totalPrunedNodes << " nodes" << std::endl;
	

	totalPrunedNodes = 0; // Reset the pruned nodes counter

	std::cout << "Prining by making player " << (gameTree.playerOneCanAlwaysWin ? "one" : "two") << " always choose the first optimal move available..." << std::endl;
	gameTree.alwaysChooseFirstMovePrune(gameTree.playerOneCanAlwaysWin, !gameTree.playerOneCanAlwaysWin); // Now we can also prune for the winning player to always choose the first optimal move available since we know they always win from the initial position
	std::cout << "First move pruning complete, pruned a total of " << totalPrunedNodes << " nodes" << std::endl;

	totalPrunedNodes = 0; // Reset the pruned nodes counter

	std::cout << "Pruning by deduplicating symmetrically equivalent games..." << std::endl;
	gameTree.symmetryDeduplicationPrune(); // Now we can also prune for the winning player to always choose the first optimal move available since we know they always win from the initial position
	std::cout << "Symmetry deduplication pruning complete, pruned a total of " << totalPrunedNodes << " nodes" << std::endl;

	std::ofstream(std::to_string(n) + "_winning_strategy_pruned.txt") << gameTree.generateGameList(false);

	/*
	std::cout << "Generating tree diagram of the final pruned game tree..." << std::endl;
	std::string diagram = gameTree.generateTreeDiagram(); // Generate a text based diagram of the final pruned game tree to visualize the winning strategy for the player that always wins
	std::cout << "Tree diagram generated, saving to a text file..." << std::endl;
	std::string outputFileName = std::to_string(n)+"_tree.txt"; // The name of the text file to save the diagram of the game tree to
	std::ofstream outFile(outputFileName); // Create an output file stream to write to the text file
	outFile << diagram; // Write the diagram of the game tree to the text file
	outFile.close(); // Close the output file stream to free up system resources
	std::cout << "Diagram saved to " << outputFileName << std::endl;
	*/
}