
#ifndef EPI_STACKED_LINKED_LIST_H
#define EPI_STACKED_LINKED_LIST_H
namespace epi {


template <class T>
class StackLinkedList {
public:
    struct Node {
        T data;
        Node* next;
    };
    
    Node* head;
public:
    StackLinkedList() = default;
    StackLinkedList(StackLinkedList &stackLinkedList) = delete;
    void push(Node * newNode) {
        newNode->next = head;
        head = newNode;
    }

    Node* pop() {
        Node * top = head;
        head = head->next;
        return top;
    }
};

}
#endif
