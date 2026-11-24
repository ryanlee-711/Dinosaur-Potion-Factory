#pragma once

#include <string>
#include <utility>
#include <vector>

// Everything characters say, read from dist/data/dialogue.tsv
//
//   npc <TAB> key <TAB> text
//
// Rows with the same npc + key are pages, shown one box at a time, in file order.
// Text wraps by itself; "\n" in the text forces a line break.
namespace Dialogue {
	// The pages for npc + key (empty if there are none).
	std::vector< std::string > const &lines(std::string const &npc, std::string const &key);

	// One page picked at random from npc + key ("" if there are none).
	std::string const &random_line(std::string const &npc, std::string const &key);

	// The speaker name for the dialogue box: the "name" row, or the npc id if there is none.
	std::string name(std::string const &npc);

	// The "ask:", "before:" and "after:" questions of an npc in file order: (question, key).
	// "before:" ones are only offered until the npc's side quest is done, "after:" ones only once
	// it's done (PlayMode checks the key).
	std::vector< std::pair< std::string, std::string > > const &questions(std::string const &npc);
}