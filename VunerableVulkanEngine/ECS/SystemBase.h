#pragma once

namespace ECS
{
	// TODO: need to consider multithread or job system in the future...
	class SystemBase
	{
	public:
		virtual ~SystemBase()
		{
		}

	public:
		virtual void OnInitialize() = 0;
		virtual void OnExecute() = 0;

	public:
		uint32_t GetIdentifier()
		{
			return m_Identifier;
		}

		void SetIdentifier(uint32_t identifier)
		{
			m_Identifier = identifier;
		}

		uint32_t GetPriority()
		{
			return m_Priority;
		}

		void SetPriority(uint32_t priority)
		{
			m_Priority = priority;
		}

	protected:
		uint32_t m_Identifier;
		uint32_t m_Priority;		
	};
}