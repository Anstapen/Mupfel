#pragma once

namespace Mupfel {

	class Layer
	{
	public:
		Layer() = default;
		virtual ~Layer() = default;

		virtual void OnEvent() {}

		virtual void OnInit() {}
		virtual void OnUpdate(float timestep) {}
		virtual void OnRender() {}
	};

}

