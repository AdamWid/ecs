#pragma once


#define ECS_ASSERT_ENABLED
#define ECS_VALIDATE_ENTITIES



#ifdef ECS_ASSERT_ENABLED

#include <cassert>
#define ECS_ASSERT(expression) assert(expression)

#else
#define ECS_ASSERT(expression)

#endif // !ECS_ASSERT_ENABLED

#ifdef ECS_VALIDATE_ENTITIES
#define ECS_ASSERT_VALID_ENTITY(expression) ECS_ASSERT(expression)
#else
#define ECS_ASSERT_VALID_ENTITY(expression)
#endif
