#pragma once
#include <memory_resource>
#include <iterator>
#include <stdexcept>

template<typename T>
class DynamicArray {
private:
    T* data_;
    std::size_t size_;
    std::size_t capacity_;
    std::pmr::polymorphic_allocator<T> allocator_;

    void resize_if_needed() {
        if (size_ >= capacity_) {
            std::size_t new_capacity = capacity_ * 2;
            T* new_data = allocator_.allocate(new_capacity);
            
            for (std::size_t i = 0; i < size_; ++i) {
                std::allocator_traits<std::pmr::polymorphic_allocator<T>>::construct(allocator_, new_data + i, std::move(data_[i]));
                std::allocator_traits<std::pmr::polymorphic_allocator<T>>::destroy(allocator_, data_ + i);
            }
            
            if (data_) {
                allocator_.deallocate(data_, capacity_);
            }
            
            data_ = new_data;
            capacity_ = new_capacity;
        }
    }

public:
    class Iterator {
    private:
        T* ptr_;
        
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;
        
        Iterator(T* ptr) : ptr_(ptr) {}
        
        reference operator*() const { return *ptr_; }
        pointer operator->() const { return ptr_; }
        
        Iterator& operator++() {
            ++ptr_;
            return *this;
        }
        
        Iterator operator++(int) {
            Iterator tmp = *this;
            ++ptr_;
            return tmp;
        }
        
        bool operator==(const Iterator& other) const { return ptr_ == other.ptr_; }
        bool operator!=(const Iterator& other) const { return ptr_ != other.ptr_; }
    };

    explicit DynamicArray(std::pmr::memory_resource* resource = std::pmr::get_default_resource())
        : data_(nullptr), size_(0), capacity_(0), allocator_(resource) {
        capacity_ = 10;
        data_ = allocator_.allocate(capacity_);
    }
    
    ~DynamicArray() {
        clear();
        if (data_) {
            allocator_.deallocate(data_, capacity_);
        }
    }
    
    void push_back(const T& value) {
        resize_if_needed();
        std::allocator_traits<std::pmr::polymorphic_allocator<T>>::construct(allocator_, data_ + size_, value);
        ++size_;
    }
    
    void push_back(T&& value) {
        resize_if_needed();
        std::allocator_traits<std::pmr::polymorphic_allocator<T>>::construct(allocator_, data_ + size_, std::move(value));
        ++size_;
    }
    
    T& operator[](std::size_t index) {
        if (index >= size_) throw std::out_of_range("Index out of range");
        return data_[index];
    }
    
    const T& operator[](std::size_t index) const {
        if (index >= size_) throw std::out_of_range("Index out of range");
        return data_[index];
    }
    
    void pop_back() {
        if (size_ > 0) {
            --size_;
            std::allocator_traits<std::pmr::polymorphic_allocator<T>>::destroy(allocator_, data_ + size_);
        }
    }
    
    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    
    void clear() {
        for (std::size_t i = 0; i < size_; ++i) {
            std::allocator_traits<std::pmr::polymorphic_allocator<T>>::destroy(allocator_, data_ + i);
        }
        size_ = 0;
    }
    
    Iterator begin() { return Iterator(data_); }
    Iterator end() { return Iterator(data_ + size_); }
};