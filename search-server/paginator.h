#pragma once
#include <algorithm>
#include <iostream>
#include "document.h"
template <typename Iterator>
class IteratorRange  {
public:
        IteratorRange (Iterator begin, Iterator end) : begin_(begin), end_(end), size_(distance(begin_, end_))
        {
        }
 
        auto begin() const {
        return begin_;
    }
 
        auto end() const {
        return end_;
    }
 
        size_t page_size() const {
        return size_;
    }
private:
    Iterator begin_; 
    Iterator end_;
    size_t size_;
};
 
 
template <typename Iterator>
class Paginator {
public:
        Paginator(Iterator begin, Iterator end, size_t page_size) {
           size_t length = distance(begin, end);
           while ( length > 0 ) {
               if ( length > page_size ) {
                   pages_.push_back({begin, begin + page_size});
                   length -= page_size;
                   begin += page_size;
               } else {
                   pages_.push_back({begin, begin + length});
                   length = 0;
               }
           }
    }
 
        auto begin() const {
            return pages_.begin();
        }
        auto end() const {
            return pages_.end();
        }
 
        size_t page_size() const {
            return pages_.size();
        }
private:
    std::vector<IteratorRange<Iterator>> pages_;
}; 
 
template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}
 
std::ostream& operator<<(std::ostream& out, Document doc) {
    return out << "{ document_id = " << doc.id << ", relevance = " << doc.relevance << ", rating = " << doc.rating << " }";
}
 
template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator>& range) {
    for (auto it = range.begin(); it < range.end(); ++it) {
        out << *it;
    }
    return out;
}