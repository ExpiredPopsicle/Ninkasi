#ifndef NINKASI_OPTIMIZE_H
#define NINKASI_OPTIMIZE_H

struct NKExpressionAstNode;

// nkbool canOptimizeOperationWithConstants(struct NKExpressionAstNode *node);
// nkbool isImmediateValue(struct NKExpressionAstNode *node);
void optimizeConstants(struct NKVM *vm, struct NKExpressionAstNode **node);

#endif // NINKASI_OPTIMIZE_H

