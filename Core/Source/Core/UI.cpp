#include "UI.h"
#include "Renderer/Renderer.h"
#include "Application.h"

uint32_t Mupfel::UI::Button(float x, float y, float width, float height, const std::string& image_path)
{
	return Application::Get().renderer->uiRenderer->Button(x, y, width, height, image_path);
}
