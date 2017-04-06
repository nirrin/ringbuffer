#include "ring_buffer.h"
#include <cassert>

const std::size_t SIZE = 4;

int base_test_ring_buffer(std::RingBuffer<int>& buffer) {
    assert(buffer.buffer_size() == SIZE);
    assert(buffer.is_empty());
    assert(!buffer.is_full());
    assert(buffer.length() == 0);
    buffer.begin();
    while (!buffer.end()) {
        assert(buffer.next() == 999);
    }

    buffer.write(1);
    assert(buffer.buffer_size() == SIZE);
    assert(!buffer.is_empty());
    assert(!buffer.is_full());
    assert(buffer.length() == 1);
    assert(buffer.front() == 1);
    assert(buffer.back() == 1);
    buffer.begin();
    int i = 1;
    while (!buffer.end()) {
        assert(buffer.next() == i++);
    }
    assert(i == 2);

    buffer.write(2);
    assert(buffer.buffer_size() == SIZE);
    assert(!buffer.is_empty());
    assert(!buffer.is_full());
    assert(buffer.length() == 2);
    assert(buffer.front() == 1);
    assert(buffer.back() == 2);
    buffer.begin();
    i = 1;
    while (!buffer.end()) {
        assert(buffer.next() == i++);
    }
    assert(i == 3);

    buffer.write(3);
    assert(buffer.buffer_size() == SIZE);
    assert(!buffer.is_empty());
    assert(!buffer.is_full());
    assert(buffer.length() == 3);
    assert(buffer.front() == 1);
    assert(buffer.back() == 3);
    buffer.begin();
    i = 1;
    while (!buffer.end()) {
        assert(buffer.next() == i++);
    }
    assert(i == 4);

    buffer.write(4);
    assert(buffer.buffer_size() == SIZE);
    assert(!buffer.is_empty());
    assert(buffer.is_full());
    assert(buffer.length() == 4);
    assert(buffer.front() == 1);
    assert(buffer.back() == 4);
    buffer.begin();
    i = 1;
    while (!buffer.end()) {
        assert(buffer.next() == i++);
    }
    assert(i == 5);

    buffer.write(5);
    assert(buffer.buffer_size() == SIZE);
    assert(!buffer.is_empty());
    assert(buffer.is_full());
    assert(buffer.length() == 4);
    assert(buffer.front() == 2);
    assert(buffer.back() == 5);
    buffer.begin();
    i = 2;
    while (!buffer.end()) {
        assert(buffer.next() == i++);
    }
    assert(i == 6);

    buffer.write(6);
    assert(buffer.buffer_size() == SIZE);
    assert(!buffer.is_empty());
    assert(buffer.is_full());
    assert(buffer.length() == 4);
    assert(buffer.front() == 3);
    assert(buffer.back() == 6);
    buffer.begin();
    i = 3;
    while (!buffer.end()) {
        assert(buffer.next() == i++);
    }
    assert(i == 7);

    assert(buffer.read() == 3);
    assert(buffer.buffer_size() == SIZE);
    assert(!buffer.is_empty());
    assert(!buffer.is_full());
    assert(buffer.length() == 3);
    assert(buffer.front() == 4);
    assert(buffer.back() == 6);
    buffer.begin();
    i = 4;
    while (!buffer.end()) {
        assert(buffer.next() == i++);
    }
    assert(i == 7);

    assert(buffer.read() == 4);
    assert(buffer.buffer_size() == SIZE);
    assert(!buffer.is_empty());
    assert(!buffer.is_full());
    assert(buffer.length() == 2);
    assert(buffer.front() == 5);
    assert(buffer.back() == 6);
    buffer.begin();
    i = 5;
    while (!buffer.end()) {
        assert(buffer.next() == i++);
    }
    assert(i == 7);

    assert(buffer.read() == 5);
    assert(buffer.buffer_size() == SIZE);
    assert(!buffer.is_empty());
    assert(!buffer.is_full());
    assert(buffer.length() == 1);
    assert(buffer.front() == 6);
    assert(buffer.back() == 6);
    buffer.begin();
    i = 6;
    while (!buffer.end()) {
        assert(buffer.next() == i++);
    }
    assert(i == 7);

    assert(buffer.read() == 6);
    assert(buffer.buffer_size() == SIZE);
    assert(buffer.is_empty());
    assert(!buffer.is_full());
    assert(buffer.length() == 0);
    buffer.begin();
    while (!buffer.end()) {
        assert(buffer.next() == 999);
    }

    buffer.write(1);
    buffer.write(2);
    buffer.write(3);
    buffer.write(4);
    assert(buffer.is_full());
    assert(!buffer.is_empty());
    buffer.clear();
    assert(!buffer.is_full());
    assert(buffer.is_empty());

    return 0;
}

int test_ring_buffer() {
    std::RingBuffer<int> buffer(SIZE);
    base_test_ring_buffer(buffer);
    base_test_ring_buffer(buffer);
    base_test_ring_buffer(buffer);
    base_test_ring_buffer(buffer);
    base_test_ring_buffer(buffer);
    base_test_ring_buffer(buffer);

    return 0;
}

int main() {
    return test_ring_buffer();
}