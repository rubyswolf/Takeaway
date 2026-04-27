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

	// Save a list of all games in the tree as a comma separated list of move bitmask numbers like "1,14,2,12,4,8", one per line
	std::cout << "Generating list of winning games..." << std::endl;
	std::string gameListFileName = "n" + std::to_string(E.size) + " winning games.txt"; // The name of the text file to save the list of games where the player that can always win does win
	std::ofstream(gameListFileName) << gameTree.generateGameList(full, false); // Generate a and save a list of all the games where the player that can always win plays perfectly to always win
	std::cout << "Game list saved to " << gameListFileName << std::endl;
	
	// Save a text based tree diagram of the game tree
	std::cout << "Generating tree diagram of the final pruned game tree..." << std::endl;
	std::string treeFileName = "n" + std::to_string(E.size) + " tree.txt"; // The name of the text file to save the diagram of the game tree to
	std::ofstream(treeFileName) << gameTree.generateTreeDiagram(full); // Generate and save a text based diagram of the final pruned game tree to visualize the winning strategy for the player that always wins
	std::cout << "Tree diagram saved to " << treeFileName << std::endl;
}

