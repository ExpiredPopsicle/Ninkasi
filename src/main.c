#include "common.h"

// ----------------------------------------------------------------------

enum TokenType
{
    TOKENTYPE_INTEGER,
    TOKENTYPE_FLOAT,
    TOKENTYPE_PLUS,
    TOKENTYPE_INCREMENT,
    TOKENTYPE_PAREN_OPEN,
    TOKENTYPE_PAREN_CLOSE
};

struct Token
{
    enum TokenType type;
    char *string;
};

bool isWhitespace(char c)
{
    if(c == ' ' || c == '\n' || c == '\t' || c == '\r') {
        return true;
    }
    return false;
}

bool isNumber(char c)
{
    return (c >= '0' && c <= '9');
}

void addToken(enum TokenType type, const char *str)
{
    // TODO
    printf("Token (%d): %s\n", type, str);
}

void tokenize(const char *str)
{
    uint32_t len = strlen(str);
    uint32_t i = 0;

    while(i < len) {

        // Skip whitespace.
        while(i < len && isWhitespace(str[i])) {
            i++;
        }

        if(str[i] == '(') {

            addToken(TOKENTYPE_PAREN_OPEN, "(");

        } else if(str[i] == ')') {

            addToken(TOKENTYPE_PAREN_CLOSE, ")");

        } else if(str[i] == '+') {

            // Check for "++".
            if(str[i+1] == '+') {
                addToken(TOKENTYPE_INCREMENT, "++");
                i++;
            } else {
                addToken(TOKENTYPE_PLUS, "+");
            }

        } else if(isNumber(str[i])) {

            // Scan until we find the end of the number.
            uint32_t numberStart = i;
            bool isFloat = false;
            char tmp[256];

            while(i < len &&
                (isNumber(str[i]) || str[i] == '.') &&
                (i - numberStart) < sizeof(tmp) - 1)
            {
                tmp[i - numberStart] = str[i];
                if(str[i] == '.') {
                    isFloat = true;
                }
                i++;
            }

            // Add null terminator.
            tmp[i - numberStart] = 0;

            // Back up one character, so we don't eat the first
            // character of whatever comes next.
            i--;

            addToken(isFloat ? TOKENTYPE_FLOAT : TOKENTYPE_INTEGER, tmp);
        }

        i++;
    }
}





int main(int argc, char *argv[])
{
    struct VMStack stack;
    vmstack_init(&stack);

    // struct Value *t1 = vmstack_push(&stack);
    // struct Value *t2 = vmstack_push(&stack);

    // t1->type = VALUETYPE_INT;
    // t2->type = VALUETYPE_INT;
    // t1->intData = 123;
    // t2->intData = 456;

    vmstack_pushInt(&stack, 123);
    vmstack_pushInt(&stack, 456);

    opcode_add(&stack);

    vmstack_pushInt(&stack, 123);
    vmstack_pushInt(&stack, 456);

    vmstack_pushInt(&stack, 123);
    vmstack_pushInt(&stack, 456);

    vmstack_dump(&stack);

    vmstack_destroy(&stack);





    printf("Tokenize test...\n");
    tokenize("(123 + 456) * 789++");


    return 0;
}

