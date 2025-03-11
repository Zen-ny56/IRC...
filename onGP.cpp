#include <iostream>
#include <stack>
#include <queue>

void reverseRotate(std::stack<std::string>& s) {
    if (s.empty() || s.size() == 1)
        return; // Nothing to rotate if stack has 0 or 1 element

    std::queue<std::string> tempQueue;

    // Step 1: Move all elements except the last one to a queue
    while (s.size() > 1) {
        tempQueue.push(s.top());
        s.pop();
    }

    // Step 2: The last remaining element is the bottom-most element
    std::string bottomElement = s.top();
    s.pop();

    // Step 3: Restore the elements back to the stack in original order
    while (!tempQueue.empty()) {
        s.push(tempQueue.front());
        tempQueue.pop();
    }

    // Step 4: Push the bottom-most element to the top
    s.push(bottomElement);
}

// **Test Function**
int main() {
    std::stack<std::string> myStack;
    myStack.push("A");
    myStack.push("B");
    myStack.push("C");
    myStack.push("D"); // Top of stack

    std::cout << "Before reverseRotate:\n";
    std::stack<std::string> temp = myStack;
    while (!temp.empty()) {
        std::cout << temp.top() << " ";
        temp.pop();
    }
    std::cout << "\n";

    reverseRotate(myStack);

    std::cout << "After reverseRotate:\n";
    temp = myStack;
    while (!temp.empty()) {
        std::cout << temp.top() << " ";
        temp.pop();
    }
    std::cout << "\n";

    return 0;
}