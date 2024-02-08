//
// Created by Sebastian on 05.01.2024.
//

#ifndef VECTOR_VECTOR_H
#define VECTOR_VECTOR_H

#include <cstddef>
#include <stdexcept>
#include <limits>
#include <cstdint>
#include <cassert>
#include <cstdarg>
#include <span>

template<typename T>
class Vector {
private:
    inline static const double growthFactor = 1.5;
    uint8_t *m_data{};
    size_t m_capacity{};
    size_t m_elemCount{};

    void *allocMany(size_t elemCount, size_t elemSize) {
        void *mem = malloc(elemCount * elemSize);

        if (mem == nullptr) {
            throw std::bad_alloc();
        }
        return mem;
    }

    void growBuffer(size_t elemCount) {
        const size_t nextCapacity = m_capacity * growthFactor;
        const size_t actualNewCapacity = std::max(nextCapacity, m_elemCount + elemCount);

        allocateBuffer(actualNewCapacity);
    }

    void allocateBuffer(size_t bufferSize) {
        auto *tmpBuffer = static_cast<uint8_t *>(allocMany(bufferSize, sizeof(T)));

        if (tmpBuffer == nullptr) {
            throw std::bad_alloc();
        }

        for (size_t i = 0; i < m_elemCount; i++) {
            const size_t elemOffset = i * sizeof(T);
            auto *obj = (T *) &m_data[elemOffset];
            new(tmpBuffer + elemOffset)T(std::move(*obj));
            obj->~T();
        }

        free(m_data);
        m_data = tmpBuffer;
        m_capacity = bufferSize;
    }

    void insertElem(const T &value) {
        uint8_t *insertPtr = growIfNeeded(1);
        new(insertPtr)T(value);

        m_elemCount++;
    }

    [[nodiscard]] bool shouldResizeBuffer(size_t elemCount) const {
        return m_elemCount + elemCount > m_capacity;
    }

    uint8_t *getPointerToWriteableMemory() const {
        return &m_data[sizeof(T) * m_elemCount];
    }

    uint8_t *growIfNeeded(size_t minRequiredCapacity) noexcept(false) {
        if (shouldResizeBuffer(minRequiredCapacity)) {
            growBuffer(minRequiredCapacity);
        }

        return getPointerToWriteableMemory();
    }

    void destructElems(size_t from, size_t to) {
        T *startPos = begin().m_ptr + from;
        T *endPos = begin().m_ptr + to;

        while (startPos != endPos) {
            startPos->~T();
            startPos++;
        }
    }

    void shiftElemsRight(size_t from, size_t amount) {
        const T *startPtr = &((T *) m_data)[from];
        T *endPtr = &((T *) m_data)[m_elemCount - 1];

        while (endPtr >= startPtr) {
            new(endPtr + amount)T(std::move(*endPtr));
            endPtr->~T();
            endPtr--;
        }
    }

    void moveElemsToOtherBuffer(T *destBuffer, T *srcBufferFrom, T *srcBufferTo) {
        for (T *elem = srcBufferFrom; elem != srcBufferTo; elem++) {
            new(destBuffer++)T(std::move(*elem));
            elem->~T();
        }
    }

    T *insertAt(size_t pos, const T &elem, size_t count = 1) {
        // Get pointer to position in memory chunk, allocate new memory if required
        const bool allocatedNewMemoryChunk = shouldResizeBuffer(count);

        // Insert to the end
        if (pos == m_elemCount) {
            T *insertPtr = (T *) growIfNeeded(count);
            for (size_t i = 0; i < count; i++) {
                new(insertPtr)T(elem);
                m_elemCount++;
                insertPtr++;
            }

            return &((T *) m_data)[pos];
        }

        // Elements can be moved when there is no new memory chunk allocated
        if (!allocatedNewMemoryChunk) {
            T *movePtr = ((T *) m_data) + pos;

            // Shift
            shiftElemsRight(pos, count);

            // Insert new elements
            for (size_t i = 0; i < count; i++) {
                new(movePtr)T(elem);
                movePtr++;
                m_elemCount++;
            }
        } else {
            const size_t totalElements = m_elemCount + count;
            const size_t nextCapacity = m_capacity * growthFactor;
            const size_t actualNewCapacity = std::max(nextCapacity, totalElements);

            T *tmpBuffer = static_cast<T *>(allocMany(actualNewCapacity, sizeof(T)));
            T *startOldBuffer = (T *) m_data;

            // Left
            moveElemsToOtherBuffer(tmpBuffer, startOldBuffer, startOldBuffer + pos);

            // New elem insertion
            tmpBuffer += pos;

            for (size_t i = 0; i < count; i++) {
                new(tmpBuffer++)T(elem);
            }

            // Right
            moveElemsToOtherBuffer(tmpBuffer, startOldBuffer + pos, end().m_ptr);

            free(m_data);
            tmpBuffer -= (pos + count);
            m_data = (uint8_t *) tmpBuffer;
            m_capacity = actualNewCapacity;
            m_elemCount = totalElements;
        }

        return &((T *) m_data)[pos];
    }

    T *insertAt(size_t pos, T &&elem) {
        // Get pointer to position in memory chunk, allocate new memory if required
        const bool allocatedNewMemoryChunk = shouldResizeBuffer(1);

        // Insert to the end
        if (pos == m_elemCount) {
            T *insertPtr = (T *) growIfNeeded(1);
            new(insertPtr)T(std::move(elem));
            elem.~T();
            m_elemCount++;

            return &((T *) m_data)[pos];
        }

        // Elements can be moved when there is no new memory chunk allocated
        if (!allocatedNewMemoryChunk) {
            T *movePtr = ((T *) m_data) + pos;

            // Shift
            shiftElemsRight(pos, 1);

            // Insert new elements
            new(movePtr)T(std::move(elem));
            elem.~T();
            m_elemCount++;
        } else {
            const size_t totalElements = m_elemCount + 1;
            const size_t nextCapacity = m_capacity * growthFactor;
            const size_t actualNewCapacity = std::max(nextCapacity, totalElements);

            T *tmpBuffer = static_cast<T *>(allocMany(actualNewCapacity, sizeof(T)));
            T *startOldBuffer = (T *) m_data;

            // Left
            moveElemsToOtherBuffer(tmpBuffer, startOldBuffer, startOldBuffer + pos);

            // New elem insertion
            tmpBuffer += pos;
            new(tmpBuffer)T(std::move(elem));
            elem.~T();

            // Right
            moveElemsToOtherBuffer(++tmpBuffer, startOldBuffer + pos, end().m_ptr);

            free(m_data);
            tmpBuffer -= (pos + 1); // Move back to buffer start
            m_data = (uint8_t *) tmpBuffer;
            m_capacity = actualNewCapacity;
            m_elemCount = totalElements;
        }

        return &((T *) m_data)[pos];
    }

    T *insertAt(size_t pos, const T *elemsRangeBegin, const T *elemsRangeEnd) {
        // Get pointer to position in memory chunk, allocate new memory if required
        const size_t elemCount = elemsRangeEnd - elemsRangeBegin;
        const bool allocatedNewMemoryChunk = m_capacity < m_elemCount + elemCount;

        // Insert to the end
        if (pos == m_elemCount) {
            T *insertPtr = (T *) growIfNeeded(elemCount);

            for (const T *elem = elemsRangeBegin; elem != elemsRangeEnd; elem++) {
                new(insertPtr)T(*elem);
                m_elemCount++;
                insertPtr++;
            }
        } else {
            // Elements can be moved when there is no new memory chunk allocated
            if (!allocatedNewMemoryChunk) {
                T *movePtr = ((T *) m_data) + pos;
                // Move stored elements to make space
                shiftElemsRight(pos, elemCount);

                // Insert new elements
                for (size_t i = 0; i < elemCount; i++) {
                    new(movePtr++)T(*(elemsRangeBegin++));
                    m_elemCount++;
                }
            } else {
                const size_t totalElements = m_elemCount + elemCount;

                const size_t nextCapacity = m_capacity * growthFactor;
                const size_t actualNewCapacity = std::max(nextCapacity, totalElements);

                T *tmpBuffer = static_cast<T *>(allocMany(actualNewCapacity, sizeof(T)));
                T *startOldBuffer = (T *) m_data;

                // Before new elems insert
                moveElemsToOtherBuffer(tmpBuffer, startOldBuffer, startOldBuffer + pos);

                tmpBuffer += pos;

                // Insert new elems
                for (const T *elem = elemsRangeBegin; elem != elemsRangeEnd; elem++) {
                    new((tmpBuffer)++)T(*(elem));
                }

                // After new elems insert
                moveElemsToOtherBuffer(tmpBuffer, startOldBuffer + pos, end().m_ptr);

                free(m_data);
                tmpBuffer -= (pos + elemCount); // Move back to buffer start
                m_data = (uint8_t *) tmpBuffer;
                m_capacity = actualNewCapacity;
                m_elemCount = totalElements;
            }
        }
        return &((T *) m_data)[pos];
    }

    T *eraseAt(T *elemsRangeBegin, T *elemsRangeEnd) {
        // No need to fill the gaps
        if (elemsRangeEnd == end().m_ptr) {
            for (const T *elem = elemsRangeBegin; elem != elemsRangeEnd; elem++) {
                // No need to free any memory
                elem->~T();
            }

            m_elemCount -= elemsRangeEnd - elemsRangeBegin;
            return end().m_ptr;
        }

        for (const T *elem = elemsRangeBegin; elem != elemsRangeEnd; elem++) {
            elem->~T();
        }

        const size_t deleteCount = elemsRangeEnd - elemsRangeBegin;

        for (T *elem = elemsRangeEnd; elem != end().m_ptr; elem++) {

            new(elem - deleteCount)T(std::move(*elem));
            elem->~T();
        }

        m_elemCount -= elemsRangeEnd - elemsRangeBegin;
        return elemsRangeEnd;
    }

public:
    // Iterators
    struct iterator {
        template<typename> friend
        class Vector;

        using Category = std::forward_iterator_tag;
        using Distance = std::ptrdiff_t;
        using value_type = T;
        using Pointer = T *;
        using Reference = T &;

        explicit iterator(Pointer ptr) : m_ptr{ptr} {}

        Reference operator*() const {
            return *m_ptr;
        }

        Pointer operator->() const {
            return m_ptr;
        }

        // ++it
        Vector<T>::iterator &operator++() {
            ++m_ptr;
            return *this;
        }

        // it++;
        Vector<T>::iterator operator++(int) {
            iterator tmp = *this;
            ++m_ptr;
            return tmp;
        }

        // --it
        Vector<T>::iterator &operator--() {
            --m_ptr;
            return *this;
        }

        Vector<T>::iterator operator--(int) {
            iterator tmp = *this;
            --m_ptr;
            return tmp;
        }

        Vector<T>::iterator &operator+=(size_t rhs) {
            m_ptr += rhs;
            return *this;
        }

        Vector<T>::iterator &operator-=(size_t rhs) {
            m_ptr -= rhs;
            return *this;
        }

        friend bool operator==(const iterator &lhs, const iterator &rhs) {
            return lhs.m_ptr == rhs.m_ptr;
        }

        friend bool operator!=(const iterator &lhs, const iterator &rhs) {
            return lhs.m_ptr != rhs.m_ptr;
        }

    private:
        Pointer m_ptr;
    };

    struct const_iterator {
        template<typename> friend
        class Vector;

        using Category = std::forward_iterator_tag;
        using Distance = std::ptrdiff_t;
        using value_type = T;
        using Pointer = const T *;
        using Reference = const T &;

        explicit const_iterator(Pointer ptr) : m_ptr{ptr} {}

        Reference operator*() const {
            return *m_ptr;
        }

        Pointer operator->() const {
            return m_ptr;
        }

        // ++it
        Vector<T>::const_iterator &operator++() {
            ++m_ptr;
            return *this;
        }

        // it++;
        Vector<T>::const_iterator operator++(int) {
            const_iterator tmp = *this;
            ++m_ptr;
            return tmp;
        }

        // --it
        Vector<T>::const_iterator &operator--() {
            --m_ptr;
            return *this;
        }

        Vector<T>::const_iterator operator--(int) {
            iterator tmp = *this;
            --m_ptr;
            return tmp;
        }

        Vector<T>::const_iterator &operator+=(size_t rhs) {
            m_ptr += rhs;
            return *this;
        }

        Vector<T>::const_iterator &operator-=(size_t rhs) {
            m_ptr -= rhs;
            return *this;
        }

        friend bool operator==(const const_iterator &lhs, const const_iterator &rhs) {
            return lhs.m_ptr == rhs.m_ptr;
        }

        friend bool operator!=(const const_iterator &lhs, const const_iterator &rhs) {
            return lhs.m_ptr != rhs.m_ptr;
        }

    private:
        Pointer m_ptr;
    };

    // Constructors
    Vector() = default;

    explicit Vector(size_t capacity) {
        m_capacity = capacity;

        // Alloc required memory
        m_data = static_cast<uint8_t *>(allocMany(capacity, sizeof(T)));

        if (m_data == nullptr) {
            throw std::bad_alloc();
        }
    }

    Vector(std::initializer_list<T> values) {
        m_capacity = values.size();

        // Alloc required memory
        m_data = static_cast<uint8_t *>(allocMany(m_capacity, sizeof(T)));

        if (m_data == nullptr) {
            throw std::bad_alloc();
        }

        for (const T &value: values) {
            insertElem(value);
        }
    }

    // Copy ctor
    Vector(const Vector<T> &other) {
        size_t elemCount{};
        T *bufferStart = (T *) allocMany(other.m_capacity, sizeof(T));

        try {
            for (const T *elem = other.begin().m_ptr; elem != other.end().m_ptr; elem++) {
                new(bufferStart++)T(*elem);
                ++elemCount;
            }
        } catch (...) {
            // Clean up
            for (T *elem = bufferStart; elem != bufferStart + elemCount; elem++) {
                elem->~T();
            }

            free(m_data);
            throw;
        }

        m_elemCount = elemCount;
        m_capacity = other.m_capacity;
        bufferStart -= m_elemCount;
        m_data = (uint8_t *) bufferStart;
    }

    // Copy assignment
    Vector<T> &operator=(const Vector<T> &rhs) {
        if (this == &rhs) {
            return *this;
        }

        T *bufferToStore{};
        size_t capacityToStore{};
        size_t elemCount{};

        // No need to allocate new memory
        if (m_capacity >= rhs.m_capacity) {
            if (m_elemCount > rhs.m_elemCount) {
                // We can use copy assignment for the elems to be inserted
                destructElems(rhs.m_elemCount, m_elemCount);
            }

            bufferToStore = begin().m_ptr;
            capacityToStore = m_capacity;
        } else {
            destructElems(0, m_elemCount);

            m_elemCount = 0;
            m_capacity = 0;
            m_data = nullptr;
            free(m_data);

            T *bufferStart = (T *) allocMany(rhs.m_capacity, sizeof(T));
            bufferToStore = bufferStart;
            capacityToStore = rhs.m_capacity;
        }

        try {
            for (const T *elem = rhs.begin().m_ptr; elem != rhs.end().m_ptr; elem++) {
                // Element at the current pos and old memory reused
                if (m_elemCount >= elemCount && m_capacity >= rhs.m_capacity) {
                    *(bufferToStore++) = *elem;
                } else {
                    new(bufferToStore++)T(*elem);
                }

                ++elemCount;
            }
        } catch (...) {
            // Clean up
            for (T *elem = bufferToStore - elemCount; elem != bufferToStore + elemCount; elem++) {
                elem->~T();
            }

            free(m_data);
            throw;
        }

        m_elemCount = elemCount;
        bufferToStore -= m_elemCount;
        m_data = (uint8_t *) bufferToStore;
        m_capacity = capacityToStore;

        return *this;
    }

    // Move ctor
    Vector(Vector<T>&& rhs) noexcept {
        m_data = rhs.m_data;
        m_elemCount = rhs.m_elemCount;
        m_capacity = rhs.m_capacity;

        rhs.m_data = nullptr;
        rhs.m_elemCount = 0;

    }

    // Move assignment
    Vector<T>& operator=(Vector<T>&& rhs) noexcept {
        destructElems(0, m_elemCount);
        free(m_data);

        m_data = rhs.m_data;
        m_elemCount = rhs.m_elemCount;
        m_capacity = rhs.m_capacity;

        rhs.m_data = nullptr;
        rhs.m_elemCount = 0;
        rhs.m_capacity = 0;

        return *this;
    }

    ~Vector() {
        // Can be the case when move semantics got triggered
        if (m_data == nullptr) {
            return;
        }

        for (const T *elem = cbegin().m_ptr; elem != cend().m_ptr; elem++) {
            elem->~T();
        }

        free(m_data);
    }

    // Modifiers
    void clear() {
        if (m_elemCount == 0) {
            return;
        }

        T *start = (T *) m_data;
        T *end = (T *) m_data + m_elemCount;

        while (start != end) {
            start->~T();
            start++;
        }

        m_elemCount = 0;
    }

    iterator insert(iterator pos, const T &value) {
        if ((uintptr_t) pos.m_ptr<(uintptr_t) begin().m_ptr || (uintptr_t) pos.m_ptr>(uintptr_t)end().m_ptr) {
            assert("Iterator pointer out of range");
        }

        const size_t index = pos.m_ptr - begin().m_ptr;
        T *insertPos = insertAt(index, value);

        return iterator{insertPos};
    }

    iterator insert(iterator pos, size_t count, const T &value) {
        if ((uintptr_t) pos.m_ptr<(uintptr_t) begin().m_ptr || (uintptr_t) pos.m_ptr>(uintptr_t)end().m_ptr) {
            assert("Iterator pointer out of range");
        }

        const size_t index = pos.m_ptr - begin().m_ptr;
        T *insertPos = insertAt(index, value, count);

        return iterator{insertPos};
    }

    iterator insert(iterator pos, T &&value) {
        if ((uintptr_t) pos.m_ptr<(uintptr_t) begin().m_ptr || (uintptr_t) pos.m_ptr>(uintptr_t)end().m_ptr) {
            assert("Iterator pointer out of range");
        }

        const size_t index = pos.m_ptr - begin().m_ptr;
        T *insertPos = insertAt(index, std::forward<T>(value));

        return iterator{insertPos};
    }

    iterator insert(iterator pos, iterator first, iterator last) {
        if ((uintptr_t) pos.m_ptr<(uintptr_t) begin().m_ptr || (uintptr_t) pos.m_ptr>(uintptr_t)end().m_ptr) {
            assert("Iterator pointer out of range");
        }

        const size_t index = pos.m_ptr - begin().m_ptr;

        T *insertPos = insertAt(index, first.m_ptr, last.m_ptr);

        return iterator{insertPos};
    }

    iterator erase(iterator pos) {
        T *deletePos = pos.m_ptr;
        return iterator{eraseAt(deletePos, deletePos + 1)};
    }

    iterator erase(iterator first, iterator last) {
        T *from = first.m_ptr;
        T *to = last.m_ptr;

        return iterator{eraseAt(from, to)};
    }

    void push_back(const T &value) {
        insertElem(value);
    }

    void push_back(T &&value) {
        emplace_back(std::forward<T>(value));
    }

    void emplace_back(T &&value) {
        uint8_t *insertPtr = growIfNeeded(1);
        new(insertPtr)T(std::move(value));

        m_elemCount++;
    }

    void pop_back() {
        if (m_elemCount == 0) {
            return;
        }

        T *lastElem = end().m_ptr - 1;
        lastElem->~T();
        m_elemCount--;
    }

    // Element access
    T &at(size_t pos) const {
        if (pos >= m_elemCount) {
            throw std::out_of_range("Out of range");
        }

        return ((T *) m_data)[pos];
    }

    T &operator[](size_t index) {
        return ((T *) m_data)[index];
    }

    T &front() const {
        if (empty()) {
            throw std::out_of_range("Container is empty");
        }

        return ((T *) m_data)[0];
    }

    T &back() const {
        if (empty()) {
            throw std::out_of_range("Container is empty");
        }

        return ((T *) m_data)[m_elemCount - 1];
    }

    T *data() const {
        return (T *) reinterpret_cast<int *>(m_data);
    }

    // Capacity
    bool empty() const {
        return m_elemCount == 0;
    }

    [[nodiscard]] size_t size() const {
        return m_elemCount;
    }

    [[nodiscard]] size_t max_size() const {
        return std::numeric_limits<std::make_signed_t<size_t>>::max() / sizeof(T);
    }

    void reserve(size_t newCapacity) {
        growIfNeeded(newCapacity);
    }

    [[nodiscard]] size_t capacity() const {
        return m_capacity;
    }

    void shrink_to_fit() {
        allocateBuffer(m_elemCount);
    }

    iterator begin() {
        return iterator{data()};
    }

    iterator end() {
        return iterator{(data() + m_elemCount)};
    }

    const_iterator begin() const {
        return const_iterator{(T *) m_data};
    }

    const_iterator cbegin() const {
        return const_iterator{(T *) m_data};
    }

    const_iterator end() const {
        return const_iterator{((T *) m_data) + m_elemCount};
    }

    const_iterator cend() const {
        return const_iterator{((T *) m_data) + m_elemCount};
    }
};

#endif //VECTOR_VECTOR_H
