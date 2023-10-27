/*
* Basic camera class
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Frustum.hpp"

namespace engine
{
	namespace scene
	{
		class Camera
		{
			float m_fov;
			float m_znear, m_zfar;

			glm::vec3 m_SpherePosition = glm::vec3();
			float m_SphereRadius = 1.0f;
			float m_theta = 0.0f;
			float m_phi = 0.0f;

			Frustum m_frustum;
		public:
			void updateViewMatrix(glm::mat4 externalmat = glm::mat4(1.0f))
			{
				glm::mat4 rotM = glm::mat4(1.0f);
				glm::mat4 transM;

				if (subtype == Camera::normal)
				{
					rotM = glm::rotate(rotM, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
					rotM = glm::rotate(rotM, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
					rotM = glm::rotate(rotM, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
					transM = glm::translate(glm::mat4(1.0f), position);
					if (type == CameraType::firstperson)
					{
						matrices.view = rotM * transM;
					}
					else
					{
						matrices.view = transM * rotM;
					}
				}
				else
				{
					rotM = glm::rotate(rotM, float(-m_phi), glm::vec3(0.0f, 1.0f, 0.0f));
					rotM = glm::rotate(rotM, float(-m_theta), glm::vec3(0.0f, 0.0f, 1.0f));

					camFront.x = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
					camFront.y = sin(glm::radians(rotation.x));
					camFront.z = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));

					camWorldFront = rotM * glm::vec4(glm::normalize(camFront), 0.0);

					glm::vec3 WorldUp = -glm::normalize(position);
					glm::vec3 Right = glm::normalize(glm::cross(camWorldFront, WorldUp));
					glm::vec3 Up = glm::normalize(glm::cross(Right, camWorldFront));
					lookatpoint = position + camWorldFront;
					matrices.view = glm::lookAt(position, lookatpoint, Up);
				}
				
				matrices.view = matrices.view * externalmat;

				updated = true;
			};
		public:
			enum CameraType { lookat, firstperson };
			enum CameraSubType { normal, surface };
			CameraType type = CameraType::lookat;
			CameraSubType subtype = CameraSubType::normal;

			glm::vec3 rotation = glm::vec3();
			glm::vec3 position = glm::vec3();
			glm::vec3 camFront;
			glm::vec3 camWorldFront;
			glm::vec3 lookatpoint;

			float rotationSpeed = 1.0f;
			float movementSpeed = 1.0f;

			bool updated = false;

			void ToSphere(glm::vec3 point)
			{
				m_SphereRadius = sqrt(point.x * point.x + point.y * point.y + point.z * point.z);
				m_theta = acos(point.y / m_SphereRadius);
				m_phi = atan2(point.z, point.x);
			}

			glm::vec3 ToSphereV(glm::vec3 point)
			{				
				float radius = sqrt(point.x * point.x + point.y * point.y + point.z * point.z);
				//std::cout << "a = " << point.y / radius << " b = " << acos(point.y / radius) << std::endl;
				return glm::vec3(radius,
				 acos(point.y / radius),
				 atan2(point.z, point.x));
			}

			glm::vec3 ToCartesian()
			{
				
				return glm::vec3(m_SphereRadius * sin(m_theta) * cos(m_phi),
					m_SphereRadius * cos(m_theta),
					m_SphereRadius * sin(m_theta) * sin(m_phi)
					);				
			}

			struct
			{
				glm::mat4 perspective;
				glm::mat4 view;
			} matrices;

			struct
			{
				bool left = false;
				bool right = false;
				bool up = false;
				bool down = false;
			} keys;

			glm::vec3 GetWorldPosition()
			{
				return -matrices.view[3];
			}

			bool moving()
			{
				return keys.left || keys.right || keys.up || keys.down;
			}

			float getNearClip() {
				return m_znear;
			}

			float getFarClip() {
				return m_zfar;
			}

			void setPerspective(float fov, float aspect, float znear, float zfar)
			{
				this->m_fov = fov;
				this->m_znear = znear;
				this->m_zfar = zfar;
				matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
			};

			void updateAspectRatio(float aspect)
			{
				matrices.perspective = glm::perspective(glm::radians(m_fov), aspect, m_znear, m_zfar);
			}

			void setPosition(glm::vec3 position)
			{
				this->position = position;
				updateViewMatrix();
			}

			void setRotation(glm::vec3 rotation)
			{
				this->rotation = rotation;
				updateViewMatrix();
			};

			void rotate(glm::vec3 delta)
			{
				this->rotation += delta;
				updateViewMatrix();
			}

			void setTranslation(glm::vec3 translation)
			{
				this->position = translation;
				if(type == surface)
				ToSphere(translation);
				updateViewMatrix();
			};
			void setTranslationOnSphere(float theta, float phi, float radius)
			{
				m_theta = theta;
				m_phi = phi;
				m_SphereRadius = radius;
				this->position = ToCartesian();
				updateViewMatrix();
			};

			void translate(glm::vec3 delta)
			{
				this->position += delta;
				updateViewMatrix();
			}
			void translateSphere(glm::vec3 delta)
			{
				this->m_SphereRadius += delta.x;
				this->m_theta += delta.y;
				this->m_phi += delta.z;
				position = ToCartesian();
				updateViewMatrix();
			}
			

			void update(float deltaTime)
			{
				updated = false;
				if (type == CameraType::firstperson)
				{
					if (moving())
					{
						//glm::vec3 camFront;
						/*camFront.x = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
						camFront.y = sin(glm::radians(rotation.x));
						camFront.z = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));
						camFront = glm::normalize(camFront);*/

						float moveSpeed = deltaTime * movementSpeed;

						if (keys.up)
						{
							if(subtype != CameraSubType::surface)
							position += camFront * moveSpeed;
							else
							{
								glm::vec3 lookatpoint2 = position + camWorldFront * moveSpeed;
								//std::cout << "otheta = " << m_theta << " ophi = " << m_phi << std::endl;
								//std::cout << "x = " << position.x << " y = " << position.y << " z = " << position.z << "  ";
								//std::cout << "lx = " << lookatpoint2.x << " ly = " << lookatpoint2.y << " lz = " << lookatpoint2.z << "  ";
								glm::vec3 sc = ToSphereV(lookatpoint2);
								//std::cout << "theta = " << sc.y << " phi = " << sc.z << std::endl;
								m_theta = sc.y;
								m_phi = sc.z;
								//m_theta -= moveSpeed * 0.001;
								position = ToCartesian();
							}
						}
						if (keys.down)
						{
							if (subtype != CameraSubType::surface)
							position -= camFront * moveSpeed;
							else
							{
								glm::vec3 lookatpoint2 = position - camWorldFront * moveSpeed;
								glm::vec3 sc = ToSphereV(lookatpoint2);
								m_theta = sc.y;
								m_phi = sc.z;
								//m_theta -= moveSpeed * 0.001;
								position = ToCartesian();
							}
						}
						if (keys.left)
						{
							if (subtype != CameraSubType::surface)
							position -= glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
							else
							{
								m_phi += moveSpeed * 0.001;
								position = ToCartesian();
							}
						}
						if (keys.right)
						{
							if (subtype != CameraSubType::surface)
							position += glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
							else
							{
								m_phi -= moveSpeed * 0.001;
								position = ToCartesian();
							}
						}

						updateViewMatrix();
					}
				}
			};

			// Update camera passing separate axis data (gamepad)
			// Returns true if view or position has been changed
			bool updatePad(glm::vec2 axisLeft, glm::vec2 axisRight, float deltaTime)
			{
				bool retVal = false;

				if (type == CameraType::firstperson)
				{
					// Use the common console thumbstick layout		
					// Left = view, right = move

					const float deadZone = 0.0015f;
					const float range = 1.0f - deadZone;

					
					camFront.x = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
					camFront.y = sin(glm::radians(rotation.x));
					camFront.z = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));
					camFront = glm::normalize(camFront);

					float moveSpeed = deltaTime * movementSpeed * 2.0f;
					float rotSpeed = deltaTime * rotationSpeed * 50.0f;

					// Move
					if (fabsf(axisLeft.y) > deadZone)
					{
						float pos = (fabsf(axisLeft.y) - deadZone) / range;
						position -= camFront * pos * ((axisLeft.y < 0.0f) ? -1.0f : 1.0f) * moveSpeed;
						retVal = true;
					}
					if (fabsf(axisLeft.x) > deadZone)
					{
						float pos = (fabsf(axisLeft.x) - deadZone) / range;
						position += glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * pos * ((axisLeft.x < 0.0f) ? -1.0f : 1.0f) * moveSpeed;
						retVal = true;
					}

					// Rotate
					if (fabsf(axisRight.x) > deadZone)
					{
						float pos = (fabsf(axisRight.x) - deadZone) / range;
						rotation.y += pos * ((axisRight.x < 0.0f) ? -1.0f : 1.0f) * rotSpeed;
						retVal = true;
					}
					if (fabsf(axisRight.y) > deadZone)
					{
						float pos = (fabsf(axisRight.y) - deadZone) / range;
						rotation.x -= pos * ((axisRight.y < 0.0f) ? -1.0f : 1.0f) * rotSpeed;
						retVal = true;
					}
				}
				else
				{
					// todo: move code from example base class for look-at
				}

				if (retVal)
				{
					updateViewMatrix();
				}

				return retVal;
			}

			/*glm::vec3 *GetFrustum()
			{
				glm::vec3 frustumCorners[8] = {
						glm::vec3(-1.0f,  1.0f, -1.0f),
						glm::vec3(1.0f,  1.0f, -1.0f),
						glm::vec3(1.0f, -1.0f, -1.0f),
						glm::vec3(-1.0f, -1.0f, -1.0f),
						glm::vec3(-1.0f,  1.0f,  1.0f),
						glm::vec3(1.0f,  1.0f,  1.0f),
						glm::vec3(1.0f, -1.0f,  1.0f),
						glm::vec3(-1.0f, -1.0f,  1.0f),
				};

				glm::mat4 invCam = glm::inverse(matrices.perspective * matrices.view);
				for (uint32_t i = 0; i < 8; i++) {
					glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
					frustumCorners[i] = invCorner / invCorner.w;
				}

				return frustumCorners;
			}*/

			Frustum* GetFrustum()
			{
				m_frustum.update(matrices.perspective * matrices.view);
				return &m_frustum;
			}

		};
	}
}