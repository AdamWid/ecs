#pragma once


namespace ecs
{
	class Registry;
}

class Scene;

namespace mys
{
	class PollingStation;

	struct UpdateContext
	{
		mys::PollingStation& pollingStation;
		Scene& scene;
		ecs::Registry& registry;
		float timeDelta;

		template <typename T>
		T& Get() { return *static_cast<T*>(this); }	
		template <typename T>
		T const& Get() const { return *static_cast<const T*>(this); }
	};
}