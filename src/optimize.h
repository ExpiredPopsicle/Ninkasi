#ifndef OPTIMIZE_H
#define OPTIMIZE_H

struct ExpressionAstNode;

// bool canOptimizeOperationWithConstants(struct ExpressionAstNode *node);
// bool isImmediateValue(struct ExpressionAstNode *node);
void optimizeConstants(struct ExpressionAstNode **node);

#endif
