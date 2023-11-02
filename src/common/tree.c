#include "headers.h"

const int MaxBufferLength = 50000;

/*
      *
     /|\
    / | \
   A  B  C(Can be stored in any server)

*--->A->B->C->NULL
     |  |  |
     |  |  |
     v  v  v
    A/D
*/

const int MaxNameLength = 32;

struct Information
{
  char DirectoryName[32];
  u8 NumChild;
  /*
  Pointer to files in directory.
  */
};

struct TreeNode
{
  struct Information NodeInfo;
  struct TreeNode *NextSibling;
  struct TreeNode *PrevSibling;
  struct TreeNode *ChildDirectoryLL; // LL - > Linked List
  struct TreeNode *Parent;
};

typedef struct TreeNode *Tree;

struct TreeNode *InitNode(const char *Name, struct TreeNode *Parent)
{
  struct TreeNode *Node = malloc(sizeof(struct TreeNode));
  strcpy(Node->NodeInfo.DirectoryName, Name);
  Node->NodeInfo.NumChild = 0;
  Node->Parent = Parent;
  Node->ChildDirectoryLL = NULL;
  Node->NextSibling = NULL;
  Node->PrevSibling = NULL;
  if (Parent != NULL)
  {
    Parent->NodeInfo.NumChild++;
  }
  return Node;
}

Tree InitTree()
{
  return InitNode(".", NULL);
}

/*
Finds a child node for a given TreeNode T.

If CreateFlag is enabled then a child node with the given name will be created if
child doesn't exist. The function will return this node.

If NoNameFlag is enabled then a child node will be created with no name.
The function will return this node.
*/
struct TreeNode *FindChild(Tree T, const char *ChildName, int CreateFlag, int NoNameFlag)
{
  if (T->ChildDirectoryLL == NULL)
  {
    if (CreateFlag)
    {
      T->ChildDirectoryLL = InitNode(ChildName, T);
      return T->ChildDirectoryLL;
    }
    return NULL;
  }

  struct TreeNode *trav = T->ChildDirectoryLL;

  while (trav->NextSibling != NULL)
  {
    if (!NoNameFlag && strcmp(trav->NodeInfo.DirectoryName, ChildName) == 0)
    {
      return trav;
    }
    trav = trav->NextSibling;
  }

  if (!NoNameFlag && strcmp(trav->NodeInfo.DirectoryName, ChildName) == 0)
  {
    return trav;
  }

  if (CreateFlag)
  {
    trav->NextSibling = InitNode(ChildName, T);
    trav->NextSibling->PrevSibling = trav;
    return trav->NextSibling;
  }
  return NULL;
}

Tree FindNode(Tree T)
{
}

Tree InsertNode(Tree T, struct TreeNode *Target)
{
}

#define DIRINFO 'D'
#define DIREND '.'

int SendTreeDataDriver(Tree T, char buffer[MaxBufferLength], int *lastindex, int BufferCapacity)
{
  if (*lastindex >= BufferCapacity)
    return -1;

  buffer[*lastindex] = DIRINFO;
  *lastindex += 1;

  if (*lastindex + sizeof(T->NodeInfo) >= BufferCapacity)
    return -1;

  memcpy(&buffer[*lastindex], &T->NodeInfo, sizeof(T->NodeInfo));
  *lastindex += sizeof(T->NodeInfo);

  struct TreeNode *trav = T->ChildDirectoryLL;

  while (trav != NULL)
  {
    if (SendTreeDataDriver(trav, buffer, lastindex, BufferCapacity) == -1)
      return -1;
    trav = trav->NextSibling;
  }

  if (*lastindex >= BufferCapacity)
    return -1;

  buffer[*lastindex] = DIREND;
  *lastindex += 1;

  return 0;
}

int SendTreeData(Tree T, char buffer[MaxBufferLength])
{
  int lastindex = 0;
  if (SendTreeDataDriver(T, buffer, &lastindex, MaxBufferLength) == -1)
    return -1;
  return 0;
}

int ReceiveTreeDataDriver(Tree T, char buffer[MaxBufferLength], int *lastindex, int BufferCapacity)
{
  if (*lastindex + sizeof(T->NodeInfo) >= BufferCapacity)
    return -1;
  memcpy(&T->NodeInfo, &buffer[*lastindex], sizeof(T->NodeInfo));
  *lastindex += sizeof(T->NodeInfo);

  T->NodeInfo.NumChild = 0;
  while (buffer[*lastindex] == DIRINFO)
  {
    *lastindex += 1;
    if (*lastindex >= BufferCapacity)
      return -1;

    struct TreeNode *Node = FindChild(T, "", 1, 1);

    if (ReceiveTreeDataDriver(Node, buffer, lastindex, BufferCapacity) == -1)
      return -1;
  }
  *lastindex += 1;
  return 0;
}

Tree ReceiveTreeData(char buffer[MaxBufferLength])
{
  Tree T = InitTree();
  int lastindex = 1;
  if (ReceiveTreeDataDriver(T, buffer, &lastindex, MaxBufferLength) == -1)
    return NULL;
  return T;
}

void PrintTree(Tree T, int indent)
{
  for (int i = 0; i < indent; i++)
  {
    printf("\t");
  }
  printf("%s\n", T->NodeInfo.DirectoryName);
  struct TreeNode *trav = T->ChildDirectoryLL;
  while (trav != NULL)
  {
    PrintTree(trav, indent + 1);
    trav = trav->NextSibling;
  }
}

int DeleteTree(Tree T)
{
  struct TreeNode *trav = T->ChildDirectoryLL;
  struct TreeNode *next;

  while (trav != NULL)
  {
    next = trav->NextSibling;
    DeleteTree(trav);
    trav = next;
  }

  if (T->Parent != NULL)
  {
    T->Parent->NodeInfo.NumChild--;
    if (T->Parent->ChildDirectoryLL == T)
    {
      T->Parent->ChildDirectoryLL = T->NextSibling;
    }
  }

  if (T->NextSibling != NULL)
    T->NextSibling->PrevSibling = T->PrevSibling;

  if (T->PrevSibling != NULL)
    T->PrevSibling->NextSibling = T->NextSibling;

  free(T);
  return 0;
}

struct TreeNode *ProcessDirPath(char *DirPath, Tree T, int CreateFlag)
{
  struct TreeNode *Cur = T;
  if (T == NULL)
    return NULL;
  char DirPathCopy[128];
  strcpy(DirPathCopy, DirPath);
  char *Delim = "/\\";
  char *token = strtok(DirPathCopy, Delim);
  while (token != NULL)
  {
    Cur = FindChild(Cur, token, CreateFlag, 0);
    if (Cur == NULL)
      return NULL;
    token = strtok(NULL, Delim);
  }
  return Cur;
}

void RandomTest()
{
  Tree T = InitTree();
  Tree T2 = FindChild(T, "Hello", 1, 0);
  Tree T3 = FindChild(T2, "Fuck YOU", 1, 0);
  Tree T4 = FindChild(T, "LOL", 1, 0);
  Tree T5 = FindChild(T3, "NOPLEASE", 1, 0);
  Tree T6 = FindChild(T4, "OAWKFEOWAJFAW", 1, 0);
  Tree T7 = ProcessDirPath("Hello/My/Name/Is/Harshvardhan", T, 1);
  DeleteTree(ProcessDirPath("Hello/My/Name", T, 0));
  char Buffer[MaxBufferLength];
  SendTreeData(T, Buffer);
  Tree TRec = ReceiveTreeData(Buffer);
  printf("OG:\n");
  PrintTree(T, 0);
  printf("Test:\n");
  PrintTree(TRec, 0);
  if ((T7 = ProcessDirPath("Hello/My/Name//Is/Harshvardhan", TRec, 0)) != NULL)
  {
    printf("Passed: %s!\n", T7->NodeInfo.DirectoryName);
  }
  else
  {
    printf("Failed\n");
  }
}

int main()
{
  RandomTest();
}