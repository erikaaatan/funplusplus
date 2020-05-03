#include <stdint.h>

#include <sys/resource.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

#define MISSING() printf("missing %s:%d\n",__FILE__,__LINE__)

char* prog;
char* original;
int len;
int nodeIndex;
struct LinkedNode* head = NULL;
struct LinkedNode* tail = NULL;
struct LinkedNode* tokenPtr = NULL;

/* Kinds of tokens */
enum Kind {
    ELSE,    // else
    END,     // <end of string>
    EQ,      // =
    EQEQ,    // ==
    ID,      // <identifier>
    IF,      // if
    INT,     // <integer value >
    LBRACE,  // {
    LEFT,    // (
    MUL,     // *
    NONE,    // <no valid token>
    PLUS,    // +
    PRINT,   // print
    RBRACE,  // }
    RIGHT,   // )
    WHILE,    // while
    FUN,      // function
    DEC,      // function declaration
    ARRAY,
    TYPE_INT,
    COMMA,
    LBRACKET, // [
    RBRACKET,  // ]
    ARRAYLIST
};

/* information about a token */
struct Token {
    enum Kind kind;
    uint64_t value;
    char *ptr;
    char *start;
    int length;
    int extra; // used for functions that have spaces between parentheses
    int index;
};

struct Node {
    enum Kind kind;
    uint64_t data;
    uint64_t* array;
    int numElements;
    char* ptr;
    int end;
    struct Node* children[36];
};

struct LinkedNode {
    struct Token* token;
    struct LinkedNode* next;
};

struct Node* newNode(void) {
    struct Node* new = (struct Node*) malloc(sizeof(struct Node));
    for (int i = 0; i < 36; i++) {
        new->children[i] = NULL;
    }
    return new;
}
 
struct Node* root = NULL;

/* The symbol table */

int getAlNumPos(char c) {
    if (isdigit(c)) return (int) c - 22;
    return (int) c - 97;
}

int inSymbolTable(char *id) {
    struct Node* current = root;
    for (int i = 0; i < strlen(id); i++) {
        int pos = getAlNumPos(id[i]);
        if (!current->children[pos]) return 0;
        current = current->children[pos];
    }
    return 1;
}

uint64_t get(char *id) {
    if (!inSymbolTable(id)) return 0;
    struct Node* current = root;
    for (int i = 0; i < strlen(id); i++) {
        int pos = getAlNumPos(id[i]);
        current = current->children[pos];
    }
    return current->data;
}

struct Node* getNode(char *id) {
    if (!inSymbolTable(id)) return 0;
    struct Node* current = root;
    for (int i = 0; i < strlen(id); i++) {
        int pos = getAlNumPos(id[i]);
        current = current->children[pos];
    }
    return current;
}

void setArrayAtIndex(char *id, uint64_t value, int index) {
    struct Node* symbolTableNode = getNode(id);
    uint64_t* array = symbolTableNode->array;
    array[index] = value;
}

void setArray(char *id, uint64_t* array, int numElements) {
    struct Node* current = root;
    for (int i = 0; i < strlen(id); i++) {
        int pos = getAlNumPos(id[i]);
        if (current->children[pos] == NULL) {
            current->children[pos] = newNode();
        }
        current = current->children[pos];
    }
    current->array = array;
    current->end = 1;
    current->numElements = numElements;
    current->kind = ARRAY;
}

void set(char *id, uint64_t value) {
    struct Node* current = root;
    for (int i = 0; i < strlen(id); i++) {
        int pos = getAlNumPos(id[i]);
        if (current->children[pos] == NULL) {
            current->children[pos] = newNode();
        }
        current = current->children[pos];
    }
    current->data = value;
    current->end = 1;
    current->kind = INT;
}

/* The current token */
static struct Token current = { NONE, 0, NULL, NULL, 0 };

static jmp_buf escape;

enum Kind peek();

static char *remaining() {
    return prog;
}

static void error() {
    printf("error at '%s'\n", remaining());
    longjmp(escape, 1);
}

enum Kind getOperatorKind(char chr) {
    switch (chr) {
        case '{': return LBRACE;
        case '(': return LEFT;
        case '*': return MUL;
        case '+': return PLUS;
        case '}': return RBRACE;
        case ')': return RIGHT;
        case '\0': return END;
        default: return NONE;
    }
}

uint64_t getIntValue(char* start, int length) {
    uint64_t res = 0;
    for (int i = 0; i < length; i++) {
        if (start[i] != '_') {
            res *= 10;
            res += start[i] - 48;
        }
    }
    return res;
}

void setCurrentToken(void) {
    if (current.kind != NONE) {
        return;
    }

    int cursor = 0;
    while (cursor < len && (isspace(prog[cursor]) || prog[cursor] == '_')) {
        cursor++;
    }

    current.start = prog + cursor;

    if (getOperatorKind(prog[cursor]) != NONE) {
        current.kind = getOperatorKind(prog[cursor]);
        current.length = 1;
    }
    else if (prog[cursor] == ',') {
        current.kind = COMMA;
        current.length = 1;
    }
    else if (prog[cursor] == '[') {
        current.kind = LBRACKET;
        current.length = 1;
    }
    else if (prog[cursor] == ']') {
        current.kind = RBRACKET;
        current.length = 1;
    }
    else if (isdigit(prog[cursor])) {
        int currLength = 0;
        int underscores = 0;
        while (cursor < len && (isdigit(prog[cursor]) || prog[cursor] == '_')) {
            if (prog[cursor] == '_') underscores++;
            cursor++;
            currLength++;
        }
        current.kind = INT;
        current.length = currLength;
        current.value = getIntValue(current.start, current.length);
    }
    else if (prog[cursor] == '=') {
        if (cursor + 1 < len && prog[cursor + 1] == '=') {
            current.kind = EQEQ;
            current.length = 2;
        } else {
            current.kind = EQ;
            current.length = 1;
        }
    }
    else if (cursor + 2 < len && prog[cursor] == 'i' && prog[cursor + 1] == 'f' && !isalnum(prog[cursor + 2])) {
        current.kind = IF;
        current.length = 2;
    }
    else if (cursor + 3 < len && prog[cursor] == 'i' && prog[cursor + 1] == 'n'&& prog[cursor + 2] == 't' && !isalnum(prog[cursor + 3])) {
        current.kind = TYPE_INT;
        current.length = 3;
    }
    else if (cursor + 5 < len && prog[cursor] == 'a' && prog[cursor + 1] == 'r' && prog[cursor + 2] == 'r' && prog[cursor + 3] == 'a' && prog[cursor + 4] == 'y' && !isalnum(prog[cursor + 5])) {
        current.kind = ARRAY;
        current.length = 5;
    }
    else if (cursor + 3 < len && prog[cursor] == 'f' && prog[cursor + 1] == 'u' && prog[cursor + 2] == 'n' && !isalnum(prog[cursor + 3])) {
        current.kind = DEC;
        current.length = 3;
    }
    else if (cursor + 4 < len && prog[cursor] == 'e' && prog[cursor + 1] == 'l' && prog[cursor + 2] == 's' && prog[cursor + 3] == 'e' &&
             !isalnum(prog[cursor + 4])) {
        current.kind = ELSE;
        current.length = 4;
    }
    else if (cursor + 5 < len && prog[cursor] == 'p' && prog[cursor + 1] == 'r' && prog[cursor + 2] == 'i' &&
             prog[cursor + 3] == 'n' && prog[cursor + 4] == 't' && !isalnum(prog[cursor + 5])) {
        current.kind = PRINT;
        current.length = 5;
    }
    else if (cursor + 5 < len && prog[cursor] == 'w' && prog[cursor + 1] == 'h' && prog[cursor + 2] == 'i' &&
             prog[cursor + 3] == 'l' && prog[cursor + 4] == 'e' && !isalnum(prog[cursor + 5])) {
        current.kind = WHILE;
        current.length = 5;
    }
    else if (cursor + 9 < len && prog[cursor] == 'a' && prog[cursor + 1] == 'r' && prog[cursor + 2] == 'r' &&
            prog[cursor + 3] == 'a' && prog[cursor + 4] == 'y' && prog[cursor + 5] == 'l' && prog[cursor + 6] == 'i' &&
            prog[cursor + 7] == 's' && prog[cursor + 8] == 't' && !isalnum(prog[cursor + 9])) {
                current.kind = ARRAYLIST;
                current.length = 9;
    }
    else {
        // it's an identifier or function
        int currLength = 0;
        while (cursor < len && isalnum(prog[cursor])) {
            cursor++;
            currLength++;
        }
        int extra = 0;
        while (cursor < len && isspace(prog[cursor])) {
            cursor++;
            extra++;
        }
        if (prog[cursor] == '(') {
            extra++;
            while (prog[cursor] != ')') {
                cursor++;
                extra++;
            }
            current.kind = FUN;
            current.length = currLength + extra;
            current.extra = extra;
        }
        else {
            current.kind = ID;
            current.length = currLength;
        }
    }
    current.index = nodeIndex;
    nodeIndex++;
}

enum Kind peek(void) {
    return (*tokenPtr->token).kind;
}

void consume(void) {
    tokenPtr = tokenPtr->next;
}

void pretokenConsume(void) {
    // used for consuming the string at the beginning instead of moving down the linked list of tokens
    current.kind = NONE;
    prog = current.start + current.length;
    if (prog[0] == '\0') {
        current.kind = END;
        current.index = nodeIndex;
    }
}

char *getId(void) {
    struct Token* current = tokenPtr->token;
    char* valStr = malloc(current->length);
    strncpy(valStr, current->start, current->length);
    return valStr;
}

char *getFunId() {
    struct Token* current = tokenPtr->token;
    char* valStr = malloc(current->length - current->extra);
    strncpy(valStr, current->start, current->length - current->extra);
    return valStr;
}

uint64_t getInt(void) {
    return tokenPtr->token->value;
}

uint64_t expression(void);
void seq(int doit);
uint64_t statement(int doit);

/* handle id, literals, and (...) */
uint64_t e1(void) {
    if (peek() == LEFT) {
        consume();
        uint64_t v = expression();
        if (peek() != RIGHT) {
            error();
        }
        consume();
        return v;
    } else if (peek() == INT) {
        uint64_t v = getInt();
        consume();
        return v;
    } else if (peek() == ID) {
        char *id = getId();
        consume();
        return get(id);
    } else if (peek() == DEC) {
        consume();
        uint64_t v = tokenPtr->token->index;
        // don't execute this function
        statement(0);
        // hash of tokenPtr
        return v;
    }
    else {
        error();
        return 0;
    }
}

/* handle '*' */
uint64_t e2(void) {
    uint64_t value = e1();
    while (peek() == MUL) {
        consume();
        value = value * e1();
    }
    return value;
}

/* handle '+' */
uint64_t e3(void) {
    uint64_t value = e2();
    while (peek() == PLUS) {
        consume();
        value = value + e2();
    }
    return value;
}

/* handle '==' */
uint64_t e4(void) {
    uint64_t value = e3();
    while (peek() == EQEQ) {
        consume();
        value = value == e3();
    }
    return value;
}

uint64_t expression(void) {
    return e4();
}

void moveTokenPtrToIndex(int index) {
    tokenPtr = head;
    for (int i = 0; i < index; i++) {
        tokenPtr = tokenPtr->next;
    }
}

uint64_t statement(int doit) {
    switch(peek()) {
        case ID: {
            char *id = getId();
            consume();
            if (peek() == LBRACKET) {
                // array indexing
                consume();
                if (peek() != INT) error();
                int index = tokenPtr->token->value;
                consume();
                if (peek() != RBRACKET) error();
                consume();

                if (peek() != EQ) error();
                consume();

                uint64_t v = expression();
                if (doit) setArrayAtIndex(id, v, index);
            }
            else {
                if (peek() != EQ) error();

                consume();
                if (peek() == ARRAY) {
                    consume();
                    if (peek() == TYPE_INT) {
                        consume();
                        int numElements = tokenPtr->token->value;
                        uint64_t* newArray = (uint64_t*) malloc(numElements * sizeof(uint64_t));
                        consume();
                        for (int i = 0; i < numElements; i++) {
                            newArray[i] = tokenPtr->token->value;
                            consume();
                            if (peek() == COMMA) consume();
                        }
                        if (doit) setArray(id, newArray, numElements); 
                    }
                }
                else {
                    uint64_t v = expression();
                    if (doit) set(id, v);
                }
            }
            return 1;
        }
        case LBRACE:
            consume();
            seq(doit);
            if (peek() != RBRACE)
                error();
            consume();
            return 1;
        case IF: {
            consume();
            uint64_t v = expression();
            if (v) {
                statement(doit);
                if (peek() == ELSE) {
                    consume();
                    statement(0);
                }
            }
            else {
                // skip the next statement
                statement(0);
                if (peek() == ELSE) {
                    consume();
                    statement(doit);
                }
            }
            return 1;
        }
        case WHILE: {
            consume();
            int expressionIndex = tokenPtr->token->index;
            uint64_t v = expression();
            while (v && doit) {
                statement(doit);
                // the statement was evaluated to true
                moveTokenPtrToIndex(expressionIndex);
                v = expression();
            }
            statement(0);
            return 1;
        }
        case FUN: {
            char* funId = getFunId();
            consume();
            if (!doit) return 1;
            int returnIndex = tokenPtr->token->index;
            // go to the start of the function
            moveTokenPtrToIndex(get(funId));
            statement(doit);
            // move it back to where it was before
            moveTokenPtrToIndex(returnIndex);
            return 1;
        }
        case PRINT: {
            consume();
            if (doit) {
                if (peek() != ID) printf("%"PRIu64"\n",expression());
                else {
                    char* id = getId();
                    struct Node* symbolTableNode = getNode(id);
                    if (symbolTableNode->kind == INT) printf("%ld\n", get(id));
                    else {
                        // it is an array
                        uint64_t* arrayPtr = symbolTableNode->array; 
                        int sizeOfArray = symbolTableNode->numElements;

                        int formatStrSize = sizeOfArray + (sizeOfArray - 1) + 3; // size, commas, brackets, end
                        char formatStr[formatStrSize];
                        int index = 0;
                        for (int i = 0; i < formatStrSize; i++) {
                            if (i == 0) formatStr[i] = '{';
                            else if (i == formatStrSize - 2) formatStr[i] = '}';
                            else if (i == formatStrSize - 1) formatStr[i] = '\0';
                            else if (i % 2 == 0) formatStr[i] = ' ';
                            else {
                                formatStr[i] = arrayPtr[index] + '0';
                                index++;
                            }
                        }               
                        printf("%s\n", formatStr);
                        consume();
                    }
                }
            }
            else {
                if (peek() == ID && getNode(getId())->kind == ARRAY) consume();
                else expression();
            }
            return 1;
        }
        default:
            return 0;
    }
}

void seq(int doit) {
    while (statement(doit)) ;
}

void program(void) {
    seq(1);
    if (peek() != END)
        error();
}

void interpret(char *prog) {
    current.kind = NONE;
    int x = setjmp(escape);
    if (x == 0) {
        program();
    }
}

char *customfgets(char *dst, int max, FILE *fp) {
	int c;
	char *p;

	for (p = dst, max--; max > 0; max--) {
		if ((c = fgetc (fp)) == EOF)
			break;
		*p++ = c;
	}
	*p = 0;
	if (p == dst || c == EOF)
		return NULL;
	return (p);
}

void increaseStackSize(void ) {
    // citation: https://stackoverflow.com/questions/2279052/increase-stack-size-in-linux-with-setrlimit
    const rlim_t kStackSize = 512L * 1024L * 1024L;   // min stack size = 64 Mb
    struct rlimit rl;
    int result;

    result = getrlimit(RLIMIT_STACK, &rl);
    if (result == 0)
    {
        if (rl.rlim_cur < kStackSize)
        {
            rl.rlim_cur = kStackSize;
            result = setrlimit(RLIMIT_STACK, &rl);
            if (result != 0)
            {
                fprintf(stderr, "setrlimit returned result = %d\n", result);
            }
        }
    }
}

void readFile(int argc, char* argv[]) {
    // goal: set the global variables "prog" and "original"

    FILE *filePointer;
    char* file = argv[1];
    // save memory by getting length of file and allocating that much space for it
    filePointer = fopen(file, "r");
    fseek(filePointer, 0L, SEEK_END);
    len = ftell(filePointer);

    fseek(filePointer, 0L, SEEK_SET);
    char* message = (char*) malloc(len + 1);
    customfgets(message, len, filePointer);
    message[len] = '\0';
    prog = message;
    original = message;
}

struct Token* copyToken(struct Token* current) {
    struct Token* copy = malloc(sizeof(struct Token));
    *copy = *current; 
    return copy; 
}

void pretokenize(void) {
    // goal: update the linked list of tokens completely
    head = (struct LinkedNode*) malloc(sizeof(struct LinkedNode));
    setCurrentToken();
    head->token = copyToken(&current);
    tail = head;
    pretokenConsume();
    tokenPtr = head;
    do {
        struct LinkedNode* newest = (struct LinkedNode*) malloc(sizeof(struct LinkedNode));
        setCurrentToken();
        newest->token = copyToken(&current);
        tail->next = newest;
        tail = newest;
        pretokenConsume();
    }
    while (tail->token->kind != END);
}

char* stringifyKind(enum Kind kind) {
    switch (kind) {
        case END: return "end";
        case ELSE: return "else";
        case EQ: return "eq";
        case EQEQ: return "eqeq";
        case ID: return "id";
        case IF: return "if";
        case INT: return "int";
        case LBRACE: return "lbrace";
        case LEFT: return "left";
        case MUL: return "Mul";
        case NONE: return "none";
        case PLUS: return "plus";
        case PRINT: return "print";
        case RBRACE: return "rbrace";
        case RIGHT: return "right";
        case WHILE: return "while";
        case FUN: return "fun";
        case DEC: return "dec";
        case ARRAY: return "array";
        case TYPE_INT: return "type_int";
        case COMMA: return "comma";
        case ARRAYLIST: return "arraylist";
    }
}

int main(int argc, char* argv[]) {
    increaseStackSize();
    readFile(argc, argv);

    // trie symbol table
    root = newNode();

    pretokenize();

   /* 
    do {
        printf("%s\n", stringifyKind(tokenPtr->token->kind));
        consume();
    }
    while (tokenPtr->token->kind != END);
*/
    

    interpret(prog);
    return 0;
}

