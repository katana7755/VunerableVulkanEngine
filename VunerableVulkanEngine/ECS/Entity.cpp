#include "Entity.h"

namespace ECS
{
	uint32_t		Entity::s_Next_Allocate_Identifier = 0;
	const Entity	Entity::INVALID_ENTITY;
}