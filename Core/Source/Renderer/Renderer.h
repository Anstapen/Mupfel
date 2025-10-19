#pragma once

namespace Mupfel {

	class Renderer
	{
	public:
		static void Init();
		static void Render();
		static void DeInit();
	private:
		static void ImportVertexShader();
		static void ImportFragmentShader();
		static void CreateShaderProgram();
	};

}


