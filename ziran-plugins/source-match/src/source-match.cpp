#include "source-match.h"

#include <fstream>
#include <sstream>

CSource_Match_Plugin::CSource_Match_Plugin(const filesystem::path& base_path, const ziran::ParamMap& params, ziran::IReporter& reporter, ziran::IEnvironment& env)
	: mBase_Path(base_path), mReporter(reporter), mEnv(env) {

	std::string str;

	if (ziran::Param_Get(params, "wordset", str)) {
		mWord_Set = str;
	}

	if (ziran::Param_Get(params, "extensions", str)) {
		mExtensions = str;
		mExtensions_Is_Regex = false;
	}

	if (ziran::Param_Get(params, "extensions_regex", str)) {
		mExtensions = str;
		mExtensions_Is_Regex = true;
	}

	if (ziran::Param_Get(params, "match", str)) {
		if (str == "positive" || str == "true") {
			mMust_Match = true;
		}
		else if (str == "negative" || str == "false") {
			mMust_Match = false;
		}
	}

	if (mExtensions.empty()) {
		mExt_Regex = std::regex{ ".*", std::regex::ECMAScript | std::regex::icase };
	}
	else if (mExtensions_Is_Regex) {
		mExt_Regex = std::regex{ mExtensions, std::regex::ECMAScript | std::regex::icase };
	}
	else {
		std::string base = "(";

		std::string ext;
		std::istringstream iss(mExtensions);
		while (std::getline(iss, ext, ',')) {
			if (base.length() > 1) {
				base += "|";
			}
			base += "\\." + ext;
		}

		base += ")";

		mExt_Regex = std::regex{ base, std::regex::ECMAScript | std::regex::icase };
	}
}

bool CSource_Match_Plugin::Build_Trie() {
	mTrie_Nodes.resize(1);
	mTrie_Nodes[0].id = 0;
	mTrie_Nodes[0].sub.clear();
	mTrie_Nodes[0].match = false;

	auto build = [this](const std::vector<std::string>& words) {

		size_t idx = 0;

		for (const auto& w : words) {
			idx = 0;

			for (auto c : w) {
				if (mTrie_Nodes[idx].Can_Advance(c)) {
					idx = mTrie_Nodes[idx].Advance(c);
				}
				else {
					const size_t nidx = mTrie_Nodes.size();
					mTrie_Nodes[idx].sub[c] = nidx;
					mTrie_Nodes.push_back({
						nidx,
						{},
						false
					});

					idx = nidx;
				}
			}

			mTrie_Nodes[idx].match = true;
			mTrie_Nodes[idx].match_word = w;
		}

	};

	// TODO: load from file
	const char* wordSetContents = nullptr;
	if (mEnv.Get_Input(mWord_Set.c_str(), &wordSetContents) == S_OK && wordSetContents != nullptr) {

		std::vector<std::string> wordSet;
		std::istringstream iss(wordSetContents);
		std::string str;
		while (std::getline(iss, str)) {
			wordSet.push_back(ziran::string::trim_whitespaces(str));
		}

		build(wordSet);
	}
	else {
		return false;
	}

	return true;
}

HRESULT CSource_Match_Plugin::Validate_File(const filesystem::path& path, std::vector<TFile_Match>& matches) {

	std::ifstream fs(path.string());
	// could not open file for some reason - this is not of concern for a plugin
	if (!fs.is_open()) {
		return S_FALSE;
	}

	bool foundMatch = false;

	std::string line;
	while (std::getline(fs, line)) {
		mTrie_States.clear();

		for (auto c : line) {
			mTrie_States.push_back(0);

			for (auto& tr_idx : mTrie_States) {
				if (mTrie_Nodes[tr_idx].Can_Advance(c)) {
					tr_idx = mTrie_Nodes[tr_idx].Advance(c);
				}

				if (mTrie_Nodes[tr_idx].match) {
					// found a match!

					if (!mMust_Match) {
						matches.push_back({ path.filename().string(), mTrie_Nodes[tr_idx].match_word });
					}

					foundMatch = true;
				}
			}
		}
	}

	if (foundMatch) {
		if (!mMust_Match) {
			return E_UNEXPECTED;
		}
	}
	else if (mMust_Match) {
		return E_UNEXPECTED;
	}

	return S_OK;
}

HRESULT CSource_Match_Plugin::Run() {
	if (!Build_Trie()) {
		mReporter.Report(ziran::NJob_Report_Type::Error, "Nelze sestavit ověřovací strom", "runtime");
		return E_FAIL;
	}

	std::vector<TFile_Match> matches;
	bool validationSuccess = true;

	for (auto& entry : filesystem::recursive_directory_iterator(mBase_Path)) {
		if (!entry.is_regular_file()) {
			continue;
		}

		auto ext = entry.path().extension().string();
		std::smatch sm;
		if (!std::regex_match(ext, sm, mExt_Regex)) {
			continue;
		}

		HRESULT res = Validate_File(entry.path(), matches);
		validationSuccess &= Succeeded(res);
	}

	if (!validationSuccess) {

		for (auto match : matches) {
			std::string mfail = "Soubor " + match.filePath + " obsahuje zakázaný řetězec (" + match.match + ")!";
			mReporter.Report(ziran::NJob_Report_Type::Error, mfail.c_str(), "static");
		}
		return E_UNEXPECTED;
	}
	else {
		mReporter.Report(ziran::NJob_Report_Type::Info, "Přeběžná kontrola obsahu zdrojových souborů proběhla úspěšně.", "static");
	}

	return S_OK;
}
