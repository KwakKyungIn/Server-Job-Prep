#pragma once
#include <mutex>

template<typename T>
class LockStack
{
public:
	LockStack() {} //기본 생성자

	LockStack(const LockStack&) = delete; //복사 생성자 삭제
	LockStack& operator=(const LockStack&) = delete; //복사 대입 연산자 삭제

	void Push(T value) 
	{
		std::lock_guard<std::mutex> lock(_mutex); // mutex 잠금
		_stack.push(std::move(value));
		_condition.notify_one(); // 스택에 요소가 추가되었음을 알림
	}

	bool TryPop(T& value) {
		std::lock_guard<std::mutex> lock(_mutex); // mutex 잠금
		if (_stack.empty()) {
			return false; // 스택이 비어있으면 false 반환
		}
		value = std::move(_stack.top()); // 스택의 top 요소를 value로 이동
		_stack.pop(); // top 요소 제거
		return true; // 성공적으로 pop했음을 나타냄
	}

	void WaitPop(T& value) {
		std::unique_lock<std::mutex> lock(_mutex); // mutex 잠금
		_condition.wait(lock, [this] { return _stack.empty()==false; }); // 스택이 비어있지 않을 때까지 대기
		value = std::move(_stack.top()); // 스택의 top 요소를 value로 이동
		_stack.pop(); // top 요소 제거
	}
private:
	stack<T> _stack;
	mutex _mutex;
	condition_variable _condition; // 조건 변수는 필요에 따라 추가할 수 있습니다.
};

template<typename T>
class LockFreeStack
{
	struct Node 
	{

		Node(const T& value) : data(value), next(nullptr) // 생성자에서 데이터와 next 포인터를 초기화합니다.
		{

		}

		T data;
		Node* next;
		
	};

public:
	void Push(const T& value) 
	{
		Node* node = new Node(value); // 새 노드 생성
		node->next = _head; // 새 노드의 next를 현재 헤드로 설정
		while (_head.compare_exchange_weak(node->next, node)==false)// 헤드를 새 노드로 교체
		{
		}
	}

	bool TryPop(T& value) 
	{
		++_popCount; // pop 횟수를 증가시킵니다.
		Node* oldHead = _head; // 현재 헤드 노드를 가져옵니다.
		

		while(oldHead && _head.compare_exchange_weak(oldHead, oldHead->next)==false) // 헤드를 다음 노드로 교체
		{
		}
		if(oldHead == nullptr) // 만약 헤드가 nullptr이면 스택이 비어있음을 나타냅니다.
		{
			--_popCount; // 실패 시 pop 횟수를 감소시킵니다.
			return false; // pop 실패

		}
		value = oldHead->data; // 값을 반환
		tryDelete(oldHead); // 메모리 해제
		return true; // 성공적으로 pop했음을 나타냄
	}

	void tryDelete(Node* oldHaed)
	{
		if(_popCount == 1) // pop 횟수가 1이면
		{
			Node* node = _pendingList.exchange(nullptr); // 대기 중인 리스트의 첫 번째 노드를 가져옵니다.

			if (--_popCount == 0)
			{
				DeleteNodes(node);
			}
			else if(node)
			{
				ChainPendingNodeList(node); // 대기 중인 노드를 연결합니다.
			}
			delete oldHaed; // 노드를 삭제합니다.
		}
		else // pop 횟수가 0이 아니면
		{
			ChainPendingNode(oldHaed); // 대기 중인 노드로 연결합니다.
			--_popCount; // pop 횟수를 감소시킵니다.
		}
	}

	void ChainPendingNodeList(Node* first, Node* last)
	{
		last->next = _pendingList; // 마지막 노드의 next를 대기 중인 리스트로 설정
		while (_pendingList.compare_exchange_weak(last->next, first) == false)		// 대기 중인 리스트의 첫 번째 노드를 새로 설정
		{
		}

	}
	void ChainPendingNodeList(Node* node) 
	{
		Node* last = node;
		while (last->next) // 마지막 노드를 찾습니다.
		{
			last = last->next;
		}
		ChainPendingNodeList(node, last); // 대기 중인 리스트에 연결합니다.
	}

	void ChainPendingNode(Node* node)
	{
		ChainPendingNodeList(node, node); // 단일 노드를 대기 중인 리스트에 연결합니다.

	}

	static void DeleteNodes(Node* node) 
	{
		while (node) // 노드가 존재하는 동안
		{
			Node* next = node->next; // 다음 노드를 저장
			delete node; // 현재 노드 삭제
			node = next; // 다음 노드로 이동
		}
	}
private:
	atomic<Node*> _head; // 스택의 헤드를 원자적으로 관리합니다.
	atomic<uint32> _popCount = 0; // pop 횟수를 원자적으로 관리합니다.
	atomic<Node*> _pendingList; // 대기 중인 노드 리스트를 원자적으로 관리합니다.
};