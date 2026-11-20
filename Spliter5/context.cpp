#include "context.h"

const VariableEntry* VariableTable::Find(const std::string& name) const {
	auto it = table_.find(name);
	return (it != table_.end()) ? &it->second : nullptr;
}

void VariableTable::Set(const std::string& name, VariableEntry entry) {
	table_[name] = std::move(entry);
}

void VariableTable::Erase(const std::string& name) {
	table_.erase(name);
}
