#pragma once
#include <cstdlib>
#include <memory_resource>
#include <vector>
#include <cstddef>

class CustomMemoryResource : public std::pmr::memory_resource {
private:
    struct Block {
        void* ptr;
        size_t size;
        
        Block(void* p, size_t s) : ptr(p), size(s) {}
    };
    
    std::vector<Block> allocated_blocks;
    std::vector<Block> free_blocks;

protected:
    void* do_allocate(size_t bytes, size_t alignment) override {
        // Проводим проверку блоков на вместимость в них bytes
        for (int i = static_cast<int>(free_blocks.size()) - 1; i >= 0; --i) {
            if (free_blocks[i].size >= bytes) {
                void* ptr = free_blocks[i].ptr;
                allocated_blocks.emplace_back(ptr, free_blocks[i].size);
                free_blocks.erase(free_blocks.begin() + i);
                return ptr;
            }
        }
        
        // Выделение нового блока
        void* ptr = std::aligned_alloc(alignment, bytes);
        if (!ptr) {
            throw std::bad_alloc();
        }
        allocated_blocks.emplace_back(ptr, bytes);
        return ptr;
    }
    
    void do_deallocate(void* ptr, size_t /*bytes*/, size_t /*alignment*/) override {
        // Ищем блок в выделенных блоках
        for (auto it = allocated_blocks.begin(); it != allocated_blocks.end(); ++it) {
            if (it->ptr == ptr) {
                free_blocks.emplace_back(ptr, it->size);
                allocated_blocks.erase(it);
                return;
            }
        }
    }
    
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }

public:
    ~CustomMemoryResource() {
        // Освобождаем всю неосвобожденную память
        for (const auto& block : allocated_blocks) {
            std::free(block.ptr);
        }
        for (const auto& block : free_blocks) {
            std::free(block.ptr);
        }
    }
};