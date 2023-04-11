#pragma once
#include "Entity.h"

namespace ecs
{
	class Registry;
	class EntityIterator
	{
	public:
		EntityIterator(const Registry& aRegistry, Entity somePos, Entity aEnd);
		inline bool operator!=(const EntityIterator& aOther) const { return myPos != aOther.myPos; }
		EntityIterator& operator++();
		Entity operator*();
	private:
		const Registry& myRegistry;
		Entity myPos;
		Entity myEnd;
	};

	struct EntityIteratorWrapper
	{
		EntityIterator myStart;
		EntityIterator myEnd;

		inline EntityIterator begin() { return myStart; }
		inline EntityIterator end() { return myEnd; }
	};
}