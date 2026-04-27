#include "takeaway.h" // Include the game logic and solver core

int main()
{
	UniversalSet E = UniversalSet(4); // Create the universal set
	bool full = false; // Full output expands equivalent positions instead of hiding duplicate subtrees

	// Generate the game tree
	std::cout << "Generating game tree..." << std::endl;
	unsigned long long* totalNodes = new unsigned long long(0); // Prepare a counter for how many nodes we generate
	MoveNode gameTree = MoveNode(Game(E), totalNodes); // Create a move node for the initial game position to generate the game tree
	std::cout << "Game tree generated, generated a total of " << *totalNodes << " nodes" << std::endl;

	// Declare which player can always win if they play perfectly
	std::cout << "Player " << (gameTree.playerOneCanAlwaysWin ? "one" : "two") << " can always win!" << std::endl << std::endl;

	//# Optimise the game tree for the player that can always win

	// Remove all nodes where the player that can always win makes a move that does not lead them to always win
	// This makes that player play perfectly
	std::cout << "Pruning for perfect play for player " << (gameTree.playerOneCanAlwaysWin ? "one" : "two") << " only..." << std::endl;
	unsigned long long* totalPrunedNodes = new unsigned long long(0); // Prepare a counter for how many nodes we prune
	gameTree.perfectPlayPrune(gameTree.playerOneCanAlwaysWin, !gameTree.playerOneCanAlwaysWin, totalPrunedNodes);
	std::cout << "Perfect play pruning complete, pruned a total of " << *totalPrunedNodes << " nodes" << std::endl;

	// Since the player that can always win often has multiple equally perfect moves
	// To reduce the game tree futher, we can make them simply always play the first perfect move available
	std::cout << "Pruning by making player " << (gameTree.playerOneCanAlwaysWin ? "one" : "two") << " always choose the first optimal move available..." << std::endl;
	*totalPrunedNodes = 0; // Reset the counter
	gameTree.alwaysChooseFirstMovePrune(gameTree.playerOneCanAlwaysWin, !gameTree.playerOneCanAlwaysWin, totalPrunedNodes); // Now we can also prune for the winning player to always choose the first optimal move available since we know they always win from the initial position
	std::cout << "First move pruning complete, pruned a total of " << *totalPrunedNodes << " nodes" << std::endl;

	//# Save the results

	// Save the strategy as set-response rules like "{1,14,2} -> 12".
	std::cout << "Generating set-response rules..." << std::endl;
	std::string setResponseFileName = "n" + std::to_string(E.size) + " set response.txt"; // The name of the text file to save the set-response strategy rules
	std::ofstream(setResponseFileName) << gameTree.generateSetResponse(full); // Save rules where every move before the arrow is part of the matched position and the move after the arrow is the response
	std::cout << "Set-response rules saved to " << setResponseFileName << std::endl;
	
	// Save a text based tree diagram of the game tree
	std::cout << "Generating tree diagram of the final pruned game tree..." << std::endl;
	std::string treeFileName = "n" + std::to_string(E.size) + " tree.txt"; // The name of the text file to save the diagram of the game tree to
	std::ofstream(treeFileName) << gameTree.generateTreeDiagram(full); // Generate and save a text based diagram of the final pruned game tree to visualize the winning strategy for the player that always wins
	std::cout << "Tree diagram saved to " << treeFileName << std::endl;
}
