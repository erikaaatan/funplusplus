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
int object = 0; // use during pretokenization if you are declaring an object

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
    TYPE_STRING,
    STRING,
    COMMA,
    LINKEDLIST,
    LBRACKET, // [
    RBRACKET,  // ]
    ARRAYLIST,
    QUEUE,
    ADD,
    PEEK,
    INSERT,
    REMOVE,
    SUB,
    LESS,
    GREAT,
    CLASS,
    OBJECT,
    DOT
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
    char *str;
};

static void error();

// ---------- ArrayList Struct ------------------
typedef struct ArrayList {
    uint64_t size;
    uint64_t arraySize;
    uint64_t *array;
    char **array_str;
    enum Kind kind;
} ArrayList;
 
 struct ArrayList* new_ArrayList(void) {
     ArrayList * new = (ArrayList *) malloc(sizeof(ArrayList));
     return new;
 }

// ---------- ArrayList Sizing Functions ----------
uint64_t* resizeUp(uint64_t* array, uint64_t oldSize);
void printArray(uint64_t* array, uint64_t size);
uint64_t* resizeDown(uint64_t* array, uint64_t oldSize);
char **resizeUp_str(char **array_str, uint64_t oldSize);
char **resizeDown_str(char **array_str, uint64_t oldSize);

 void insertArrayList(ArrayList* list, uint64_t item, char *str_item)
 {
    if (list->kind == INT) {
        if (list->size >= (list->arraySize) / 2)
        {
            list->array = resizeUp(list->array, list->size);
            list->arraySize *= 2;
        }
        list->array[list->size] = item;
        list->size += 1;
        //printArray(list->array, list->size);
     }
     else if (list->kind == STRING) {
        if (list->size >= (list->arraySize) / 2)
        {
            list->array_str = resizeUp_str(list->array_str, list->size);
            list->arraySize *= 2;
        }
        list->array_str[list->size] = str_item;
        list->size += 1;
     }
     
     
 }

 void removeArrayList(ArrayList* list, uint64_t index)
 {
    uint64_t oldSize = list->size;

    // Bounds Checking
    if (oldSize == 0 | index >= oldSize | index < 0)
    {
        printf("OUT OF BOUNDS\n"); 
        error();
    }
    uint64_t newSize = oldSize - 1;

    if (list->kind == INT) {
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
    else if (list->kind == STRING) {
        for (int i = index; i < oldSize - 1; i++)
        {
            list->array_str[i] = list->array_str[i+1];
        }

        list->array_str[oldSize] = 0;
        list->size = newSize;

        if (newSize <= ((list->arraySize) / 2))
        {
            list->array_str = resizeDown_str(list->array_str, list->size);
            list->arraySize /= 2;
        }
    }
    
    
 }

uint64_t* resizeUp(uint64_t* array, uint64_t oldSize) {
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

 char **resizeUp_str(char **array_str, uint64_t oldSize) {
    //printf("oldsize: %ld\n", oldSize);
    uint64_t newSize = oldSize * 2;

    char **newArray_str = (char **) malloc(newSize*sizeof(char*));

    // Copy array elements
    for (int i = 0; i < oldSize; i++)
    {
        newArray_str[i] = array_str[i];
    }
    return newArray_str;

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
    //printArray(array, newSize);
    return newArray;
 }

 char **resizeDown_str(char **array_str, uint64_t oldSize) {
    uint64_t newSize = oldSize / 2;
    // TODO: Consider freeing old array

    char ** newArray_str = (char **) malloc(newSize*sizeof(char *));

    for (int i = 0; i < newSize; i++)
    {
        newArray_str[i] = array_str[i];
    }

    return newArray_str;

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
    struct Node *objSymbolTable;
    uint64_t data;
    struct LinkedList* head;
    struct LinkedList* tail;
    ArrayList* arraylist;
    uint64_t* array;
    char** array_str;
    int numElements;
    char* ptr;
    char* str;
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
    char* str;
    struct LinkedList* next;
};

void insertLinkedList(struct Node* symbolTableNode, uint64_t item, char* str) {
    struct LinkedList* tail = symbolTableNode->tail;
    struct LinkedList* newNode = (struct LinkedList*)malloc(sizeof(struct LinkedList));

    newNode->data = item;
    newNode->str = str;
    newNode->next = NULL;
    tail->next = newNode;
    symbolTableNode->tail = newNode;
    symbolTableNode->numElements += 1;
}

void removeLinkedList(struct Node* symbolTableNode, uint64_t index) {
    // Bounds Checking
    if (index >= symbolTableNode->numElements | index < 0) {
        printf("OUT OF BOUNDS\n"); 
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

//QUEUE REMOVE 
void removeQueue(struct Node* symbolTableNode) {
    if (symbolTableNode->numElements == 0) {
        printf("OUT OF BOUNDS\n");
        error(); 
    }

    if (symbolTableNode->numElements == 1) {
        symbolTableNode->head = symbolTableNode->head->next;
        symbolTableNode->numElements -=1;
        return; 
    }

    struct LinkedList* previous = symbolTableNode->head;
    struct LinkedList* current = previous->next;

    while (current != symbolTableNode->tail) {
        previous = current; 
	    current = current->next;
    }

    previous->next = current->next;
    symbolTableNode->tail = previous;
    symbolTableNode->numElements -= 1;
}

//QUEUE Peek 

uint64_t peekQueue(struct Node* symbolTableNode) {
    if (symbolTableNode->numElements == 0) {
        printf("OUT OF BOUNDS\n");
        error(); 
    }

    return symbolTableNode->tail->data;
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

int inSymbolTable(struct Node* root, char *id) {
    struct Node* current = root;
    for (int i = 0; i < strlen(id); i++) {
        int pos = getAlNumPos(id[i]);
        if (!current->children[pos]) return 0;
        current = current->children[pos];
    }
    return 1;
}

struct Node* getNode(struct Node* root, char *id) {
    if (!inSymbolTable(root, id)) return 0;
    struct Node* current = root;
    for (int i = 0; i < strlen(id); i++) {
        int pos = getAlNumPos(id[i]);
        current = current->children[pos];
    }
    return current;
}

uint64_t get(struct Node* root, char *id) {
    if (!inSymbolTable(root, id)) return 0;
    return getNode(root, id)->data;
}

struct Node* getNewNode(struct Node* root, char *id) {
    struct Node* current = root;
    for (int i = 0; i < strlen(id); i++) {
        int pos = getAlNumPos(id[i]);
        if (current->children[pos] == NULL) {
            current->children[pos] = newNode();
        }
        current = current->children[pos];
    }
    return current;
}

void setLinkedList(char *id, struct LinkedList* head, struct LinkedList* tail, int numElements) {
    struct Node* current = getNewNode(root, id);    
    current->head = head;
    current->tail = tail;
    current->end = 1;
    current->numElements = numElements;
    current->kind = LINKEDLIST;
}

void setQueue(struct Node* root, char *id, struct LinkedList* head, struct LinkedList* tail, int numElements) {
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
    current->kind = QUEUE;
}

void setArrayAtIndex(struct Node* root, char *id, uint64_t value, char *str, int index) {
    struct Node* symbolTableNode = getNode(root, id);
    enum Kind type = symbolTableNode->kind;
    uint64_t* array;
    char** array_str;
    if (type == ARRAY) {
        array = symbolTableNode->array;
        array_str = symbolTableNode->array_str;
        // Bounds Checking
        if (index < 0 | index >= symbolTableNode->numElements) {
            printf("OUT OF BOUNDS\n"); 
            error(); 
        }
        
        if (array == NULL) array_str[index] = str;
        else array[index] = value;
    }
    else if (type == ARRAYLIST) {
        array = symbolTableNode->arraylist->array;
        array_str = symbolTableNode->arraylist->array_str;
        // Bounds Checking
        if (index < 0 || index >= symbolTableNode->arraylist->size) {
            printf("OUT OF BOUNDS\n"); 
            error();
        }
    }
    if (array == NULL) array_str[index] = str;
    else array[index] = value;
}
// Set Trie Node Methods for Data structure types
void setArray(struct Node* root, char *id, uint64_t* array, int numElements) {
    struct Node* current = getNewNode(root, id); 
    current->array = array;
    current->end = 1;
    current->numElements = numElements;
    current->kind = ARRAY;
}

void setArray_str(struct Node* root, char *id, char** array, int numElements) {
    struct Node* current = getNewNode(root, id); 
    current->array_str = array;
    current->end = 1;
    current->numElements = numElements;
    current->kind = ARRAY;
}

void setArrayList(struct Node* root, char *id, ArrayList* arraylist, int numElements) {
    struct Node* current = getNewNode(root, id);
    current->arraylist = arraylist;
    current->end = 1;
    current->numElements = numElements;
    current->kind = ARRAYLIST;
}

void set(struct Node* root, char *id, uint64_t value) {
    struct Node* current = getNewNode(root, id); 
    current->data = value;
    current->end = 1;
    current->kind = INT;
}

void set_str(struct Node* root, char *id, char *str) {
    struct Node* current = getNewNode(root, id); 
    current->str = str;
    current->end = 1;
    current->kind = STRING;
}

void set_obj(struct Node* root, char*id) {
    struct Node* current = getNewNode(root, id);
    current->kind = OBJECT;
}

struct Node* copy(struct Node* copyNode, struct Node* originalNode) {
    copyNode->objSymbolTable = (originalNode->objSymbolTable != NULL) ? originalNode->objSymbolTable : NULL;
    copyNode->data = originalNode->data;
    copyNode->head = (originalNode->head != NULL) ? originalNode->head : NULL;
    copyNode->tail = (originalNode->tail != NULL) ? originalNode->tail : NULL;
    copyNode->arraylist = originalNode->arraylist != NULL ? originalNode->arraylist : NULL;
    copyNode->array = originalNode->array != NULL ? originalNode->array : NULL;
    copyNode->array_str = originalNode->array_str != NULL ? originalNode->array_str : NULL;
    copyNode->numElements = originalNode->numElements;
    copyNode->ptr = originalNode->ptr != NULL ? originalNode->ptr : NULL;
    copyNode->str = originalNode->str != NULL ? originalNode->str : NULL;
    copyNode->end = originalNode->end;
    for (int i = 0; i < 36; i++) {
        copyNode->children[i] = originalNode->children[i] != NULL ? originalNode->children[i] : NULL;
    }
}

/*
void copyTable(struct Node* copyNode, struct Node* template) {
    copy(copyNode, template);
    for (int i = 0; i < 36; i++) {
        if (template->children[i] != NULL) {
            copyNode->children[i] = (struct Node*) malloc(sizeof(struct Node));
            copyTable(copyNode->children[i], template->children[i]);
        }
    }
}
*/

void set_newObj(struct Node* root, char* id, char* objName) {
    struct Node* current = getNewNode(root, id);
    current->kind = OBJECT;
    current->objSymbolTable = (struct Node*)malloc(sizeof(struct Node));

    // copy over the symbol table from the template
    copy(current->objSymbolTable, getNode(root, objName)->objSymbolTable);
}

struct Node* initializeObject(struct Node* root, char* id) {
    struct Node* objNode = getNode(root, id);
    objNode->objSymbolTable = newNode();
    // return root of new symbol table for this object
    return objNode->objSymbolTable; 
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
        case '<': return LESS;
        case '>': return GREAT;
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

int checkKind(int cursor, int length, char* kind) {
    if (cursor + length >= len) return 0;
    for (int i = 0; i < length; i++) {
        if (prog[cursor + i] != kind[i]) return 0;
    }
    if (isalnum(prog[cursor + length])) return 0;
    return 1;
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
    else if (prog[cursor] == '.') {
        current.kind = DOT;
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
    // May need backslash
    else if (prog[cursor] == '"') {
        int currLength = 0;
        cursor++;
        int start = cursor;
        while (cursor < len && (isalnum(prog[cursor]) || prog[cursor] == '_') && prog[cursor] != '"') {
            cursor++;
            currLength++;
        }
        cursor++;
        char *str = (char *)malloc(currLength * sizeof(char));
        strncpy(str, prog+start, currLength);
        
        current.kind = STRING;
        current.length = currLength + 2;
        current.str = str;
        
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
    else if (checkKind(cursor, 2, "if")) {
        current.kind = IF;
        current.length = 2;
    }
    else if (checkKind(cursor, 3, "int")) {
        current.kind = TYPE_INT;
        current.length = 3;
    }
    else if (checkKind(cursor, 6, "string")) {
        current.kind = TYPE_STRING;
        current.length = 6;
    }
    else if (checkKind(cursor, 5, "array")) {
        current.kind = ARRAY;
        current.length = 5;
    }
    else if (checkKind(cursor, 3, "fun")) {
        current.kind = DEC;
        current.length = 3;
    }
    else if (checkKind(cursor, 5, "else")) {
        current.kind = ELSE;
        current.length = 4;
    }
    else if (checkKind(cursor, 5, "print")) {
        current.kind = PRINT;
        current.length = 5;
    }
    else if (checkKind(cursor, 10, "linkedlist")) {
        current.kind = LINKEDLIST;
        current.length = 10;
    }
    else if (checkKind(cursor, 5, "while")) {
        current.kind = WHILE;
        current.length = 5;
    }
    else if (checkKind(cursor, 9, "arraylist")) {
        current.kind = ARRAYLIST;
        current.length = 9;
    }
    else if (checkKind(cursor, 6, "insert")) {
        current.kind = INSERT;
        current.length = 6;
    }
    else if (checkKind(cursor, 6, "remove")) {
        current.kind = REMOVE;
        current.length = 6;
    }
    else if (checkKind(cursor, 3, "add")) {
        current.kind = ADD;
        current.length = 3;
    }
    else if (checkKind(cursor, 5, "queue")) {
        current.kind = QUEUE; 
	    current.length = 5;
    }
    else if (checkKind(cursor, 4, "peek")) {
        current.kind = PEEK;
        current.length = 4;
    }
    else if (checkKind(cursor, 5, "class")) {
        current.kind = CLASS;
        current.length = 5;
        object = 1;
    }
    else {
        // it's an identifier, function, or object
        int start = cursor;
        int currLength = 0;
        while (cursor < len && isalnum(prog[cursor])) {
            cursor++;
            currLength++;
        }

        if (object) {
            char id[currLength];
            strncpy(id, prog + start, currLength);
            set_obj(root, id); 
            // Case: class declaration
            current.kind = OBJECT;
            current.length = currLength;
            object = 0; // toggle off
        }
        else {
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
                // check if this ID is an object first
                char id[currLength];
                strncpy(id, prog + start, currLength);

                if (inSymbolTable(root, id) && getNode(root, id)->kind == OBJECT) {
                    current.kind = OBJECT;
                    current.length = currLength;
                }
                else {
                    current.kind = ID;
                    current.length = currLength;
                }
            }
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

uint64_t expression(struct Node* root);
void seq(struct Node* root, int doit);
uint64_t statement(struct Node* root, int doit);

/* handle id, literals, and (...) */
uint64_t e1(struct Node* root) {
    if (peek() == LEFT) {
        consume();
        uint64_t v = expression(root);
        if (peek() != RIGHT) {
            printf("MISSING RIGHT PARENTHESIS\n");
            error();
        }
        consume();
        return v;
    }  else if (peek() == INT) {
        uint64_t v = getInt();
        consume();
        return v;
    }  /*else if (peek() == ID && tokenPtr->token->kind == STRING) {
        consume();
        return (uint64_t) tokenPtr->token->str;
    }*/else if (peek() == ID) {
        char *id = getId();
        struct Node* symbolTableNode = getNode(root, id);
        consume();
	
	//Queue Peek management
	if (peek() == PEEK)
        {
            struct Node* symbolTableNode = getNode(root, id);
            uint64_t returnVal = peekQueue(symbolTableNode);
            consume();
            return returnVal;
        }
        // Array Indexing for arrays and arraylists
	else if (peek() == LBRACKET) {
            consume();
            uint64_t index = expression(root);
            if (peek() != RBRACKET) {
                printf("MISSING RIGHT BRACKET\n");
                error();
            }
            consume();

            // ONly for int array / arraylists
            if (symbolTableNode->kind == ARRAY) {
                // String array
                if (index >= 0 && index < symbolTableNode->numElements) {
                    //printf("index: %ld\n", index);
                    return symbolTableNode->array[index];
                }    
            }
            else if (symbolTableNode->kind == ARRAYLIST && symbolTableNode->arraylist->kind == INT) {
                if (index >= 0 && index < symbolTableNode->arraylist->size) {
                    //printf("index: %ld\n", index);
                    return symbolTableNode->arraylist->array[index];
                }
            }
        }
        return get(root, id);
    } /*else if (peek() == STRING) {
        consume();
        return (uint64_t) tokenPtr->token->str;
    }*/
    else if (peek() == DEC) {
        consume();
        uint64_t v = tokenPtr->token->index;
        // don't execute this function
        statement(root, 0);
        // hash of tokenPtr
        return v;
    }
    else {
        printf("UNKNOWN DECLARATION\n");
        error();
        return 0;
    }
}

/* handle '*' */
uint64_t e2(struct Node* root) {
    uint64_t value = e1(root);
    while (peek() == MUL) {
        consume();
        value = value * e1(root);
    }
    return value;
}

/* handle '+' */
uint64_t e3(struct Node* root) {
    uint64_t value = e2(root);
    while (peek() == PLUS) {
        consume();
        value = value + e2(root);
    }
    while (peek() == SUB) {
        consume();
        value = value - e2(root);
    }
    return value;
}

/* handle '==' */
uint64_t e4(struct Node* root) {
    uint64_t value = e3(root);
    while (peek() == EQEQ) {
        consume();
        value = value == e3(root);
    }
    while (peek() == LESS) {
        consume();
        if (value < e3(root)) {
            return 1;
        }
        return 0;
    }
    while (peek() == GREAT) {
        consume();
        if (value > e3(root)) {
            return 1;
        }
        return 0;
    }
    return value;
}

uint64_t expression(struct Node* root) {
    return e4(root);
}

void moveTokenPtrToIndex(int index) {
    tokenPtr = head;
    for (int i = 0; i < index; i++) {
        tokenPtr = tokenPtr->next;
    }
}

uint64_t statement(struct Node* root, int doit) {
    switch(peek()) {
	    case ID: { 
            char *id = getId();
            struct Node* symbolTableNode;
            consume();
		
            // Must be brackets or insert / remove
            if (peek() != EQ) {
                
                // Get node in symbol table 
                symbolTableNode = getNode(root, id);  

                // object function
                if (peek() == DOT) {
                    consume();
                    char* funId = getFunId();
                    consume();
                    if (!doit) return 1;
                    int returnIndex = tokenPtr->token->index;
                    // go to the start of the function
                    struct Node* newRoot = symbolTableNode->objSymbolTable;
                    moveTokenPtrToIndex(get(newRoot, funId));
                    statement(newRoot, doit);
                    // move it back to where it was before
                    moveTokenPtrToIndex(returnIndex);
                    return 1;
                }
                // Array and ArrayList indexing
                else if (peek() == LBRACKET) {
                    consume();
                    uint64_t index = expression(root );
                    if (peek() != RBRACKET) {
                        printf("MISSING RIGHT BRACKET\n");
                        error();
                    }
                        consume();

                    if (peek() != EQ) {
                        printf("UNKNOWN COMMAND");
                        error();
                    }
                        consume();

                    char* str;
                    uint64_t v;
                    if (peek() == STRING) {
                        str = tokenPtr->token->str;
                        consume();
                    }
                    else v = expression(root);

                    // Set array for arrays and arraylists
                    if (doit) setArrayAtIndex(root, id, v, str, index);
                    
                }
                // CASE: ArrayList, LinkedList, Queue insert
                else if (peek() == INSERT) {
                    consume();
                    uint64_t item;
                    char *item_str = NULL;
                    if (symbolTableNode->kind == ARRAYLIST) {
                        // TODO: Type checking with data structure
                        if (symbolTableNode->arraylist->kind == STRING) {
                            item_str = tokenPtr->token->str;
                            consume();
                        }
                        else if (symbolTableNode->arraylist->kind == INT) {
                            item = expression(root);
                        }
                        if (doit) insertArrayList(symbolTableNode->arraylist, item, item_str);

                    }
                    else if (symbolTableNode->kind == LINKEDLIST) {
                        if (peek() == STRING) {
                            item_str = tokenPtr->token->str;
                            consume();
                        }
                        else {
                            item = expression(root);
                        }
                        if (doit) insertLinkedList(symbolTableNode, item, item_str);
                    }
                }
                else if (peek() == ADD) {
                    consume();
                    uint64_t item = expression(root); 
                    if (doit) insertLinkedList(symbolTableNode, item, NULL);
                }
                // CASE: ArrayList, LinkedList, Queue Remove
                else if (peek() == REMOVE) {
                    consume();
                    // queues don't need a specific index to remove from
                    if (symbolTableNode->kind == QUEUE) {
                        if (doit) removeQueue(symbolTableNode);
                    }
                    else {
                        uint64_t index = expression(root);

                        if (symbolTableNode->kind == ARRAYLIST) {
                            if (doit) removeArrayList(symbolTableNode->arraylist, index);
                        }
                        else if (symbolTableNode->kind == LINKEDLIST) {
                            if (doit) removeLinkedList(symbolTableNode, index);
                        }
                    }
                }
                return 1;
            }

            // Check for equals after ID
            if (peek() != EQ) {
                printf("UNKNOWN COMMAND\n");
                error();
            }

            consume();

            if (peek() == ARRAY || peek() == LINKEDLIST || peek() == ARRAYLIST || peek() == QUEUE) {
		        enum Kind kind = peek();
                consume();

		        if (peek() == TYPE_INT) {
                    consume();
                    int numElements = tokenPtr->token->value;
                    struct Node* symbolTableNode = getNode(root, id);

                    switch (kind) {
                        case ARRAY: {
                            uint64_t* newArray = (uint64_t*) malloc(numElements * sizeof(uint64_t));
                            consume();
                            for (int i = 0; i < numElements; i++) {
                                newArray[i] = tokenPtr->token->value;
                                consume();
                                if (peek() == COMMA) consume();
                            }
                            if (symbolTableNode != NULL) {
                                if (symbolTableNode->kind != ARRAY || 
                                (symbolTableNode->kind == ARRAY && symbolTableNode->array == NULL)){
                                    printf("TYPE ERROR\n");
                                    error();
                                }
                            }
                     
                            
                            if (doit) setArray(root, id, newArray, numElements); 
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
                            tail->next = NULL;
                            if (symbolTableNode != NULL) {
                                if (symbolTableNode->kind != LINKEDLIST || 
                                (symbolTableNode->kind == LINKEDLIST && symbolTableNode->head->data == (uint64_t)NULL)) {
                                printf("TYPE ERROR\n");
                                error();
                            }
                            }

                            if (doit) setLinkedList(id, head, tail, numElements);
                            break;
                        }
                        case ARRAYLIST: {
                            ArrayList* newArrayList = new_ArrayList();
                            newArrayList->size = numElements;
                            
                            newArrayList->array = (uint64_t*) malloc(numElements * sizeof(uint64_t));
                            newArrayList->array_str = NULL;
                            newArrayList->kind = INT;
                            consume();

                            for (int i = 0; i < numElements; i++) {
                                newArrayList->array[i] = tokenPtr->token->value;
                                consume();
                                if (peek() == COMMA) consume();
                            }
                            if (symbolTableNode != NULL) {
                                if (symbolTableNode->kind != ARRAYLIST || 
                                (symbolTableNode->kind == ARRAYLIST && symbolTableNode->arraylist->kind != INT)) {
                                    printf("TYPE ERROR\n");
                                    error();
                                }
                            }

                            if (doit) setArrayList(root, id, newArrayList, numElements);
                            break;
                        }
			            case QUEUE: {
			                struct LinkedList* head = (struct LinkedList*)malloc(sizeof(struct LinkedList));
                            struct LinkedList* tail = (struct LinkedList*)malloc(sizeof(struct LinkedList));
                            tail->data = tokenPtr->token->value;
                            head = tail;
                            consume();
                            if (doit) setQueue(root, id, head, tail, 1);
                            break;	    
			            }
                   }
                }
                else if (peek() == TYPE_STRING) {
                    consume();
                    int numElements = tokenPtr->token->value;
                    struct Node* symbolTableNode = getNode(root, id);
                    switch (kind) {
                        case ARRAY: {
                            char** newArray = (char**) malloc(numElements * sizeof(char*));
                            consume();
                            for (int i = 0; i < numElements; i++) {
                                if (peek() != STRING) {
                                    printf("INPUT ELEMENTS ARE NOT STRINGS\n");
                                    error();
                                }
                                newArray[i] = tokenPtr->token->str;
                                consume();
                                if (peek() == COMMA) consume();
                            }
                            if (symbolTableNode != NULL) {
                                if (symbolTableNode->kind != ARRAY || 
                                (symbolTableNode->kind == ARRAY && symbolTableNode->array_str == NULL)) {
                                printf("TYPE ERROR\n");
                                error();
                            }
                            }
                            if (doit) setArray_str(root, id, newArray, numElements); 
                            break;
                        }
                        case ARRAYLIST: {
                            ArrayList* newArrayList = new_ArrayList();
                            newArrayList->size = numElements;
                            newArrayList->array_str = (char**)malloc(numElements*sizeof(char*));
                            consume();
                            newArrayList->kind = STRING;
                            for (int i = 0; i < numElements; i++) {
                                if (peek() != STRING) {
                                    printf("INPUT ELEMENTS ARE NOT STRINGS\n");
                                    error();
                                }
                                newArrayList->array_str[i] = tokenPtr->token->str;
                                consume();
                                if (peek() == COMMA) consume();
                            }
                            if (symbolTableNode != NULL) {
                                if (symbolTableNode->kind != ARRAYLIST || 
                                (symbolTableNode->kind == ARRAYLIST && symbolTableNode->arraylist->kind != STRING)) 
                            {
                                printf("TYPE ERROR\n");
                                error();
                            }
                            }
                            if (doit) setArrayList(root, id, newArrayList, numElements);
                            break;
                        }
                        case LINKEDLIST: {
                            struct LinkedList* head = (struct LinkedList*)malloc(sizeof(struct LinkedList));
                            struct LinkedList* tail = (struct LinkedList*)malloc(sizeof(struct LinkedList));
                            consume();
                            tail->str = tokenPtr->token->str;
                            head = tail;
                            consume();
                            for (int i = 1; i < numElements; i++) {
                                if (peek() == COMMA) consume();
                                struct LinkedList* newNode = (struct LinkedList*)malloc(sizeof(struct LinkedList));
                                newNode->str = tokenPtr->token->str;
                                consume();
                                tail->next = newNode;
                                tail = newNode;
                            }
                            tail->next = NULL;
                            if (symbolTableNode != NULL) {
                                if (symbolTableNode->kind != LINKEDLIST || 
                                (symbolTableNode->kind == LINKEDLIST && symbolTableNode->head->str == NULL)) {
                                    printf("TYPE ERROR\n");
                                    error();
                                }
                            }
                            if (doit) setLinkedList(id, head, tail, numElements);
                            break;
                        }
                    }
                }
            }
            // Normal expression assignment
            else {
                if (peek() == STRING) {
                    char* str = tokenPtr->token->str;
                    if (doit) set_str(root, id, str);
                    consume();
                }
                else if (peek() == OBJECT) {
                    if (doit) set_newObj(root, id, getId()); 
                    consume();
                }
                else {
                    uint64_t v = expression(root);
		            if (doit) set(root, id, v); 
		        }
            }
            return 1;
        }
        case CLASS: {
            consume();
            char* id = getId();
            consume(); // consume object id
            consume(); // consume =
            struct Node *objSymbolTableRoot = initializeObject(root, id);
            statement(objSymbolTableRoot, doit); 
            return 1;
        }
        case LBRACE: {
            consume();
            seq(root, doit);
            if (peek() != RBRACE) {
                printf("MISSING RIGHT BRACE\n");
                error();
            }
            consume();
            return 1;
        }
        case IF: {
            consume();
            uint64_t v = expression(root);
            if (v) {
                statement(root, doit);
                if (peek() == ELSE) {
                    consume();
                    statement(root, 0);
                }
            }
            else {
                // skip the next statement
                statement(root, 0);
                if (peek() == ELSE) {
                    consume();
                    statement(root, doit);
                }
            }
            return 1;
        }
        case WHILE: {
            consume();
            int expressionIndex = tokenPtr->token->index;
            uint64_t v = expression(root);
            while (v && doit) {
                statement(root, doit);
                // the statement was evaluated to true
                moveTokenPtrToIndex(expressionIndex);
                v = expression(root);
            }
            statement(root, 0);
            return 1;
        }
        case FUN: {
            char* funId = getFunId();
            consume();
            if (!doit) return 1;
            int returnIndex = tokenPtr->token->index;
            // go to the start of the function
            moveTokenPtrToIndex(get(root, funId));
            statement(root, doit);
            // move it back to where it was before
            moveTokenPtrToIndex(returnIndex);
            return 1;
        }
        case PRINT: {
            consume();
            if (doit) {
                // Print expression
                if (peek() != ID) printf("%"PRIu64"\n",expression(root));
                // Print ID value
                else {
                    char* id = getId(); 

                    consume();
                    struct Node* symbolTableNode = getNode(root, id);

                    if (symbolTableNode == NULL) {
                        // the id is not in the symbol table yet
                        printf("0\n");
                    }
                    // Print INT ID
                    else if (symbolTableNode->kind == INT) {
                        printf("%ld\n", get(root, id));
		            }
                    else if (symbolTableNode->kind == OBJECT) {
                        consume(); // consume the dot
                        if (peek() != ID) {
                            printf("INVALID FIELD\n");
                            error();
                        }
                        struct Node* objRoot = symbolTableNode->objSymbolTable;
                        struct Node* objectNode = getNode(objRoot, getId());
                        if (objectNode->kind == INT) printf("%ld\n", objectNode->data);
                        else if (objectNode->kind == STRING) printf("%s\n", objectNode->str);
                        consume(); // consume the id
                    }
		            else if (symbolTableNode->kind == STRING) {
                        printf("%s\n", symbolTableNode->str);
                    }
                    else if (symbolTableNode->kind == LINKEDLIST) {
                        struct LinkedList* current = getNode(root, id)->head;
                        printf("{");
                        while (current != NULL) {
                            if (current->str != NULL) printf("%s", current->str);
                            else printf("%ld", current->data);
                            current = current->next;
                            if (current != NULL) printf(" ");
                        }
                        printf("}\n");
                    }
		            else if (symbolTableNode->kind == QUEUE) {
		    	        struct LinkedList* current = getNode(root, id)->head;
                        printf("{");
                        while (current != NULL) {
                            printf("%ld", current->data);
                            current = current->next;
                            if (current != NULL) printf(" ");
                        }
                        printf("}\n");
		            }
                    // ID is an array/arraylist
                    else {
                        // it is an array
                        uint64_t* arrayPtr;
                        char **array_strPtr;
                        uint64_t sizeOfArray;
                        enum Kind type;

                        // ID is an array
                        if (symbolTableNode->kind == ARRAY) {
                            
                            arrayPtr = symbolTableNode->array; 
                            array_strPtr = symbolTableNode->array_str;
                            sizeOfArray = symbolTableNode->numElements;
                            type = (arrayPtr == NULL) ? STRING : INT;
                        }
                        // ID is an ArrayList
                        else if (symbolTableNode->kind == ARRAYLIST) {
                            ArrayList* list = symbolTableNode->arraylist;
                            type = list->kind;
                            arrayPtr = list->array;
                            array_strPtr = list->array_str;
                            sizeOfArray = list->size;
                        }

                        uint64_t loopAmount = (sizeOfArray == 0) ? 3 : sizeOfArray + (sizeOfArray - 1) + 3; // size, commas, brackets, end;
                        
                        int index = 0;
                        for (int i = 0; i < loopAmount; i++) {
                            if (i == 0) printf("{");
                            else if (i == loopAmount - 2) printf("}");
                            else if (i == loopAmount - 1) printf("\n");
                            else if (i % 2 == 0) printf(" ");
                            else {
                                if (type == INT) printf("%ld", arrayPtr[index]);
                                else if (type == STRING) printf("%s", (char *)array_strPtr[index]);
                              
                                index++;
                            }
                        }
                    }
                }
            }
            // Consume tokens if not being executed
            else {
                if (getNode(root, getId()) != NULL && (peek() == ID && getNode(root, getId())->kind == LINKEDLIST | getNode(root, getId())->kind == ARRAY | getNode(root, getId())->kind == ARRAYLIST | getNode(root, getId())->kind == QUEUE)) consume();
                else expression(root);
            }
            return 1;
        }
        default:
            return 0;
    }
}

void seq(struct Node* root, int doit) {
    while (statement(root, doit)) ;
}

void program(void) {
    seq(root, 1);
    if (peek() != END) {
        printf("UNDEFINED END OF PROGRAM\n");
        error();
    }
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

// use for debugging
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
        case TYPE_STRING: return "type_string";
        case STRING: return "string";
	    case QUEUE: return "queue";
	    case PEEK: return "peek";
	    case ADD: return "add";       
        case CLASS: return "class";
        case OBJECT: return "object";
        case DOT: return "dot";
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

