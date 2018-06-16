#pragma once
class GraphicsManager
{
	public:
		GraphicsManager(float x, float y);
		~GraphicsManager();
		void InitD3D(HWND hWnd);
		void EndD3D();
		void RenderFrame();
		void InitShaders();
		void CreateVertBuffer();
		void CreateConstBuffer();
		void UpdateConstBuffer();
		float deltaTime;
		float timeElapsed;
};