#include "EntityIterator.h"
#include "Ecs.h"

namespace ecs
{
	EntityIterator::EntityIterator(const Registry& aRegistry, Entity somePos, Entity aEnd) : myRegistry(aRegistry), myPos(somePos), myEnd(aEnd)
	{
		while (myPos < myEnd && myRegistry.myEntityQueue.Contains(myPos))
			++myPos;
	}

	EntityIterator& EntityIterator::operator++()
	{
		++myPos;

		while (myRegistry.myEntityQueue.Contains(myPos))
			++myPos;

		return *this;
	}

	Entity EntityIterator::operator*()
	{
		return myPos;
	}
}

