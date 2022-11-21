#include "camera.h"

#include "glm/gtc/matrix_transform.hpp"


Camera::Camera(float aspectRatio)
	: m_CameraPos(glm::vec3(0.0f, 0.0f, 3.0f)), 
	  m_CameraFront(glm::vec3(0.0f, 0.0f, -1.0f)), 
	  m_CameraUp(glm::vec3(0.0f, 1.0f, 0.0f)), 
	  m_Yaw(-90.0f), m_Pitch(0.0f), 
	  m_LastX(0.0f), m_LastY(0.0f),
	  m_FOVy(glm::radians(45.0f)),
	  m_ZNear(1.0f),
	  m_ZFar(10.0f),
	  m_ViewMatrix{},
	  m_ProjectionMatrix{}
{

}

void Camera::OnUpdate(GLFWwindow* window, float deltaTime, uint32_t width, uint32_t height)
{
	m_ViewMatrix = glm::lookAt(m_CameraPos, m_CameraPos + m_CameraFront, m_CameraUp);
	m_ProjectionMatrix = glm::perspective(m_FOVy, width / (float)height, m_ZNear, m_ZFar);

	// movement
	const float cameraSpeed = 2.0f * deltaTime;
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
		m_CameraPos -= cameraSpeed * glm::normalize(upVec); // minus becuause y is flipped
	}
	else if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) // down
	{
		const glm::vec3 rightVec = glm::cross(m_CameraFront, m_CameraUp);
		const glm::vec3 upVec    = glm::cross(rightVec, m_CameraFront);
		m_CameraPos += cameraSpeed * glm::normalize(upVec);
	}
	
	// unhide cursor when camera stops moving
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_RELEASE)
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void Camera::OnMouseMove(GLFWwindow* window, double xpos, double ypos)
{
	static bool firstMouse = true;
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) != GLFW_PRESS) // only move the camera on mouse button click
	{
		firstMouse = true;
		return;
	}

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // hide cursor when moving camera
	
	if (firstMouse)
	{
		m_LastX = xpos;
		m_LastY = ypos;
		firstMouse = false;
	}

	const float sensitivity = 0.05f; 
	float xOffset = (xpos - m_LastX) * sensitivity;
	float yOffset = (ypos - m_LastY) * sensitivity;

	m_LastX = xpos;
	m_LastY = ypos;

	m_Yaw += xOffset;
	m_Pitch += yOffset;

	if (m_Pitch > 89.0f)
		m_Pitch = 89.0f;
	if (m_Pitch < -89.0f)
		m_Pitch = -89.0f;

	glm::vec3 direction;
	direction.x = std::cosf(glm::radians(m_Yaw)) * std::cosf(glm::radians(m_Pitch));
	direction.y = std::sinf(glm::radians(m_Pitch));
	direction.z = std::sinf(glm::radians(m_Yaw)) * std::cosf(glm::radians(m_Pitch));
	m_CameraFront = glm::normalize(direction);
}
