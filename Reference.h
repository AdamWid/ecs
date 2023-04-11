#pragma once
#include "Entity.h"
#include "Assert.h"
#include <cassert>

namespace ecs
{
	template <typename T>
	class Container;

	template <typename T>
	class Reference
	{
	public:
		Reference() : myContainer(nullptr), myEntity(ecs::nullentity) {}
		Reference(Entity aEntity, Container<T>* aContainer) : myEntity(aEntity), myContainer(aContainer)
		{}
		Reference(const Reference&) = default;
		Reference& operator=(const Reference&) = default;
		Reference(Reference&& aReference) noexcept : myEntity(aReference.myEntity), myContainer(aReference.myContainer)
		{
			aReference.myEntity = ecs::nullentity;
			aReference.myContainer = nullptr;
		}
		Reference& operator=(Reference&& aReference) noexcept
		{
			myEntity = aReference.myEntity;
			aReference.myEntity = ecs::nullentity;
			myContainer = aReference.myContainer;
			aReference.myContainer = nullptr;
			return *this;
		}


		bool Valid() const
		{
			return myContainer && myEntity != nullentity && myContainer->Contains(myEntity);
		}
		
		operator bool() const
		{
			return Valid();
		}

		T& operator*()
		{
			ECS_ASSERT(Valid() && "reference invalid");
			return myContainer->Get(myEntity);
		}

		const T& operator*() const
		{
			ECS_ASSERT(Valid() && "reference invalid");
			return myContainer->Get(myEntity);
		}

		T* operator->()
		{
			ECS_ASSERT(Valid() && "reference invalid");
			return &myContainer->Get(myEntity);
		}

		const T* operator->() const
		{
			ECS_ASSERT(Valid() && "reference invalid");
			return &myContainer->Get(myEntity);
		}

		Entity GetEntity() const
		{
			ECS_ASSERT(Valid() && "reference invalid");
			return myEntity;
		}

		bool operator==(const Reference& aOther) const
		{
			return myEntity == aOther.myEntity && myContainer == aOther.myContainer;
		}

	private:
		Entity myEntity;
		Container<T>* myContainer;
	};
}