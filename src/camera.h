#pragma once

#include <cstdint>

#include <GLFW/glfw3.h>
#include "glm/glm.hpp"


class Camera
{
public:
	Camera(float aspectRatio);

	void OnUpdate(GLFWwindow* window, float deltaTime, uint32_t width, uint32_t height);
	void OnMouseMove(GLFWwindow* window, double xpos, double ypos);

	inline glm::mat4 GetViewMatrix()       const { return m_ViewMatrix; }
	inline glm::mat4 GetProjectionMatrix() const { return m_ProjectionMatrix; }

private:
	glm::vec3 m_CameraPos;
	glm::vec3 m_CameraFront;
	glm::vec3 m_CameraUp;

	float m_Yaw;
	float m_Pitch;

	float m_LastX;
	float m_LastY;

	// projection matrix
	float m_FOVy;
	float m_ZNear;
	float m_ZFar;

	glm::mat4 m_ViewMatrix;
	glm::mat4 m_ProjectionMatrix;
};