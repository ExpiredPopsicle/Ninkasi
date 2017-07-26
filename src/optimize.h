#ifndef OPTIMIZE_H
#define OPTIMIZE_H

struct NKExpressionAstNode;

// bool canOptimizeOperationWithConstants(struct NKExpressionAstNode *node);
// bool isImmediateValue(struct NKExpressionAstNode *node);
void optimizeConstants(struct VM *vm, struct NKExpressionAstNode **node);

#endif
