#pragma once

namespace Mupfel {

	class Layer
	{
	public:
		Layer() = default;
		virtual ~Layer() = default;

		virtual void OnEvent() {}

		virtual void OnInit() {}
		virtual void OnUpdate(double timestep) {}
		virtual void OnRender() {}
	};

}

