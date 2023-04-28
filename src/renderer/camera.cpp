#include "camera.h"

#include <cmath>
#include <iostream>

#include "glm/gtc/matrix_transform.hpp"


// initial values
constexpr glm::vec3 g_CameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
constexpr glm::vec3 g_CameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
constexpr glm::vec3 g_CameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
constexpr glm::vec3 g_Target = glm::vec3(0.0f, 0.0f, 0.0f);
constexpr float g_Yaw = -90.0f;
constexpr float g_Pitch = 0.0f;
constexpr float g_FOVy = glm::radians(45.0f);
constexpr float g_ZNear = 0.1f;
constexpr float g_ZFar = 50.0f;


Camera::Camera(float aspectRatio)
	: m_CameraPos{g_CameraPos},
	  m_CameraFront{g_CameraFront},
	  m_CameraUp{g_CameraUp},
	  m_Target{g_Target},
	  m_Yaw{g_Yaw}, m_Pitch{g_Pitch},
	  m_LastX{0.0f}, m_LastY{0.0f},
	  m_FOVy{g_FOVy},
	  m_ZNear{g_ZNear},
	  m_ZFar{g_ZFar},
	  m_ViewMatrix{},
	  m_ProjectionMatrix{}
{

}

void Camera::OnUpdate(GLFWwindow* window, float deltaTime, uint32_t width, uint32_t height)
{
	m_ViewMatrix = glm::lookAt(m_CameraPos, m_CameraPos + m_CameraFront, m_CameraUp);
	m_ProjectionMatrix = glm::perspective(m_FOVy, width / static_cast<float>(height), m_ZNear, m_ZFar);
	m_ProjectionMatrix[1][1] *= -1; // glm was designed for opengl where the y-coord for clip coordinate is flipped

	// movement
	const float cameraSpeed = 5.0f * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) // forward
		m_CameraPos += cameraSpeed * m_CameraFront;
	else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) // backward
		m_CameraPos -= cameraSpeed * m_CameraFront;

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) // left
		m_CameraPos -= cameraSpeed * (glm::normalize(glm::cross(m_CameraFront, m_CameraUp)));
	else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) // right
		m_CameraPos += cameraSpeed * (glm::normalize(glm::cross(m_CameraFront, m_CameraUp)));

	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) // up
	{
		const glm::vec3 rightVec = glm::cross(m_CameraFront, m_CameraUp);
		const glm::vec3 upVec    = glm::cross(rightVec, m_CameraFront);
		m_CameraPos += cameraSpeed * glm::normalize(upVec);
	}
	else if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) // down
	{
		const glm::vec3 rightVec = glm::cross(m_CameraFront, m_CameraUp);
		const glm::vec3 upVec    = glm::cross(rightVec, m_CameraFront);
		m_CameraPos -= cameraSpeed * glm::normalize(upVec);
	}

	// reset camera
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
	{
		m_FirstMouseMove = true;
		m_CameraPos = g_CameraPos;
		m_CameraFront = g_CameraFront;
		m_CameraUp = g_CameraUp;
		m_Target = g_Target;
		m_Yaw = g_Yaw;
		m_Pitch = g_Pitch;
		m_LastX = 0.0f;
		m_LastY = 0.0f;
	}
}

void Camera::OnMouseMove(GLFWwindow* window, double xpos, double ypos)
{
	// initial values: m_Yaw = -90.0f, m_Pitch = 0.0f
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) != GLFW_PRESS) // only move the camera on mouse button click
	{
		m_FirstMouseMove = true;
		return;
	}

	if (m_FirstMouseMove)
	{
		m_LastX = xpos;
		m_LastY = ypos;
		m_FirstMouseMove = false;
	}

	const float sensitivity = 0.1f;
	float xOffset = (xpos - m_LastX) * sensitivity;
	float yOffset = (ypos - m_LastY) * sensitivity;

	m_LastX = xpos;
	m_LastY = ypos;

	m_Yaw += xOffset;
	m_Pitch -= yOffset; // negative because the y coord is flupped in projection matrix

	if (m_Pitch > 89.0f)
		m_Pitch = 89.0f;
	if (m_Pitch < -89.0f)
		m_Pitch = -89.0f;

	if (m_Yaw > 359.0f || m_Yaw < -359.0f)
		m_Yaw = 0.0f;

	glm::vec3 direction;
	direction.x = std::cosf(glm::radians(m_Yaw)) * std::cosf(glm::radians(m_Pitch));
	direction.y = std::sinf(glm::radians(m_Pitch));
	direction.z = std::sinf(glm::radians(m_Yaw)) * std::cosf(glm::radians(m_Pitch));
	m_CameraFront = glm::normalize(direction);
}

// BUG: doesnt work for some reason
void Camera::Orbit(GLFWwindow* window, double xpos, double ypos)
{
	// initial value: m_Yaw = 90.0f, m_Pitch = 90.0f
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) != GLFW_PRESS) // only move the camera on mouse button click
	{
		m_FirstMouseMove = true;
		return;
	}

	if (m_FirstMouseMove)
	{
		m_LastX = xpos;
		m_LastY = ypos;
		m_FirstMouseMove = false;
	}

	const float sensitivity = 0.5f;
	float xOffset = (xpos - m_LastX) * sensitivity;
	float yOffset = (ypos - m_LastY) * sensitivity;

	m_LastX = xpos;
	m_LastY = ypos;

	m_Yaw += xOffset;
	m_Pitch += yOffset;

	if (m_Pitch > 179.0f)
		m_Pitch = 179.0f;
	else if (m_Pitch < 1.0f)
		m_Pitch = 1.0f;

	if (m_Yaw > 359.0f || m_Yaw < -359.0f)
		m_Yaw = 0.0f;


	const float radius = std::sqrtf((m_Target.x - m_CameraPos.x) * (m_Target.x - m_CameraPos.x) +
									(m_Target.y - m_CameraPos.y) * (m_Target.y - m_CameraPos.y) +
									(m_Target.z - m_CameraPos.z) * (m_Target.z - m_CameraPos.z));
	glm::vec3 position;
	position.x = m_Target.x + radius * std::sinf(glm::radians(m_Pitch)) * std::cosf(glm::radians(m_Yaw));
	position.y = m_Target.y + radius * std::cosf(glm::radians(m_Pitch));
	position.z = m_Target.z + radius * std::sinf(glm::radians(m_Pitch)) * std::sinf(glm::radians(m_Yaw));
	m_CameraPos = position;
}
