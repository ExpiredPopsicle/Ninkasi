#ifndef NINKASI_OPTIMIZE_H
#define NINKASI_OPTIMIZE_H

struct NKExpressionAstNode;

// bool canOptimizeOperationWithConstants(struct NKExpressionAstNode *node);
// bool isImmediateValue(struct NKExpressionAstNode *node);
void optimizeConstants(struct NKVM *vm, struct NKExpressionAstNode **node);

#endif // NINKASI_OPTIMIZE_H

