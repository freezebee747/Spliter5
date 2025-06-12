#include <functional>

#include "directive.h"
#include "file.h"

std::unordered_map<std::string, std::function<void(DirectiveContext&)>> directive_table = {
    {"include", include_directive},
    {"sinclude", sinclude_directive},

};

void include_directive(DirectiveContext& ctx) {
    Parser par;
    par.parsing(ctx.filename);
    SyntaxChecker sc(par.Getnodes(), par.GetError());
    sc.SyntaxCheck();
    ctx.checker.IncludeOtherSyntax(sc, ctx.filename);
}

void sinclude_directive(DirectiveContext& ctx) {
    Parser par;
    par.parsing(ctx.filename);
    SyntaxChecker sc(par.Getnodes(), par.GetError());
    sc.SyntaxCheck();
    ctx.checker.IncludeOtherSyntax(sc);
}

