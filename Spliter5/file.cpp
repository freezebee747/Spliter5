#include "file.h"

#include <algorithm>
#include <filesystem>
#include <regex>
#include <system_error>

bool IFileLookup::Exists(const std::string& filename) const {
	return Filenames().count(filename) != 0;
}

std::vector<std::string> IFileLookup::Glob(const std::string& pattern) const {
	std::vector<std::string> matched;
	if (!HasGlobChars(pattern)) {
		if (Exists(pattern)) matched.push_back(pattern);
		return matched;
	}
	std::regex rx(GlobToRegex(pattern));
	for (const auto& name : Filenames()) {
		if (std::regex_match(name, rx)) matched.push_back(name);
	}
	std::sort(matched.begin(), matched.end());
	return matched;
}

bool HasGlobChars(const std::string& text) {
	return text.find_first_of("*?[") != std::string::npos;
}

std::string GlobToRegex(const std::string& glob) {
	std::string regex = "^";
	for (char c : glob) {
		switch (c) {
		case '*': regex += ".*"; break;
		case '?': regex += "."; break;
		case '[': regex += "["; break;
		case ']': regex += "]"; break;
		case '.': case '^': case '$': case '|': case '(': case ')':
		case '+': case '{': case '}': case '\\':
			regex += '\\';
			regex += c;
			break;
		default:
			regex += c;
		}
	}
	regex += "$";
	return regex;
}

DirSingleton& DirSingleton::GetInstance() {
	static DirSingleton instance;
	return instance;
}

void DirSingleton::SetDir(const std::string& dir) {
	dir_ = dir;
	NotifyObservers();
}

void DirSingleton::AttachObserver(IDirObserver* observer) {
	observers_.push_back(observer);
}

void DirSingleton::DetachObserver(IDirObserver* observer) {
	observers_.erase(std::remove(observers_.begin(), observers_.end(), observer), observers_.end());
}

void DirSingleton::NotifyObservers() {
	for (auto* observer : observers_) {
		observer->OnDirChanged(dir_);
	}
}

FileManagement::FileManagement() : dir_(DirSingleton::GetInstance().GetDir()) {
	DirSingleton::GetInstance().AttachObserver(this);
	Rescan();
}

FileManagement::~FileManagement() {
	DirSingleton::GetInstance().DetachObserver(this);
}

void FileManagement::OnDirChanged(const std::string& new_dir) {
	dir_ = new_dir;
	Rescan();
}

void FileManagement::Rescan() {
	namespace fs = std::filesystem;
	filenames_.clear();
	std::error_code ec;
	valid_dir_ = fs::is_directory(dir_, ec);
	if (!valid_dir_) return;

	for (const auto& entry : fs::directory_iterator(dir_, ec)) {
		if (entry.is_regular_file(ec)) {
			filenames_.insert(entry.path().filename().string());
		}
	}
}
