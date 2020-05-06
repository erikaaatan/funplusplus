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
    LINKEDLIST,
    LBRACKET, // [
    RBRACKET,  // ]
    ARRAYLIST,
    INSERT,
    REMOVE,
    SUB
    
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

static void error();

// ---------- ArrayList Struct ------------------
typedef struct ArrayList {
    uint64_t size;
    uint64_t arraySize;
    uint64_t *array;
} ArrayList;
 
 struct ArrayList* new_ArrayList(void) {
     ArrayList * new = (ArrayList *) malloc(sizeof(ArrayList));
     return new;
 }

// ---------- ArrayList Sizing Functions ----------
uint64_t* resizeUp(uint64_t* array, uint64_t oldSize);
void printArray(uint64_t* array, uint64_t size);
uint64_t* resizeDown(uint64_t* array, uint64_t oldSize);

 void insertArrayList(ArrayList* list, uint64_t item)
 {
     if (list->size >= (list->arraySize) / 2)
     {
         list->array = resizeUp(list->array, list->size);
         list->arraySize *= 2;
     }
     list->array[list->size] = item;
     list->size += 1;
     //printArray(list->array, list->size);
     

 }

 void removeArrayList(ArrayList* list, uint64_t index)
 {
    uint64_t oldSize = list->size;

    // Bounds Checking
    if (oldSize == 0 | index >= oldSize | index < 0)
    {
        error();
    }
    uint64_t newSize = oldSize - 1;
    
    for (int i = index; i < oldSize - 1; i++)
    {
        list->array[i] = list->array[i+1];
    }

    list->array[oldSize] = 0;
    list->size = newSize;

    if (newSize <= ((list->arraySize) / 2))
    {
        list->array = resizeDown(list->array, list->size);
        list->arraySize /= 2;
    }
 }

uint64_t* resizeUp(uint64_t* array, uint64_t oldSize)
 {
     //printf("oldsize: %ld\n", oldSize);
     uint64_t newSize = oldSize * 2;

     // TODO: Consider freeing old array
     uint64_t* newArray = (uint64_t *) malloc(newSize*sizeof(uint64_t));

     // Copy array elements
     for (int i = 0; i < oldSize; i++)
     {
         newArray[i] = array[i];
     }
     //printArray(newArray, newSize);
     return newArray;
 }

 uint64_t* resizeDown(uint64_t* array, uint64_t oldSize)
 {
     uint64_t newSize = oldSize / 2;
    // TODO: Consider freeing old array
     uint64_t* newArray = (uint64_t *) malloc(newSize*sizeof(uint64_t));

     // Copy array elements
     for (int i = 0; i < newSize; i++)
     {
         newArray[i] = array[i];
     }
    printArray(array, newSize);
    return newArray;
 }

 void printArray(uint64_t* array, uint64_t size)
 {
    for (int i = 0; i < size; i++)
    {
        printf("%ld\n", array[i]);
    }
 }

// Not needed anymore- unless if we want to use formatString for printing
char* intToString(uint64_t num)
{
    char* result;

    uint64_t copy = num;
    uint64_t numDigits = 0;

    while (copy > 0)
    {
        copy = copy / 10;
        numDigits++;
    }

    result = (char *) malloc(numDigits * sizeof(char));

    uint64_t digit = 0;

    for (int i = 0; i < numDigits; i++)
    {
        digit = num % 10;
        result[i] = digit + '0';
        num = num / 10;
    }

    return result;
    
}


// ---------- Trie Node ------------
struct Node {
    enum Kind kind;
    uint64_t data;
    struct LinkedList* head;
    struct LinkedList* tail;
    ArrayList* arraylist;
    uint64_t* array;
    int numElements;
    char* ptr;
    int end;
    struct Node* children[36];
};

// LINKEDNODES CARRY PRETOKENIZED VALUES
struct LinkedNode {
    struct Token* token;
    struct LinkedNode* next;
};

// LINKEDLISTS REPRESENT VARIABLES DECLARED IN FUN
struct LinkedList {
    uint64_t data;
    struct LinkedList* next;
};

void insertLinkedList(struct Node* symbolTableNode, uint64_t item) {
    struct LinkedList* tail = symbolTableNode->tail;
    struct LinkedList* newNode = (struct LinkedList*)malloc(sizeof(struct LinkedList));

    newNode->data = item;
    tail->next = newNode;
    symbolTableNode->tail = newNode;
    symbolTableNode->numElements += 1;
}

void removeLinkedList(struct Node* symbolTableNode, uint64_t index) {
    // Bounds Checking
    if (index >= symbolTableNode->numElements | index < 0) {
        error();
    }

    if (index == 0) {
        symbolTableNode->head = symbolTableNode->head->next;
    }
    else {
        struct LinkedList* previous = symbolTableNode->head;
        struct LinkedList* current = previous->next ;
        for (int i = 1; i < index; i++) {
            previous = current;
            current = current->next;
        }
        previous->next = current->next;
    }

    symbolTableNode->numElements -= 1;
}

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

void setLinkedList(char *id, struct LinkedList* head, struct LinkedList* tail, int numElements) {
    struct Node* current = root;
    for (int i = 0; i < strlen(id); i++) {
        int pos = getAlNumPos(id[i]);
        if (current->children[pos] == NULL) {
            current->children[pos] = newNode();
        }
        current = current->children[pos];
    }
    current->head = head;
    current->tail = tail;
    current->end = 1;
    current->numElements = numElements;
    current->kind = LINKEDLIST;
}

void setArrayAtIndex(char *id, uint64_t value, int index) {
    struct Node* symbolTableNode = getNode(id);
    enum Kind type = symbolTableNode->kind;
    uint64_t* array;
    if (type == ARRAY) {
        array = symbolTableNode->array;
        // Bounds Checking
        if (index < 0 | index >= symbolTableNode->numElements)
        {
            error();
        }
    }
    else if (type == ARRAYLIST) {
        array = symbolTableNode->arraylist->array;
        // Bounds Checking
        if (index < 0 || index >= symbolTableNode->arraylist->size) {
            error();
        }
    }
    
    array[index] = value;
}
// Set Trie Node Methods for Data structure types
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

void setArrayList(char *id, ArrayList* arraylist, int numElements) {
    struct Node* current = root;
    for (int i = 0; i < strlen(id); i++) {
        int pos = getAlNumPos(id[i]);
        if (current->children[pos] == NULL) {
            current->children[pos] = newNode();
        }
        current = current->children[pos];
    }
    current->arraylist = arraylist;
    current->end = 1;
    current->numElements = numElements;
    current->kind = ARRAYLIST;
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
        case '-': return SUB;
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
    else if (cursor + 10 < len && prog[cursor] == 'l' && prog[cursor + 1] == 'i' && prog[cursor + 2] == 'n' &&
             prog[cursor + 3] == 'k' && prog[cursor + 4] == 'e' && prog[cursor + 5] == 'd' && prog[cursor + 6] == 'l' && prog[cursor + 7] == 'i' && prog[cursor + 8] == 's' && prog[cursor + 9] == 't' && !isalnum(prog[cursor + 10])) {
        current.kind = LINKEDLIST;
        current.length = 10;
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
    else if (cursor + 6 < len && prog[cursor] == 'i' && prog[cursor + 1] == 'n' && prog[cursor + 2] == 's' &&
            prog[cursor + 3] == 'e' && prog[cursor + 4] == 'r' && prog[cursor + 5] == 't' && !isalnum(prog[cursor + 6])) {
        current.kind = INSERT;
        current.length = 6;
    }
    else if (cursor + 6 < len && prog[cursor] == 'r' && prog[cursor + 1] == 'e' && prog[cursor + 2] == 'm' && prog[cursor + 3] == 'o' &&
            prog[cursor + 4] == 'v' && prog[cursor + 5] == 'e' && !isalnum(prog[cursor + 6])) {
        current.kind = REMOVE;
        current.length = 6;
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
    while (peek() == SUB) {
        consume();
        value = value - e2();
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
            struct Node* symbolTableNode;
            consume();

            // Must be brackets or insert / remove
            if (peek() != EQ) {
                
                // Get node in symbol table 
                symbolTableNode = getNode(id);  

                // Array and ArrayList indexing
                if (peek() == LBRACKET) {
                    // array indexing
                    consume();
                    
                    // get index from expression
                    uint64_t index = expression();
                    /*
                    if (peek() == INT) {
                        index = tokenPtr->token->value;
                    }
                    else if (peek() == ID) {
                        char* id = getId();
                        struct Node* idNode = getNode(id);
                        if (idNode->kind == INT) {
                            index = idNode->data;
                        }
                        else error();
                    }
                    else error();
                    */
                    
                    if (peek() != RBRACKET) error();
                    consume();

                    if (peek() != EQ) error();
                    consume();

                    uint64_t v = expression();
                    // Set array for arrays and arraylists
                    if (doit) setArrayAtIndex(id, v, index);
                }
                // CASE: ArrayList, LinkedList, Queue insert
                else if (peek() == INSERT) {
                    consume();
                    uint64_t item = expression();
                    if (symbolTableNode->kind == ARRAYLIST) {
                        // TODO: Type checking with data structure
                        if (doit) insertArrayList(symbolTableNode->arraylist, item);

                        /*
                        if (peek() == INT) {
                            uint64_t item = getInt();
                            if(doit)
                                insertArrayList(symbolTableNode->arraylist, item);
                            consume();
                        }
                        else if (peek() == ID) {
                            char* id = getId();
                            struct Node* idNode = getNode(id);
                            if (idNode->kind == INT) {
                                uint64_t v = expression();
                                if (doit) insertArrayList(symbolTableNode->arraylist, v);
                            }
                        }
                        
                        else {
                            error();
                            }
                        */
                    }
                    else if (symbolTableNode->kind == LINKEDLIST) {
                        if (doit) insertLinkedList(symbolTableNode, item);
                    }
                }
                // CASE: ArrayList, LinkedList, Queue Remove
                else if (peek() == REMOVE) {
                    consume();
                    uint64_t index = expression();
                    if (symbolTableNode->kind == ARRAYLIST) {
                        if (doit) removeArrayList(symbolTableNode->arraylist, index);
                        /*
                        if (peek() == INT) {
                        uint64_t index = getInt();
                        removeArrayList(symbolTableNode->arraylist, index);
                        consume();
                        }
                        else if (peek() == ID) {
                            char* id = getId();
                            struct Node* idNode = getNode(id);
                            if (idNode->kind == INT)
                            {
                                uint64_t index = expression();
                                if (doit) removeArrayList(symbolTableNode->arraylist, index);
                            }
                        }
                        else {
                        error();
                        }
                        */
                    }
                    else if (symbolTableNode->kind == LINKEDLIST) {
                        if (doit) removeLinkedList(symbolTableNode, index);
                    }
                }
                return 1;
            }
            // Check for equals after ID
            if (peek() != EQ) error();
            consume();
            if (peek() == ARRAY || peek() == LINKEDLIST || peek() == ARRAYLIST) {
                enum Kind kind = peek();
                consume();
                if (peek() == TYPE_INT) {
                    consume();
                    int numElements = tokenPtr->token->value;

                    switch (kind) {
                        case ARRAY: {
                            uint64_t* newArray = (uint64_t*) malloc(numElements * sizeof(uint64_t));
                            consume();
                            for (int i = 0; i < numElements; i++) {
                                newArray[i] = tokenPtr->token->value;
                                consume();
                                if (peek() == COMMA) consume();
                            }
                            if (doit) setArray(id, newArray, numElements); 
                            break;
                        }
                        case LINKEDLIST: {
                            struct LinkedList* head = (struct LinkedList*)malloc(sizeof(struct LinkedList));
                            struct LinkedList* tail = (struct LinkedList*)malloc(sizeof(struct LinkedList));
                            consume();
                            tail->data = tokenPtr->token->value;
                            head = tail;
                            consume();
                            for (int i = 1; i < numElements; i++) {
                                if (peek() == COMMA) consume();
                                struct LinkedList* newNode = (struct LinkedList*)malloc(sizeof(struct LinkedList));
                                newNode->data = tokenPtr->token->value;
                                consume();
                                tail->next = newNode;
                                tail = newNode;
                            }
                            if (doit) setLinkedList(id, head, tail, numElements);
                            break;
                        }
                        case ARRAYLIST: {
                            ArrayList* newArrayList = new_ArrayList();
                            newArrayList->size = numElements;
                            newArrayList->array = (uint64_t*) malloc(numElements * sizeof(uint64_t));
                            consume();

                            for (int i = 0; i < numElements; i++) {
                                newArrayList->array[i] = tokenPtr->token->value;
                                consume();
                                if (peek() == COMMA) consume();
                            }
                            uint64_t testItem = 3;
                            insertArrayList(newArrayList, testItem);

                            if (doit) setArrayList(id, newArrayList, numElements);
                        }
                   }
                }
            }
            // Normal expression assignment
            else {
                uint64_t v = expression();
                if (doit) set(id, v);
            }
            return 1;
        }
        case LBRACE: {
            consume();
            seq(doit);
            if (peek() != RBRACE)
                error();
            consume();
            return 1;
        }
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
            
                // Print expression
                if (peek() != ID) printf("%"PRIu64"\n",expression());
                // Print ID value
                else {
                    char* id = getId();
                    struct Node* symbolTableNode = getNode(id);

                    // Print INT ID
                    if (symbolTableNode->kind == INT) {
                        printf("%ld\n", get(id));
                        consume();
                    }
                    else if (symbolTableNode->kind == LINKEDLIST) {
                        struct LinkedList* current = getNode(id)->head;
                        printf("{");
                        while (current != NULL) {
                            printf("%ld", current->data);
                            current = current->next;
                            if (current != NULL) printf(" ");
                        }
                        printf("}\n");
                        consume();
                    }
                    // ID is an array/arraylist
                    else {
                        // it is an array
                        uint64_t* arrayPtr;
                        uint64_t sizeOfArray;

                        // ID is an array
                        if (symbolTableNode->kind == ARRAY) {
                            arrayPtr = symbolTableNode->array; 
                            sizeOfArray = symbolTableNode->numElements;
                        }
                        // ID is an ArrayList
                        else if (symbolTableNode->kind == ARRAYLIST)
                        {
                            ArrayList* list = symbolTableNode->arraylist;
                            arrayPtr = list->array;
                            sizeOfArray = list->size;
                        }

                        // Print Array based data structure
                        // TODO: Make this printing scheme support larger numbers
                        /*
                        int formatStrSize = sizeOfArray + (sizeOfArray - 1) + 3; // size, commas, brackets, end
                        char formatStr[formatStrSize];
                        int index = 0;
                        for (int i = 0; i < formatStrSize; i++) {
                            if (i == 0) formatStr[i] = '{';
                            else if (i == formatStrSize - 2) formatStr[i] = '}';
                            else if (i == formatStrSize - 1) formatStr[i] = '\0';
                            else if (i % 2 == 0) formatStr[i] = ' ';
                            else {
                                if (arrayPtr[index] < 10) {
                                    formatStr[i] = arrayPtr[index] + '0';
                                }
                                else {
                                    formatStr[i] = intToString(arrayPtr[index]);
                                }
                                
                                index++;
                            }
                        }               
                        printf("%s\n", formatStr);
                        */
                        uint64_t formatStrSize;
                        if (sizeOfArray == 0)
                        {
                            formatStrSize = 3;
                        }
                        else {
                            formatStrSize = sizeOfArray + (sizeOfArray - 1) + 3; // size, commas, brackets, end
                        }
                        
                        char formatStr[formatStrSize];
                        int index = 0;
                        for (int i = 0; i < formatStrSize; i++) {
                            if (i == 0) printf("{");
                            else if (i == formatStrSize - 2) printf("}");
                            else if (i == formatStrSize - 1) printf("\n");
                            else if (i % 2 == 0) printf(" ");
                            else {
                                printf("%ld", arrayPtr[index]);
                                index++;
                            }
                        }
                        consume();
                    }
                }
            }
            // Consume tokens if not being executed
            else {
                if (getNode(getId()) != NULL && (peek() == ID && getNode(getId())->kind == LINKEDLIST | getNode(getId())->kind == ARRAY | getNode(getId())->kind == ARRAYLIST)) consume();
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
        case INSERT: return "insert";
        case REMOVE: return "remove";
        case LBRACKET: return "[";
        case RBRACKET: return "]";
        case SUB: return "sub";
        
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

