#pragma once

namespace mys
{
	template <typename T>
	struct Greater
	{
		inline constexpr bool operator()(const T& aLhs, const T& aRhs) const
		{
			return aRhs < aLhs;
		}
	};

	template <typename T>
	struct Less
	{
		inline constexpr bool operator()(const T& aLhs, const T& aRhs) const
		{
			return aLhs < aRhs;
		}
	};

	template <typename T>
	struct Equal
	{
		inline constexpr bool operator()(const T& aLhs, const T& aRhs) const
		{
			return !(aLhs < aRhs) && !(aRhs < aLhs);
		}
	};

	template <typename T, typename Comparator = Greater<T>>
	class Heap
	{
	public:
		Heap() : heap(nullptr), size(0), capacity(0)
		{}

		~Heap()
		{
			delete[] heap;
		}

		Heap(Heap&& anOther) : capacity(anOther.capacity), size(anOther.size), heap(anOther.heap)
		{
			anOther.heap = 0;
			anOther.capacity = 0;
			anOther.size = 0;
		}

		void Clear()
		{
			delete[] heap;
			heap = nullptr;
			size = 0;
			capacity = 0;
		}

		Heap(const Heap& anOther) : capacity(anOther.capacity), size(anOther.size), heap(new T[anOther.capacity])
		{
			std::copy(anOther.heap, anOther.heap + size, heap);
		}

		Heap& operator=(const Heap& anOther)
		{
			
			capacity = anOther.capacity;

			if (size < anOther.size)
			{
				delete[] heap;
				heap = new T[capacity];	
			}

			size = anOther.size;

			std::copy(anOther.heap, anOther.heap + size, heap);

			return *this;
		}

		int Size() const
		{
			return static_cast<int>(size);
		}

		bool Contains(const T& aElement) const
		{
			if (!size)
				return false;

			for (int i = 0; i < size; ++i) // not optimal, will fix later
			{
				if (Equal<T>{}(heap[i], aElement))
					return true;
			}
			return false;

			//int index = size-1;
			//while (index > 0 && !Equal<T>{}(heap[index], aElement)) // Bubbles up element to correct location
			//{
			//	//std::swap(heap[index], heap[(index - 1) / 2]);
			//	index = (index - 1) / 2;
			//}

			//return Equal<T>{}(heap[index], aElement);
		}

		void Enqueue(const T& aElement)
		{
			if (size >= capacity)
			{
				capacity = capacity * 2 + 1;
				T* tmp = new T[capacity];
				std::move(heap, heap + size, tmp);
				delete[] heap;
				heap = tmp;
			}

			int index = size++;
			heap[index] = aElement;
			while (Comparator{}(heap[index], heap[(index - 1) / 2])) // Bubbles up element to correct location
			{
				std::swap(heap[index], heap[(index - 1) / 2]);
				index = (index - 1) / 2;
			}

		}

		const T& GetTop() const
		{
			return heap[0];
		}

		T Dequeue()
		{
			T item = heap[0];
			std::swap(heap[0], heap[--size]);

			int i = 0;

			while (true)
			{
				int left = 2 * i + 1;
				int right = 2 * i + 2;


				if (right < size)
				{
					int largest = (Comparator{}(heap[right], heap[left])) ? right : left;
					if (Comparator{}(heap[largest], heap[i]))
					{
						std::swap(heap[i], heap[largest]);
						i = largest;
						continue;
					}
				}
				else if (left < size)
				{
					if (Comparator{}(heap[left], heap[i]))
					{
						std::swap(heap[i], heap[left]);
					}
				}
				break;
			}

			return item;
		}

	private:
		int capacity;
		int size;
		T* heap;
	};
}