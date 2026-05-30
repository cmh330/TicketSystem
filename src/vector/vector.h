#ifndef SJTU_VECTOR_HPP
#define SJTU_VECTOR_HPP

#include "../vector/exceptions.h"

#include <climits>
#include <cstddef>

namespace sjtu
{
/**
 * a data container like std::vector
 * store data in a successive memory and support random access.
 */
template<typename T>
class vector
{
public:
	/**
	 * TODO
	 * a type for actions of the elements of a vector, and you should write
	 *   a class named const_iterator with same interfaces.
	 */
	/**
	 * you can see RandomAccessIterator at CppReference for help.
	 */
	class const_iterator;
	class iterator
	{
	// The following code is written for the C++ type_traits library.
	// Type traits is a C++ feature for describing certain properties of a type.
	// For instance, for an iterator, iterator::value_type is the type that the
	// iterator points to.
	// STL algorithms and containers may use these type_traits (e.g. the following
	// typedef) to work properly. In particular, without the following code,
	// @code{std::sort(iter, iter1);} would not compile.
	// See these websites for more information:
	// https://en.cppreference.com/w/cpp/header/type_traits
	// About value_type: https://blog.csdn.net/u014299153/article/details/72419713
	// About iterator_category: https://en.cppreference.com/w/cpp/iterator
	public:
		using difference_type = std::ptrdiff_t;
		using value_type = T;
		using pointer = T*;
		using reference = T&;
		using iterator_category = std::output_iterator_tag;

	private:
		/**
		 * TODO add data members
		 *   just add whatever you want.
		 */
		T* p;
		vector* vec;
		friend class vector;
		friend class const_iterator;
		iterator(T* p_, vector* vec_) : p(p_), vec(vec_) {}
		iterator() : p(nullptr), vec(nullptr) {}
	public:
		/**
		 * return a new iterator which pointer n-next elements
		 * as well as operator-
		 */
		iterator operator+(const int &n) const
		{
			//TODO
			return iterator(p + n, vec);
		}
		iterator operator-(const int &n) const
		{
			//TODO
			return iterator(p - n, vec);
		}
		// return the distance between two iterators,
		// if these two iterators point to different vectors, throw invaild_iterator.
		int operator-(const iterator &rhs) const
		{
			//TODO
			if (vec != rhs.vec) throw invalid_iterator();
			return p - rhs.p;
		}
		iterator& operator+=(const int &n)
		{
			//TODO
			p += n;
			return *this;
		}
		iterator& operator-=(const int &n)
		{
			//TODO
			p -= n;
			return *this;
		}
		/**
		 * TODO iter++
		 */
		iterator operator++(int) {
			iterator tmp = *this;
			p += 1;
			return tmp;
		}
		/**
		 * TODO ++iter
		 */
		iterator& operator++() {
			p += 1;
			return *this;
		}
		/**
		 * TODO iter--
		 */
		iterator operator--(int) {
			iterator tmp = *this;
			p -= 1;
			return tmp;
		}
		/**
		 * TODO --iter
		 */
		iterator& operator--() {
			p -= 1;
			return *this;
		}
		/**
		 * TODO *it
		 */
		T& operator*() const {
			return *p;
		}
		/**
		 * a operator to check whether two iterators are same (pointing to the same memory address).
		 */
		bool operator==(const iterator &rhs) const {
			return p == rhs.p;
		}
		bool operator==(const const_iterator &rhs) const {
			return p == rhs.p;
		}
		/**
		 * some other operator for iterator.
		 */
		bool operator!=(const iterator &rhs) const {
			return p != rhs.p;
		}
		bool operator!=(const const_iterator &rhs) const {
			return p != rhs.p;
		}
	};
	/**
	 * TODO
	 * has same function as iterator, just for a const object.
	 */
	class const_iterator
	{
	public:
		using difference_type = std::ptrdiff_t;
		using value_type = T;
		using pointer = T*;
		using reference = T&;
		using iterator_category = std::output_iterator_tag;

	private:
		/*TODO*/
		const T* p;
		const vector* vec;
		friend class iterator;
		friend class vector;
		const_iterator(const T* p_, const vector* vec_) : p(p_), vec(vec_) {}
		const_iterator() : p(nullptr), vec(nullptr) {}
	public:
		const_iterator operator+(const int &n) const
		{
			return const_iterator(p + n, vec);
		}
		const_iterator operator-(const int &n) const
		{
			return const_iterator(p - n, vec);
		}
		const_iterator& operator+=(const int &n)
		{
			p += n;
			return *this;
		}
		const_iterator& operator-=(const int &n)
		{
			p -= n;
			return *this;
		}
		/**
		 * TODO iter++
		 */
		const_iterator operator++(int) {
			const_iterator tmp = *this;
			p += 1;
			return tmp;
		}
		/**
		 * TODO ++iter
		 */
		const_iterator& operator++() {
			p += 1;
			return *this;
		}
		/**
		 * TODO iter--
		 */
		const_iterator operator--(int) {
			const_iterator tmp = *this;
			p -= 1;
			return tmp;
		}
		/**
		 * TODO --iter
		 */
		const_iterator& operator--() {
			p -= 1;
			return *this;
		}
		/**
		 * TODO *it
		 */
		const T& operator*() const {
			return *p;
		}
		bool operator==(const const_iterator &rhs) const {
			return p == rhs.p;
		}
		bool operator!=(const const_iterator &rhs) const {
			return p != rhs.p;
		}
	};
private:
	T* start;
	T* finish;  // 指向最后元素下一个位置
	T* end_of_storage;

	void checkRange(int pos) const {
		if (pos >= size() || pos < 0) throw index_out_of_bound();
	}
	void checkEmpty() const {
		if (empty()) throw container_is_empty();
	}
	void expand(size_t new_capacity) {
		T* new_start = static_cast<T*>(operator new(new_capacity * sizeof(T)));
		size_t old_size = size();
		for (size_t i = 0; i < old_size; i++) {
			new(new_start + i) T(start[i]);
		}
		for (T* p = start; p != finish; ++p) {
			p->~T();
		}
		operator delete(start);
		start = new_start;
		finish = start + old_size;
		end_of_storage = start + new_capacity;
	}
	void checkCapacity() {
		if (finish == end_of_storage) {
			size_t old_capacity = static_cast<size_t>(end_of_storage - start);
			if (old_capacity == 0) {
				expand(1);
			} else {
				expand(old_capacity * 2);
			}
		}
	}

	/**
	 * TODO Constructs
	 * At least two: default constructor, copy constructor
	 */
public:
	vector() : start(nullptr), finish(nullptr), end_of_storage(nullptr){}
	vector(const vector &other) {
		size_t n = other.size();
		if (n == 0) {
			start = finish = end_of_storage = nullptr;
			return;
		}
		start = static_cast<T*>(operator new(n * sizeof(T)));
		for (int i = 0; i < n; ++i) {
			new (start + i) T(other.start[i]);
		}
		finish = end_of_storage = start + n;
	}
	/**
	 * TODO Destructor
	 */
	~vector() {
		for (T* p = start; p != finish; ++p) {
			p->~T();
		}
		operator delete(start);
	}
	/**
	 * TODO Assignment operator
	 */
	vector &operator=(const vector &other) {
		if (this == &other) {
			return *this;
		}

		for (T* p = start; p != finish; ++p) {
			p->~T();
		}
		operator delete(start);

		size_t n = other.size();
		if (n == 0) {
			start = finish = end_of_storage = nullptr;
			return *this;
		}
		start = static_cast<T*>(operator new(n * sizeof(T)));
		for (size_t i = 0; i < n; ++i) {
			new (start + i) T(other.start[i]);
		}
		finish = end_of_storage = start + n;
		return *this;
	}
	/**
	 * assigns specified element with bounds checking
	 * throw index_out_of_bound if pos is not in [0, size)
	 */
	T & at(const size_t &pos) {
		checkRange(pos);
		return start[pos];
	}
	const T & at(const size_t &pos) const {
		checkRange(pos);
		return start[pos];
	}
	/**
	 * assigns specified element with bounds checking
	 * throw index_out_of_bound if pos is not in [0, size)
	 * !!! Pay attentions
	 *   In STL this operator does not check the boundary but I want you to do.
	 */
	T & operator[](const size_t &pos) {
		return at(pos);
	}
	const T & operator[](const size_t &pos) const {
		return at(pos);
	}
	/**
	 * access the first element.
	 * throw container_is_empty if size == 0
	 */
	const T & front() const {
		checkEmpty();
		return *start;
	}
	/**
	 * access the last element.
	 * throw container_is_empty if size == 0
	 */
	const T & back() const {
		checkEmpty();
		return *(finish - 1);
	}
	/**
	 * returns an iterator to the beginning.
	 */
	iterator begin() {
		return iterator(start, this);
	}
	const_iterator begin() const {
		return const_iterator(start, this);
	}
	const_iterator cbegin() const {
		return const_iterator(start, this);
	}
	/**
	 * returns an iterator to the end.
	 */
	iterator end() {
		return iterator(finish, this);
	}
	const_iterator end() const {
		return const_iterator(finish, this);
	}
	const_iterator cend() const {
		return const_iterator(finish, this);
	}
	/**
	 * checks whether the container is empty
	 */
	bool empty() const {
		return size() == 0;
	}
	/**
	 * returns the number of elements
	 */
	size_t size() const {
		return finish - start;
	}
	/**
	 * clears the contents
	 */
	void clear() {
		for (T* p = start; p != finish; ++p) {
			p->~T();
		}
		finish = start;
	}
	/**
	 * inserts value before pos
	 * returns an iterator pointing to the inserted value.
	 */
	iterator insert(iterator pos, const T &value) {
		size_t index = static_cast<size_t>(pos.p - start);
		return insert(index, value);
	}
	/**
	 * inserts value at index ind.
	 * after inserting, this->at(ind) == value
	 * returns an iterator pointing to the inserted value.
	 * throw index_out_of_bound if ind > size (in this situation ind can be size because after inserting the size will increase 1.)
	 */
	iterator insert(const size_t &ind, const T &value) {
		if (ind > size()) {
			throw index_out_of_bound();
		}
		checkCapacity();
		size_t old_size = size();
		if (ind < old_size) {
			new (finish) T(start[old_size - 1]);
			for (size_t i = old_size - 1; i > ind; --i) {
				start[i] = start[i - 1];
			}
			start[ind] = value;
		} else {
			new (finish) T(value);
		}
		++finish;
		return iterator(start + ind, this);
	}
	/**
	 * removes the element at pos.
	 * return an iterator pointing to the following element.
	 * If the iterator pos refers the last element, the end() iterator is returned.
	 */
	iterator erase(iterator pos) {
		size_t index = static_cast<size_t>(pos.p - start);
		return erase(index);
	}
	/**
	 * removes the element with index ind.
	 * return an iterator pointing to the following element.
	 * throw index_out_of_bound if ind >= size
	 */
	iterator erase(const size_t &ind) {
		if (ind >= size()) {
			throw index_out_of_bound();
		}
		for (size_t i = ind; i < size() - 1; ++i) {
			start[i] = start[i + 1];
		}
		--finish;
		finish->~T();
		return iterator(start + ind, this);
	}
	/**
	 * adds an element to the end.
	 */
	void push_back(const T &value) {
		checkCapacity();
		new (finish) T(value);
		++finish;
	}
	/**
	 * remove the last element from the end.
	 * throw container_is_empty if size() == 0
	 */
	void pop_back() {
		if (size() == 0) {
			throw container_is_empty();
		}
		--finish;
		finish->~T();
	}
};


}

#endif