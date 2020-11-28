/* abstract-tree.h */

typedef enum {
  Program,
  VarDeclList,
  IntLiteral,
  CharLiteral,
  Identifier,
  Pointer,
  Function,
  EnTeteFonct,
  Void,
  Parameters,
  Corps,
  Instruction,
  Assign,
  Or,
  And,
  Equal,
  NotEqual,
  Greater,
  Less,
  GreaterEqual,
  LessEqual,
  Add,
  Sub,
  Mul,
  Div,
  Mod,
  Not,
  ReadE,
  ReadC,
  Print,
  If,
  Else,
  While,
  Return,
  Call,
  Deref,
  Evaluation
  /* and allother node labels */
  /* The list must coincide with the strings in abstract-tree.c */
  /* To avoid listing them twice, see https://stackoverflow.com/a/10966395 */
} Kind;

typedef struct Node {
  Kind kind;
  union {
    int integer;
    char character;
    char identifier[64];
  } u;
  int charno;
  int lineno;
  struct Node *firstChild, *nextSibling;
} Node;

Node *makeNode(Kind kind,int charno,int lineno);
Node * makeNodeIdentifier(char name[64],int charno,int lineno);
void addSibling(Node *node, Node *sibling);
void addChild(Node *parent, Node *child);
void deleteTree(Node*node);
void printTree(Node *node);

#define FIRSTCHILD(node) node->firstChild
#define SECONDCHILD(node) node->firstChild->nextSibling
#define THIRDCHILD(node) node->firstChild->nextSibling->nextSibling
