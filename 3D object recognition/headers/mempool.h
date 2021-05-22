#pragma once

#ifndef MEMPOOL_H_
#define MEMPOOL_H_

#include <vector>
#include <algorithm>

#define POOL_MEMBLOCK 4088


class PoolAlloc {
private:
	size_t itemsize;
	void *freelist;
	void grow_freelist()
	{
		size_t n = POOL_MEMBLOCK / itemsize;
		freelist = ::operator new(n * itemsize);
		for (size_t i = 0; i < n-1; i++)
			*(void **)((char *)freelist + itemsize*i) =
					(char *)freelist + itemsize*(i+1);
		*(void **)((char *)freelist + itemsize*(n-1)) = 0;
	}

public:
	PoolAlloc(size_t size) : itemsize(size), freelist(0) {}
	void *alloc(size_t n)
	{
		if (n != itemsize)
			return ::operator new(n);
		if (!freelist)
			grow_freelist();
		void *next = freelist;
		freelist = *(void **)next;
		return next;
	}
	void free(void *p, size_t n)
	{
		if (!p)
			return;
		else if (n != itemsize)
			::operator delete(p);
		else {
			*(void **)p = freelist;
			freelist = p;
		}
	}
	void sort_freelist()
	{
		if (!freelist)
			return;
		std::vector<void *> v;
		void *p;
		for (p = freelist; *(void **)p; p = *(void **)p)
			v.push_back(p);
		std::sort(v.begin(), v.end());
		p = freelist = v[0];
		for (size_t i = 1; i < v.size(); i++) {
			*(void **)p = v[i];
			p = *(void **)p;
		}
		*(void **)p = NULL;
	}
};

#endif /* MEMPOOL_H_ */

