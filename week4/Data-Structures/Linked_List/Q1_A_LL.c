//////////////////////////////////////////////////////////////////////////////////

/* CE1007/CZ1007 Data Structures
Lab Test: Section A - Linked List Questions
Purpose: Implementing the required functions for Question 1 */

//////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>

//////////////////////////////////////////////////////////////////////////////////

typedef struct _listnode{ // struct는 여러 변수를 묶어서 새로운 자료형처럼 사용하는 문법
	int item;
	struct _listnode *next;
} ListNode;			// You should not change the definition of ListNode

typedef struct _linkedlist{
	int size;
	ListNode *head;
} LinkedList;			// You should not change the definition of LinkedList


///////////////////////// function prototypes ////////////////////////////////////

//You should not change the prototype of this function
int insertSortedLL(LinkedList *ll, int item);

void printList(LinkedList *ll);
void removeAllItems(LinkedList *ll);
ListNode *findNode(LinkedList *ll, int index);
int insertNode(LinkedList *ll, int index, int value);
int removeNode(LinkedList *ll, int index);


//////////////////////////// main() //////////////////////////////////////////////

int main()
{
	LinkedList ll;
	int c, i, j;
	c = 1;

	//Initialize the linked list 1 as an empty linked list
	ll.head = NULL;
	ll.size = 0;

	printf("1: Insert an integer to the sorted linked list:\n");
	printf("2: Print the index of the most recent input value:\n");
	printf("3: Print sorted linked list:\n");
	printf("0: Quit:");

	while (c != 0)
	{
		printf("\nPlease input your choice(1/2/3/0): ");
		scanf("%d", &c);

		switch (c)
		{
		case 1:
			printf("Input an integer that you want to add to the linked list: ");
			scanf("%d", &i);
			j = insertSortedLL(&ll, i);
			printf("The resulting linked list is: ");
			printList(&ll);
			break;
		case 2:
			printf("The value %d was added at index %d\n", i, j);
			break;
		case 3:
			printf("The resulting sorted linked list is: ");
			printList(&ll);
			removeAllItems(&ll);
			break;
		case 0:
			removeAllItems(&ll);
			break;
		default:
			printf("Choice unknown;\n");
			break;
		}


	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////

int insertSortedLL(LinkedList *ll, int item)
{
    // ll이 구조체 자체가 아닌 구조체를 가리키는 포인터 LinkedList * 이기 때문에 .을 쓰는 것이 불가능함
    // ll이 주소일 경우 포인터로 멤버에 접근 할 시에는 ->이거를 써야함
	if (ll -> head == NULL){
		ListNode *newNode = malloc(sizeof(ListNode)); // ll->head가 노드 자체가 아닌 노드를 가리키는 포인터 ListNode*
		// ListNode 타입의 newNode의 주소는 ListNode의 사이즈 크기만큼 malloc으로 할당을 받음
		newNode->item = item; // 포인터 -> 변수 접근 값 item으로 함
		newNode->next = NULL; // 마찬가지로 포인터 next의 값은 Null
		ll->head = newNode; // ll 링크드리스트 구조체의 내부 head는 newNode로 가리키게
		ll->size ++;
	}else{
		ListNode *newNode = malloc(sizeof(ListNode));
		newNode->item = item;
		newNode->next = NULL;
		ListNode *curNode = ll->head;
		ListNode *prevNode = curNode;
		int count = 0;
		while(curNode!=NULL){
			if (newNode->item < ll->head->item){
				newNode->next = ll->head;
				ll->head = newNode;
				ll->size ++;
				return count;
			}
			if (curNode->item < newNode->item){
				prevNode = curNode;
				curNode = curNode -> next;
				count ++;
			}else if(curNode->item == newNode->item) {
				free(newNode);
				return -1;
			}else{
				newNode->next = prevNode->next;
				prevNode->next = newNode;
				ll->size ++;
				return count;
			}
			if (prevNode -> next == NULL){
				prevNode->next = newNode;
				ll->size++;
				return count;
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////

void printList(LinkedList *ll){

	ListNode *cur;
	if (ll == NULL)
		return;
	cur = ll->head;

	if (cur == NULL)
		printf("Empty");
	while (cur != NULL)
	{
		printf("%d ", cur->item);
		cur = cur->next;
	}
	printf("\n");
}


void removeAllItems(LinkedList *ll)
{
	ListNode *cur = ll->head;
	ListNode *tmp;

	while (cur != NULL){
		tmp = cur->next;
		free(cur);
		cur = tmp;
	}
	ll->head = NULL;
	ll->size = 0;
}


ListNode *findNode(LinkedList *ll, int index){

	ListNode *temp;

	if (ll == NULL || index < 0 || index >= ll->size)
		return NULL;

	temp = ll->head;

	if (temp == NULL || index < 0)
		return NULL;

	while (index > 0){
		temp = temp->next;
		if (temp == NULL)
			return NULL;
		index--;
	}

	return temp;
}

int insertNode(LinkedList *ll, int index, int value){

	ListNode *pre, *cur;

	if (ll == NULL || index < 0 || index > ll->size + 1)
		return -1;

	// If empty list or inserting first node, need to update head pointer
	if (ll->head == NULL || index == 0){
		cur = ll->head;
		ll->head = malloc(sizeof(ListNode));
		ll->head->item = value;
		ll->head->next = cur;
		ll->size++;
		return 0;
	}


	// Find the nodes before and at the target position
	// Create a new node and reconnect the links
	if ((pre = findNode(ll, index - 1)) != NULL){
		cur = pre->next;
		pre->next = malloc(sizeof(ListNode));
		pre->next->item = value;
		pre->next->next = cur;
		ll->size++;
		return 0;
	}

	return -1;
}


int removeNode(LinkedList *ll, int index){

	ListNode *pre, *cur;

	// Highest index we can remove is size-1
	if (ll == NULL || index < 0 || index >= ll->size)
		return -1;

	// If removing first node, need to update head pointer
	if (index == 0){
		cur = ll->head->next;
		free(ll->head);
		ll->head = cur;
		ll->size--;

		return 0;
	}

	// Find the nodes before and after the target position
	// Free the target node and reconnect the links
	if ((pre = findNode(ll, index - 1)) != NULL){

		if (pre->next == NULL)
			return -1;

		cur = pre->next;
		pre->next = cur->next;
		free(cur);
		ll->size--;
		return 0;
	}

	return -1;
}
