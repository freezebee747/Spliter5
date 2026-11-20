#include "directive.h"

namespace {

DirectiveHandler IncludeHandler(bool optional) {
	return [optional](const DirectiveCall& call) {
		if (!call.include) return;
		for (const auto& path : call.arguments) {
			call.include(path, optional);
		}
	};
}

void Ignore(const DirectiveCall&) {}

}  // namespace

DirectiveRegistry DirectiveRegistry::WithBuiltins() {
	DirectiveRegistry r;
	r.Register("include", IncludeHandler(false));
	r.Register("-include", IncludeHandler(true));
	r.Register("sinclude", IncludeHandler(true));

	// 인식만 하고 아직 검사하지 않는 지시어
	for (const char* name : { "vpath", "override", "export", "unexport",
							  "ifeq", "ifneq", "ifdef", "ifndef", "else", "endif" }) {
		r.Register(name, Ignore);
	}
	return r;
}

void DirectiveRegistry::Register(const std::string& name, DirectiveHandler handler) {
	handlers_[name] = std::move(handler);
}

bool DirectiveRegistry::IsDirective(const std::string& keyword) const {
	return handlers_.count(keyword) != 0;
}

void DirectiveRegistry::Run(const DirectiveCall& call) const {
	auto it = handlers_.find(call.node.name);
	if (it != handlers_.end()) it->second(call);
}
