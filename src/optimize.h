#ifndef NINKASI_OPTIMIZE_H
#define NINKASI_OPTIMIZE_H

struct NKExpressionAstNode;

void nkiCompilerOptimizeConstants(struct NKVM *vm, struct NKExpressionAstNode **node);

#endif // NINKASI_OPTIMIZE_H

