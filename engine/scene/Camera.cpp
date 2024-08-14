#include "Camera.h"

namespace engine
{
	namespace scene
	{
		Frustum* Camera::GetFrustum()
		{
			m_frustum.update(matrices.perspective * matrices.view);
			return &m_frustum;
		}

		void Camera::SetPerspective(float fov, float aspect, float znear, float zfar)
		{
			m_fov = fov;
			m_znear = znear;
			m_zfar = zfar;
			matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
		};

		void Camera::UpdateAspectRatio(float aspect)
		{
			matrices.perspective = glm::perspective(glm::radians(m_fov), aspect, m_znear, m_zfar);
		}

		void Camera::UpdateViewMatrix(glm::mat4 externalmat)
		{
			glm::mat4 rotM = glm::mat4(1.0f);
			glm::mat4 transM;

			m_direction.x = -cos(glm::radians(m_rotation.x)) * sin(glm::radians(m_rotation.y));
			m_direction.y = sin(glm::radians(m_rotation.x));
			m_direction.z = cos(glm::radians(m_rotation.x)) * cos(glm::radians(m_rotation.y));

			if (type == Camera::normal)
			{
				rotM = glm::rotate(rotM, glm::radians(m_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
				rotM = glm::rotate(rotM, glm::radians(m_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
				rotM = glm::rotate(rotM, glm::radians(m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
				transM = glm::translate(glm::mat4(1.0f), m_position);

				matrices.view = rotM * transM;
			}
			else
			{
				rotM = glm::rotate(rotM, float(-m_phi), glm::vec3(0.0f, 1.0f, 0.0f));
				rotM = glm::rotate(rotM, float(-m_theta), glm::vec3(0.0f, 0.0f, 1.0f));

				m_worldDirection = rotM * glm::vec4(glm::normalize(m_direction), 0.0);

				glm::vec3 WorldUp = -glm::normalize(m_position);
				glm::vec3 Right = glm::normalize(glm::cross(m_worldDirection, WorldUp));
				glm::vec3 Up = glm::normalize(glm::cross(Right, m_worldDirection));
				glm::vec3 lookatpoint = m_position + m_worldDirection;
				matrices.view = glm::lookAt(m_position, lookatpoint, Up);
			}

			matrices.viewold = matrices.view;
			matrices.view = matrices.view * externalmat;
		};

		void Camera::SetPosition(glm::vec3 position)
		{
			m_position = position;
			if (type == surface)
				ToSphere(position);
			UpdateViewMatrix();
		}

		void Camera::SetPositionOnSphere(float theta, float phi, float radius)
		{
			m_theta = theta;
			m_phi = phi;
			m_SphereRadius = radius;
			m_position = ToCartesian();
			UpdateViewMatrix();
		};

		void Camera::Translate(glm::vec3 delta)
		{
			m_position += delta;
			UpdateViewMatrix();
		}

		void Camera::TranslateOnSphere(glm::vec3 delta)
		{
			m_SphereRadius += delta.x;
			m_theta += delta.y;
			m_phi += delta.z;
			m_position = ToCartesian();
			UpdateViewMatrix();
		}

		void Camera::SetRotation(glm::vec3 rotation)
		{
			m_rotation = rotation;
			UpdateViewMatrix();
		};

		void Camera::Rotate(glm::vec3 delta)
		{
			m_rotation += delta;
			UpdateViewMatrix();
		}

		void Camera::ToSphere(glm::vec3 point)
		{
			m_SphereRadius = sqrt(point.x * point.x + point.y * point.y + point.z * point.z);
			m_theta = acos(point.y / m_SphereRadius);
			m_phi = atan2(point.z, point.x);
		}

		glm::vec3 Camera::ToSphereV(glm::vec3 point)
		{
			float radius = sqrt(point.x * point.x + point.y * point.y + point.z * point.z);
			return glm::vec3(radius,
				acos(point.y / radius),
				atan2(point.z, point.x));
		}

		glm::vec3 Camera::ToCartesian()
		{

			return glm::vec3(m_SphereRadius * sin(m_theta) * cos(m_phi),
				m_SphereRadius * cos(m_theta),
				m_SphereRadius * sin(m_theta) * sin(m_phi)
			);
		}

		void Camera::Update(float deltaTime)
		{

				if (moving())
				{
					float moveSpeed = deltaTime * movementSpeed;

					if (keys.up)
					{
						if (type != CameraType::surface)
							m_position += m_direction * moveSpeed;
						else
						{
							glm::vec3 lookatpoint2 = m_position + m_worldDirection * moveSpeed;
							glm::vec3 sc = ToSphereV(lookatpoint2);
							m_theta = sc.y;
							m_phi = sc.z;
							m_position = ToCartesian();
						}
					}
					if (keys.down)
					{
						if (type != CameraType::surface)
							m_position -= m_direction * moveSpeed;
						else
						{
							glm::vec3 lookatpoint2 = m_position - m_worldDirection * moveSpeed;
							glm::vec3 sc = ToSphereV(lookatpoint2);
							m_theta = sc.y;
							m_phi = sc.z;
							m_position = ToCartesian();
						}
					}
					if (keys.left)
					{
						if (type != CameraType::surface)
							m_position -= glm::normalize(glm::cross(m_direction, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
						else
						{
							m_phi += moveSpeed * 0.001f;
							m_position = ToCartesian();
						}
					}
					if (keys.right)
					{
						if (type != CameraType::surface)
							m_position += glm::normalize(glm::cross(m_direction, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
						else
						{
							m_phi -= moveSpeed * 0.001f;
							m_position = ToCartesian();
						}
					}

					UpdateViewMatrix();
				}	
		};		
	}
}
