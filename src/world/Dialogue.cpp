#include "Dialogue.hpp"

#include "../starter/data_path.hpp"

#include <fstream>
#include <map>
#include <random>
#include <stdexcept>

static constexpr char const *DIALOGUE_PATH = "data/dialogue.tsv";

namespace {
	struct Book {
		std::map< std::pair< std::string, std::string >, std::vector< std::string > > pages; //(npc, key) -> pages
		std::map< std::string, std::vector< std::pair< std::string, std::string > > > questions; //npc -> (question, key)
	};

	//"\n" (backslash, n) -> a real line break
	std::string unescape(std::string const &text) {
		std::string out;
		out.reserve(text.size());
		for (size_t i = 0; i < text.size(); ++i) {
			if (text[i] == '\\' && i + 1 < text.size() && text[i + 1] == 'n') {
				out += '\n';
				i += 1;
			} else {
				out += text[i];
			}
		}
		return out;
	}

	Book const &book() {
		static Book const loaded = []() {
			Book b;
			std::ifstream file(data_path(DIALOGUE_PATH));
			if (!file.is_open()) throw std::runtime_error{"Dialogue data file is not found"};

			std::string line;
			while (std::getline(file, line)) {
				if (!line.empty() && line.back() == '\r') line.pop_back();
				if (line.empty() || line[0] == '#') continue;

				size_t tab1 = line.find('\t');
				if (tab1 == std::string::npos) continue;
				size_t tab2 = line.find('\t', tab1 + 1);
				if (tab2 == std::string::npos) continue; //incomplete line skip it

				std::string npc = line.substr(0, tab1);
				std::string key = line.substr(tab1 + 1, tab2 - tab1 - 1);
				std::string text = unescape(line.substr(tab2 + 1));
				if (npc.empty() || key.empty()) continue;

				auto &list = b.pages[{npc, key}];
				if (list.empty() && key.rfind("ask:", 0) == 0) {
					b.questions[npc].emplace_back(key.substr(4), key);
				}
				list.emplace_back(text);
			}
			return b;
		}();
		return loaded;
	}
}

std::vector< std::string > const &Dialogue::lines(std::string const &npc, std::string const &key) {
	static std::vector< std::string > const none;
	auto const &pages = book().pages;
	auto f = pages.find({npc, key});
	return f == pages.end() ? none : f->second;
}

std::string const &Dialogue::random_line(std::string const &npc, std::string const &key) {
	static std::string const none;
	static std::mt19937 rng(0x68656c6f);
	auto const &list = lines(npc, key);
	if (list.empty()) return none;
	return list[std::uniform_int_distribution< size_t >(0, list.size() - 1)(rng)];
}

std::string Dialogue::name(std::string const &npc) {
	auto const &list = lines(npc, "name");
	return list.empty() ? npc : list[0];
}

std::vector< std::pair< std::string, std::string > > const &Dialogue::questions(std::string const &npc) {
	static std::vector< std::pair< std::string, std::string > > const none;
	auto const &all = book().questions;
	auto f = all.find(npc);
	return f == all.end() ? none : f->second;
}