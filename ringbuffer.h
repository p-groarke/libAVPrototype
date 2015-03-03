/*
 *
 * To the extent possible under law, Philippe Groarke has waived all
 * copyright and related or neighboring rights to ringbuffer.h.
 *
 * You should have received a copy of the CC0 legalcode along with this
 * work.  If not, see http://creativecommons.org/publicdomain/zero/1.0/
 *
 */

#ifndef RINGBUFFER
#define RINGBUFFER

#include <atomic>
#include <deque>
#include <QDebug>

class Ringbuffer {
public:
    Ringbuffer(const size_t size) :
        size_(size), head_(0), tail_(0) {
        buffer_ = (char*)malloc(size_);
    }

    ~Ringbuffer() { free(buffer_); }

    void write(const void* object, size_t size) {
        if (size > size_) {
            qDebug() << "Trying to write over yourself,"
                    << "please choose a bigger Ringbuffer size.";
            return;
        }
        size_t head = head_.load(std::memory_order_relaxed);
        size_t nextHead = next(head, size);

        // If the head is passing tail.
        tailCatchup(head, size);

        if (head + size <= size_) {
            memcpy(&buffer_[head], object, size);

        } else {
            const size_t smallerSize = size_ - head; // Gives max write length.
            memcpy(&buffer_[head], object, smallerSize); // Copy as much as possible.
            memcpy(&buffer_[0], (const char*)object + smallerSize,
                    size - smallerSize); // Copy remaining at pos 0.
        }

        head_.store(nextHead, std::memory_order_release);
        sizes_.push_back(size);
        return;
    }

    void* read() {
        size_t tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire)) {
            qDebug() << "Can't read Ringbuffer, no data.";
            return NULL;
        }

        size_t size = sizes_.front();
        sizes_.pop_front();
        void* object = malloc(size);

        if (tail + size <= size_) {
            memcpy(object, &buffer_[tail], size);
        } else {
            const size_t smallerSize = size_ - tail;
            memcpy(object, &buffer_[tail], smallerSize);
            memcpy(object, &buffer_[0], size - smallerSize);
        }

        tail_.store(next(tail, size), std::memory_order_release);
        return object;
    }

private:
    size_t next(size_t curr, size_t newSize) {
        return (curr + newSize) % size_;
    }

    void tailCatchup(size_t head, size_t newSize) {
        size_t tail = tail_.load(std::memory_order_acquire);

        size_t newHead = next(head, newSize);

        int delta = newHead - head;
        bool catchup = false;


        if (delta >= 0) {
            if (head < tail && newHead >= tail)
                catchup = true;
        } else {
            if (head < tail + size_ && newHead >= tail)
                catchup = true;
        }
        if (tail == 32256)
            printf("debugme");

        if (catchup) {

        // Basically, the portion between tail and head.
        //if (head + newSize > tail && temp < head) {
            qDebug() << "Ringbuffer isn't being consumed fast enough.";

            // Find the new tail which matches a stored object.
            size_t newPos = sizes_.front();
            sizes_.pop_front();
            while (tail + newPos < next(head,newSize)) {
                newPos += sizes_.front();
                sizes_.pop_front();
            }
            qDebug() << "New tail position" << next(tail, newPos);
            tail_.store(next(tail, newPos),
                    std::memory_order_release);
        }
        return;
    }

    char* buffer_;
    const size_t size_;
    std::atomic<size_t> head_;
    std::atomic<size_t> tail_;
    std::deque<size_t> sizes_; // Save sizes of objects.

};

#endif // RINGBUFFER

