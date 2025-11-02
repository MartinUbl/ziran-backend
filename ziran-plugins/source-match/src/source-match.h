#pragma once

#include "../../../ziran-shared/FilesystemLib.h"
#include "../../../ziran-shared/plugin.h"
#include "../../../ziran-shared/plugin_utils.h"

#include <map>
#include <regex>

struct TTrie_Node {
	size_t id;
	std::map<char, size_t> sub;
	bool match = false;
	std::string match_word = "";

	bool Can_Advance(char c) const {
		return sub.find(c) != sub.end();
	}

	size_t Advance(char c) const {
		auto itr = sub.find(c);
		return itr->second;
	}
};

class CSource_Match_Plugin {
	private:
		struct TFile_Match {
			std::string filePath;
			std::string match;
		};

	private:
		const filesystem::path mBase_Path;
		std::string mWord_Set = "cpp";
		ziran::IReporter& mReporter;
		ziran::IEnvironment& mEnv;

		std::vector<TTrie_Node> mTrie_Nodes;

		std::vector<size_t> mTrie_States;

		std::string mExtensions = "";
		bool mExtensions_Is_Regex = false;

		bool mMust_Match = true;

		std::regex mExt_Regex;

	protected:
		bool Build_Trie();

		HRESULT Validate_File(const filesystem::path& path, std::vector<TFile_Match>& matches);

	public:
		CSource_Match_Plugin(const filesystem::path& base_path, const ziran::ParamMap& params, ziran::IReporter& reporter, ziran::IEnvironment& env);

		HRESULT Run();
};
