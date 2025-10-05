#pragma once

#include <optional>


template <typename T> // T는 shared_ptr<Job> 같은 타입이 됩니다.
class FineGrainedConcurrentQueue
{
private:
    // 노드 정의: T(shared_ptr) 데이터를 담고, 다음 노드를 unique_ptr로 연결합니다.
    struct Node
    {
        T data; // T가 shared_ptr<Job>이므로, 이 노드는 shared_ptr<Job>을 직접 소유합니다.
        std::unique_ptr<Node> next;

        // 더미 노드용 생성자
        Node() = default;
        // 데이터 노드용 생성자
        Node(T value) : data(std::move(value)) {}
    };

    // Head는 unique_ptr로 관리하여 메모리 자동 해제를 보장합니다.
    std::unique_ptr<Node> head;
    // Tail은 Raw Pointer로 관리하여 삽입 시 Head Lock 없이 빠르게 접근합니다.
    Node* tail;

    // 세밀한 동기화를 위한 두 개의 뮤텍스
    std::mutex head_mutex; // Pop (Head) 작업을 보호
    std::mutex tail_mutex; // Push (Tail) 작업을 보호

    // Head 포인터를 이동하고 이전 노드를 해제합니다. (Pop 과정의 내부 함수)
    std::unique_ptr<Node> pop_head()
    {
        std::unique_ptr<Node> old_head = std::move(head);
        head = std::move(old_head->next);
        return old_head;
    }

    // 큐가 비어있는지 확인합니다. (Pop Lock이 필요)
    bool is_empty()
    {
        // Head와 Tail이 같은 노드(더미 노드)를 가리키면 비어있습니다.
        return head.get() == tail;
    }

public:
    FineGrainedConcurrentQueue() : head(std::make_unique<Node>()), tail(head.get())
    {
        // 생성 시 더미 노드를 생성하여 Head와 Tail이 동일하게 가리키도록 합니다.
    }

    // 복사 및 이동 금지 (Thread-Safe 컨테이너의 일관성을 위해)
    FineGrainedConcurrentQueue(const FineGrainedConcurrentQueue&) = delete;
    FineGrainedConcurrentQueue& operator=(const FineGrainedConcurrentQueue&) = delete;

    // 1. 데이터 삽입 (Push - Tail Lock 사용)
    void Push(T new_value)
    {
        // T가 이미 shared_ptr<Job>이므로, new_value를 새 데이터로 사용합니다.
        T new_data = std::move(new_value);

        // 새로운 Node 객체 생성 (unique_ptr로 관리)
        // 이 new_node는 shared_ptr<Job>의 소유권을 new_data로 받습니다.
        std::unique_ptr<Node> new_node = std::make_unique<Node>(std::move(new_data));

        // Tail Lock 획득 (Head Lock 없이 삽입 가능)
        std::lock_guard<std::mutex> tail_lock(tail_mutex);

        // 기존 Tail의 다음 노드를 새 노드로 연결
        tail->next = std::move(new_node);

        // Tail 포인터를 새 노드를 가리키도록 업데이트
        tail = tail->next.get();
    }

    // 2. 데이터 추출 (Non-Blocking Try-Pop - Head Lock 사용)
    // std::optional<T>를 반환합니다. T는 shared_ptr<Job> 타입입니다.
    std::optional<T> try_pop()
    {
        // Head Lock 획득 (Tail Lock 없이 추출 가능)
        std::lock_guard<std::mutex> head_lock(head_mutex);

        // 큐가 비어있는지 확인 (더미 노드만 남아있는 경우)
        if (is_empty())
        {
            return std::nullopt;
        }

        // Head 포인터 이동 및 이전 노드 해제
        std::unique_ptr<Node> old_head_node = pop_head();

        // 이전 노드의 T (shared_ptr<Job>) 데이터를 이동하여 추출
        // old_head_node는 소멸되면서 메모리 해제, Node::data (shared_ptr<Job>)는 소유권을 이동합니다.
        return std::move(old_head_node->data);
    }
    
    // 3. 모든 데이터 추출 (PopAll - Head Lock 사용)
    std::vector<T> PopAll()
    {
        // Head Lock만 획득합니다.
        // Tail은 항상 새로운 노드에 연결되므로, Pop 작업은 Tail에 간섭하지 않습니다.
        std::lock_guard<std::mutex> head_lock(head_mutex);

        std::vector<T> result_list;

        // 현재 Head 노드 (더미 노드)부터 Tail이 가리키는 노드까지 순회하며 데이터를 추출합니다.
        // Tail은 항상 마지막 데이터 노드의 이전 더미 노드에 연결되어 있습니다.
        // 큐가 비어있으면 head.get() == tail 이므로 루프는 돌지 않습니다.
        while (head.get() != tail)
        {
            // pop_head()는 head를 다음 노드로 이동하고, 이전 노드의 unique_ptr을 반환합니다.
            std::unique_ptr<Node> old_head_node = pop_head();

            // old_head_node의 data를 결과 리스트에 이동합니다. (T가 shared_ptr<Job>이므로 소유권 이동)
            result_list.push_back(std::move(old_head_node->data));

            // 주의: pop_head()가 실행되면 head는 다음 노드를 가리키게 됩니다.
            // 큐에 요소가 N개 있었다면, N번의 pop_head() 후에 head는 tail을 가리키게 됩니다.
        }

        // 루프가 끝난 후, Head는 Tail(마지막 삽입 시점의 Tail)을 가리키는 상태가 됩니다.
        // 즉, 큐는 '비어있는 상태'로 돌아갑니다 (head와 tail이 더미 노드를 가리킴).

        return result_list;
    }
};
