#pragma once
#include "Core/Layer.h"
#include "Physics/ShapeType.h"
#include "raylib.h"

class EditorLayer : public Mupfel::Layer
{
private:
	void OnInit() override;
	void OnUpdate(double timestep) override;
	void OnRender() override;
	void ProcessEvents();
	void CreateNewEntity();
	void DrawCircleCreator(Vector2 anchor);
	void DrawAABBCreator(Vector2 anchor);
	void DrawPreview(Vector2 anchor);
	void DrawRects();
private:
	bool DropdownBox000EditMode = false;
	int DropdownBox000Active = 0;
	ShapeType current_shape = ShapeType::Circle;

	float current_pos_x = 0.0f;
	float current_pos_y = 0.0f;
	float initial_pos_x = 0.0f;
	float initial_pos_y = 0.0f;
	float initial_velocity_x = 0.0f;
	float initial_velocity_y = 0.0f;
	float current_velocity_x = 0.0f;
	float current_velocity_y = 0.0f;
	float current_scale = 32.0f;
	float collider_size_x = 16.0f;
	float collider_size_y = 16.0f;
};

