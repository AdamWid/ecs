#include "Ecs.h"

int main(void)
{
	ecs::Registry registry;

	struct ExampleComponent
	{
		int a, b, c, d;
	};

	struct RandomComponent
	{
		int e, f, g, h;
	};

	struct ExcludeComponent
	{};


	for (int i = 0; i < 10000; ++i)
	{
		ecs::Entity entity = registry.Create();
		ExampleComponent& exampleComponent = registry.Emplace<ExampleComponent>(entity);

		exampleComponent.a = entity;

		if (rand() % 3 == 0)
		{
			RandomComponent& randomComponent = registry.Emplace<RandomComponent>(entity);
			randomComponent.e = entity;
		}
		if (rand() % 3 == 0)
		{
			registry.Emplace<ExcludeComponent>(entity);
		}
	}

	auto view = registry.View<ExampleComponent, RandomComponent>(ecs::Exclude<ExcludeComponent>());

	for (ecs::Entity entity : view)
	{
		ExampleComponent& exampleComponent = registry.Get<ExampleComponent>(entity);
		RandomComponent& randomComponent = registry.Get<RandomComponent>(entity);
		// Update
		assert(exampleComponent.a == entity);
		assert(randomComponent.e == entity);
	}

	for (auto&& [entity, exampleComponent, randomComponent] : view.Each())
	{
		// Update
		assert(exampleComponent.a == entity);
		assert(randomComponent.e == entity);
	}

	return 0;
}
