#pragma once

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

			Frustum m_frustum;

			float flipY = false;

			glm::vec3 m_rotation = glm::vec3();
			glm::vec3 m_position = glm::vec3();
			glm::vec3 m_direction;
			glm::vec3 m_worldDirection;
			//glm::vec3 lookatpoint;

			glm::vec3 m_SpherePosition = glm::vec3();
			float m_SphereRadius = 1.0f;
			float m_theta = 0.0f;
			float m_phi = 0.0f;

			struct
			{
				glm::mat4 perspective;
				glm::mat4 view;
				glm::mat4 viewold;
			} matrices;

			

		public:

			struct
			{
				bool left = false;
				bool right = false;
				bool up = false;
				bool down = false;
			} keys;

			float movementSpeed = 1.0f;
			enum CameraType { normal, surface };
			CameraType type = CameraType::normal;

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

			float getFOV()
			{
				return m_fov;
			}
			const glm::vec3 GetPosition() const { return m_position; }
			const glm::mat4 GetViewMatrix() const  { return matrices.view; }
			const glm::mat4 GetOldViewMatrix() const { return matrices.viewold; }
			const glm::mat4 GetPerspectiveMatrix() const { return matrices.perspective; }

			Frustum* GetFrustum();

			void SetFlipY(float value)
			{
				flipY = value;
			}

			void SetPerspective(float fov, float aspect, float znear, float zfar);
			void UpdateAspectRatio(float aspect);
			void UpdateViewMatrix(glm::mat4 externalmat = glm::mat4(1.0f));

			void SetPosition(glm::vec3 position);
			void SetPositionOnSphere(float theta, float phi, float radius);
			void Translate(glm::vec3 delta);
			void TranslateOnSphere(glm::vec3 delta);
			void SetRotation(glm::vec3 rotation);
			void Rotate(glm::vec3 delta);

			void ToSphere(glm::vec3 point);
			glm::vec3 ToSphereV(glm::vec3 point);
			glm::vec3 ToCartesian();

			void Update(float deltaTime);
			
		};
	}
}

