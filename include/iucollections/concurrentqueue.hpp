
//***********************************************************//
//                                                           //
// iucollections                                             //
// C++11 class-template library of thread-safe collections   //
//                                                           //
// Repository:  https://github.com/islandu/iucollections     //
// Author:      Daniel Thompson, Ph.D (2022)                 //
// File:        include/iucollections/concurrentqueue.hpp    //
// Description: ConcurrentQueue<T> class-template definition //
//                                                           //
//***********************************************************//

#ifndef IUCOLLECTIONS_CONCURRENTQUEUE_HPP
#define IUCOLLECTIONS_CONCURRENTQUEUE_HPP

#include <cstddef>
#include <mutex>
#include <stdexcept>

namespace iucollections {

template<typename T>
class ConcurrentQueue final {

public:

    // Constructors (basic, copy, move, no default)

    ConcurrentQueue(const std::size_t capacity);
    ConcurrentQueue(const ConcurrentQueue & queue);
    ConcurrentQueue(ConcurrentQueue && queue);
    ConcurrentQueue() = delete;

    // Destructor

    ~ConcurrentQueue();

    // Operator overloads (copy/move assignment)

    ConcurrentQueue & 
    operator=(const ConcurrentQueue & queue);

    ConcurrentQueue & 
    operator=(ConcurrentQueue && queue);

    // Observers/getters

    std::size_t
    size() const;

    std::size_t
    capacity() const;

    bool 
    empty() const;

    bool 
    full() const;

    // Queue operations

    const T &
    peek_front() const;

    const T &
    peek_back() const;

    void 
    push(const T & item);

    void
    push(T && item);

    T &
    pop();

private:

    // Data members

    mutable std::mutex mutex;
    T * items;
    std::size_t capacity_, size_, front, back;

    // Helper functions

    inline void
    safe_increment(std::size_t & value)
    {
        if (value == SIZE_MAX)
            value = 0;
        
        value = (value + 1) % this->capacity_;
    }
};

// Constructors

template <typename T>
ConcurrentQueue<T>::ConcurrentQueue(const std::size_t capacity)
    : capacity_(capacity), size_(0), front(0)
{
    if (!capacity)
    {
        throw std::underflow_error(
            "iucollections::ConcurrentQueue<T> instance cannot have a capacity of 0 items!"
        );
    }

    this->back = capacity - 1;
    this->items = new T[capacity];
}

template <typename T>
ConcurrentQueue<T>::ConcurrentQueue(const ConcurrentQueue & queue)
{
    std::lock_guard<std::mutex> lock_guard(queue.mutex);

    this->items = new T[queue.capacity_];
    this->capacity_ = queue.capacity_;
    this->size_ = queue.size_;
    this->front = queue.front;
    this->back = queue.back;

    for (std::size_t i = 0; i < queue.capacity_; ++i)
    {
        this->items[i] = queue.items[i];
    }
}

template <typename T>
ConcurrentQueue<T>::ConcurrentQueue(ConcurrentQueue && queue)
{
    std::lock_guard<std::mutex> lock_guard(queue.mutex);

    this->items = new T[queue.capacity_];
    this->capacity_ = queue.capacity_;
    this->size_ = queue.size_;
    this->front = queue.front;
    this->back = queue.back;

    for (std::size_t i = 0; i < queue.capacity_; ++i)
    {
        this->items[i] = std::move(queue.items[i]);
    }

    delete [] queue.items;
}

// Destructor

template <typename T>
ConcurrentQueue<T>::~ConcurrentQueue()
{
    std::lock_guard<std::mutex> lock_guard(this->mutex);
    delete [] this->items;
}

// Operator overloads

template <typename T>
ConcurrentQueue<T> & 
ConcurrentQueue<T>::operator=(const ConcurrentQueue & queue)
{
    std::lock_guard<std::mutex> lock_guard1(this->mutex), lock_guard2(queue.mutex);

    if (this == &queue)
        return *this;
    
    delete [] this->items;
    this->items = new T[queue.capacity_];
    this->capacity_ = queue.capacity_;
    this->size_ = queue.size_;
    this->front = queue.front;
    this->back = queue.back;

    for (std::size_t i = 0; i < queue.capacity_; ++i)
    {
        this->items[i] = queue.items[i];
    }

    return *this;
}

template <typename T>
ConcurrentQueue<T> & 
ConcurrentQueue<T>::operator=(ConcurrentQueue && queue)
{
    std::lock_guard<std::mutex> lock_guard1(this->mutex), lock_guard2(queue.mutex);

    if (this == &queue)
        return *this;
    
    delete [] this->items;
    this->items = queue.items;
    this->capacity_ = queue.capacity_;
    this->size_ = queue.size_;
    this->front = queue.front;
    this->back = queue.back;

    return *this;
}

// Observers/getters

template <typename T>
std::size_t
ConcurrentQueue<T>::size() const
{
    return this->size_;
}

template <typename T>
std::size_t
ConcurrentQueue<T>::capacity() const
{
    return this->capacity_;
}

template <typename T>
bool 
ConcurrentQueue<T>::empty() const
{
    std::lock_guard<std::mutex> lock_guard(this->mutex);
    return this->size_ == 0;
}

template <typename T>
bool 
ConcurrentQueue<T>::full() const
{
    std::lock_guard<std::mutex> lock_guard(this->mutex);
    return this->size_ == this->capacity_;
}

// Queue operations

template <typename T>
const T &
ConcurrentQueue<T>::peek_front() const
{
    std::lock_guard<std::mutex> lock_guard(this->mutex);
    return this->items[front];
}

template <typename T>
const T &
ConcurrentQueue<T>::peek_back() const
{
    std::lock_guard<std::mutex> lock_guard(this->mutex);
    return this->items[back];
}

template <typename T>
void 
ConcurrentQueue<T>::push(const T & item)
{
    std::lock_guard<std::mutex> lock_guard(this->mutex);

    if (this->size_ == this->capacity_)
    {
        throw std::overflow_error(
            "iucollections::ConcurrentQueue<T> instance is full!"
        );
    }

    safe_increment(this->back);
    this->items[this->back] = item;
    ++(this->size_);
}

template <typename T>
void
ConcurrentQueue<T>::push(T && item)
{
    std::lock_guard<std::mutex> lock_guard(this->mutex);

    if (this->size_ == this->capacity_)
    {
        throw std::overflow_error(
            "iucollections::ConcurrentQueue<T> instance is full!"
        );
    }

    safe_increment(this->back);
    this->items[this->back] = std::move(item);
    ++(this->size_);
}

template <typename T>
T &
ConcurrentQueue<T>::pop()
{
    std::lock_guard<std::mutex> lock_guard(this->mutex);

    if (!(this->size_))
    {
        throw std::underflow_error(
            "iucollections::ConcurrentQueue<T> instance is empty!"
        );
    }

    T & result = this->items[this->front];
    safe_increment(this->front);
    --(this->size_);

    return result;
}

} // namespace iucollections

#endif // IUCOLLECTIONS_CONCURRENTQUEUE_HPP
