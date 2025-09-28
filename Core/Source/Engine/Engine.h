#pragma once

namespace Mupfel {
	/**
	 * @brief
	 */
	class Engine
	{
	public:
		Engine();
		virtual ~Engine();
		Engine(const Engine& other) = delete;
		Engine(Engine&& other) = delete;
		Engine& operator=(const Engine& other) = delete;
		Engine& operator=(Engine&& other) = delete;
	public:
		/**
		 * @brief
		 * @return
		 */
		bool Init();

		/**
		 * @brief 
		 */
		void Run();
	};
}


