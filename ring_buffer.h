#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <cstring>

namespace std {
template <typename T>
class RingBuffer {
public:

    RingBuffer(const size_t size_, const size_t an_element_size_ = sizeof (T), const bool create_elements = true) : size(size_), an_element_size(an_element_size_) {
        init(create_elements);
    }

    RingBuffer(const size_t size_, const T& elem, const size_t element_size, const size_t an_element_size_ = sizeof (T), const bool create_elements = true) : size(size_), an_element_size(an_element_size_) {
        init(create_elements);
        write(elem, element_size);
    }

    virtual ~RingBuffer() {
        if (elements) {
            free(elements);
        }
    }

    inline bool is_full() const {
        return ((b_end == b_start) && (e_msb != s_msb));
    }

    inline bool is_empty() const {
        return (is_at_end(b_start, s_msb));
    }

    inline void write(const T& element, const size_t size_ = sizeof (T)) {
        memmove((void*) ((uint64_t) (&(elements[0])) + (uint64_t) (an_element_size * b_end)), (void*) &element, size_);
        post_write();
    }

    inline T& read() {
        return (read_from(b_start, s_msb));
    }

    inline T& front() const {
        return (elements[b_start]);
    }

    inline T& back() const {
        return (elements[((b_end > 0) ? (size_t)(b_end - 1) : (size - 1))]);
    }

    inline size_t length() const {
        if (is_empty()) {
            return (0);
        } else if (is_full()) {
            return (size);
        } else {
            return ((size_t)((b_end > b_start) ? b_end - b_start : size + b_end - b_start));
        }
    }

    inline size_t buffer_size() const {
        return (size);
    }

    inline void begin() {
        iterator = b_start;
        iterator_msb = s_msb;
    }

    inline bool end() const {
        return (is_at_end(iterator, iterator_msb));
    }

    inline T& next() {
        return (read_from(iterator, iterator_msb));
    }

    inline void clear() {
        init(false);
    }

    friend inline ostream& operator<<(ostream& os, const RingBuffer<T>& buffer) {
        os << "{ \"full\": \"" << boolalpha << buffer.is_full() <<
           "\", \"empty\": \"" << buffer.is_empty() << "\"" <<
           ", \"size\": " << buffer.size <<
           ", \"start\": " << buffer.b_start <<
           ", \"end\": " << buffer.b_end <<
           ", \"start_msb\": " << buffer.s_msb <<
           ", \"end_msb\": " << buffer.e_msb <<
           ", \"length\": " << buffer.length();
        if (!buffer.is_empty()) {
            os << ", \"front\": " << buffer.front() << ", \"back\": " << buffer.back();
        }
        os << " }";
        return os;
    }

protected:
    size_t size;
    size_t an_element_size;
    size_t b_start;
    size_t b_end;
    short int s_msb;
    short int e_msb;
    size_t iterator;
    short int iterator_msb;
    size_t index;
    T* elements;

    inline void incr(size_t& p, short int& msb) const {
        if (++p == size) {
            msb ^= 1;
            p = 0;
        }
    }

    inline void post_write() {
        if (is_full()) {
            incr(b_start, s_msb);
        }
        incr(b_end, e_msb);
    }

    inline void init(const bool create_elements = true) {
        b_start = 0;
        b_end = 0;
        s_msb = 0;
        e_msb = 0;
        iterator = 0;
        iterator_msb = 0;
        index = 0;
        if (create_elements) {
            elements = (T*)malloc(size * an_element_size);
        }
    }

    inline bool is_at_end(const size_t i, const short int msb) const {
        return ((b_end == i) && (e_msb == msb));
    }

    inline T& read_from(size_t& i, short int& msb) {
        index = i;
        incr(i, msb);
        return elements[index];
    }
};

}

#endif