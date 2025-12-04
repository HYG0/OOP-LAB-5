#include <gtest/gtest.h>
#include <string>
#include <iterator>
#include "custom_memory.h"
#include "array.h"

struct TestStruct {
    int id;
    std::string name;
    double value;
    
    TestStruct(int i, const std::string& n, double v) : id(i), name(n), value(v) {}
    
    bool operator==(const TestStruct& other) const {
        return id == other.id && name == other.name && value == other.value;
    }
};

// Тест CustomMemoryResource
class CustomMemoryResourceTest : public ::testing::Test {
protected:
    CustomMemoryResource resource;
};

TEST_F(CustomMemoryResourceTest, BasicAllocation) {
    void* ptr1 = resource.allocate(100);
    ASSERT_NE(ptr1, nullptr);
    
    void* ptr2 = resource.allocate(200);
    ASSERT_NE(ptr2, nullptr);
    ASSERT_NE(ptr1, ptr2);
    
    resource.deallocate(ptr1, 100);
    resource.deallocate(ptr2, 200);
}

TEST_F(CustomMemoryResourceTest, MemoryReuse) {
    void* ptr1 = resource.allocate(100);
    resource.deallocate(ptr1, 100);
    
    void* ptr2 = resource.allocate(100);
    EXPECT_EQ(ptr1, ptr2); // LIFO - должен переиспользовать тот же блок
    
    resource.deallocate(ptr2, 100);
}

TEST_F(CustomMemoryResourceTest, LIFOBehavior) {
    void* ptr1 = resource.allocate(50);
    void* ptr2 = resource.allocate(50);
    void* ptr3 = resource.allocate(50);
    
    resource.deallocate(ptr1, 50);
    resource.deallocate(ptr2, 50);
    resource.deallocate(ptr3, 50);
    
    void* reused1 = resource.allocate(50);
    EXPECT_EQ(ptr3, reused1); // Последний освобожденный используется первым
    
    resource.deallocate(reused1, 50);
}

// Тест DynamicArray
class DynamicArrayTest : public ::testing::Test {
protected:
    CustomMemoryResource resource;
};

TEST_F(DynamicArrayTest, InitialCapacity) {
    DynamicArray<int> arr(&resource);
    EXPECT_EQ(arr.size(), 0);
    EXPECT_TRUE(arr.empty());
}

TEST_F(DynamicArrayTest, PushBackInt) {
    DynamicArray<int> arr(&resource);
    
    arr.push_back(10);
    arr.push_back(20);
    arr.push_back(30);
    
    EXPECT_EQ(arr.size(), 3);
    EXPECT_EQ(arr[0], 10);
    EXPECT_EQ(arr[1], 20);
    EXPECT_EQ(arr[2], 30);
}

TEST_F(DynamicArrayTest, PushBackStruct) {
    DynamicArray<TestStruct> arr(&resource);
    
    arr.push_back(TestStruct(1, "test1", 1.5));
    arr.push_back(TestStruct(2, "test2", 2.5));
    
    EXPECT_EQ(arr.size(), 2);
    EXPECT_EQ(arr[0], TestStruct(1, "test1", 1.5));
    EXPECT_EQ(arr[1], TestStruct(2, "test2", 2.5));
}

TEST_F(DynamicArrayTest, PopBack) {
    DynamicArray<int> arr(&resource);
    
    arr.push_back(10);
    arr.push_back(20);
    arr.push_back(30);
    
    arr.pop_back();
    EXPECT_EQ(arr.size(), 2);
    EXPECT_EQ(arr[1], 20);
    
    arr.pop_back();
    arr.pop_back();
    EXPECT_EQ(arr.size(), 0);
}

TEST_F(DynamicArrayTest, AutoResize) {
    DynamicArray<int> arr(&resource);
    
    // Добавляем больше 10 элементов (начальная емкость)
    for (int i = 0; i < 15; ++i) {
        arr.push_back(i);
    }
    
    EXPECT_EQ(arr.size(), 15);
    for (int i = 0; i < 15; ++i) {
        EXPECT_EQ(arr[i], i);
    }
}

TEST_F(DynamicArrayTest, OutOfRangeAccess) {
    DynamicArray<int> arr(&resource);
    arr.push_back(10);
    
    EXPECT_THROW(arr[5], std::out_of_range);
}

// Тест итератора
TEST_F(DynamicArrayTest, ForwardIterator) {
    DynamicArray<int> arr(&resource);
    
    for (int i = 0; i < 5; ++i) {
        arr.push_back(i * 10);
    }
    
    // Проверка итерации
    int expected = 0;
    for (auto it = arr.begin(); it != arr.end(); ++it) {
        EXPECT_EQ(*it, expected * 10);
        ++expected;
    }
    EXPECT_EQ(expected, 5);
}

TEST_F(DynamicArrayTest, IteratorTraits) {
    DynamicArray<int> arr(&resource);
    
    using Iterator = DynamicArray<int>::Iterator;
    
    // Проверка типов итератора
    EXPECT_TRUE((std::is_same_v<Iterator::iterator_category, std::forward_iterator_tag>));
    EXPECT_TRUE((std::is_same_v<Iterator::value_type, int>));
    EXPECT_TRUE((std::is_same_v<Iterator::pointer, int*>));
    EXPECT_TRUE((std::is_same_v<Iterator::reference, int&>));
}

TEST_F(DynamicArrayTest, IteratorOperations) {
    DynamicArray<int> arr(&resource);
    arr.push_back(10);
    arr.push_back(20);
    
    auto it1 = arr.begin();
    auto it2 = arr.begin();
    
    EXPECT_TRUE(it1 == it2);
    EXPECT_FALSE(it1 != it2);
    
    ++it2;
    EXPECT_FALSE(it1 == it2);
    EXPECT_TRUE(it1 != it2);
    
    EXPECT_EQ(*it1, 10);
    EXPECT_EQ(*it2, 20);
}

// Тест использования std::pmr::polymorphic_allocator
TEST_F(DynamicArrayTest, UsesCustomAllocator) {
    DynamicArray<int> arr1(&resource);
    DynamicArray<int> arr2; // Использует default_resource
    
    // Добавляем элементы в оба массива
    for (int i = 0; i < 5; ++i) {
        arr1.push_back(i);
        arr2.push_back(i);
    }
    
    // Оба должны работать корректно
    EXPECT_EQ(arr1.size(), 5);
    EXPECT_EQ(arr2.size(), 5);
}

// Интеграционные тесты
TEST_F(DynamicArrayTest, MemoryReuseIntegration) {
    // Создаем и уничтожаем массивы для проверки переиспользования
    {
        DynamicArray<int> arr1(&resource);
        for (int i = 0; i < 10; ++i) {
            arr1.push_back(i);
        }
    } // arr1 уничтожается
    
    {
        DynamicArray<int> arr2(&resource);
        for (int i = 0; i < 10; ++i) {
            arr2.push_back(i * 2);
        }
        
        EXPECT_EQ(arr2.size(), 10);
        for (int i = 0; i < 10; ++i) {
            EXPECT_EQ(arr2[i], i * 2);
        }
    }
}

TEST_F(DynamicArrayTest, ComplexObjectLifecycle) {
    DynamicArray<TestStruct> arr(&resource);
    
    // Добавляем сложные объекты
    arr.push_back(TestStruct(1, "Alice", 100.5));
    arr.push_back(TestStruct(2, "Bob", 200.5));
    arr.push_back(TestStruct(3, "Charlie", 300.5));
    
    // Проверяем через итератор
    int id = 1;
    for (auto it = arr.begin(); it != arr.end(); ++it) {
        EXPECT_EQ(it->id, id);
        ++id;
    }
    
    // Удаляем элементы
    arr.pop_back();
    EXPECT_EQ(arr.size(), 2);
    EXPECT_EQ(arr[1].name, "Bob");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}