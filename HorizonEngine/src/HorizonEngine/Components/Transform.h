#pragma once
#ifndef HZN_TRANSFORM_H
#define HZN_TRANSFORM_H

#include "Component.h"
#include "HorizonEngine/GameObject.h"
#include "HorizonEngine/Utils/Time.h"
#include "HorizonEngine/Utils/Math.h"
#include "glm/vec2.hpp"

namespace Hzn {

	class Transform;
	class GameObject;

	class Transform : public Component {
	public:
		std::vector<ComponentType> componentTypes{ ComponentType::C_Transform };
		GameObject* gameObject;

		glm::vec2 localPosition = glm::vec2(0, 0);
		float localRotation = 0;
		glm::vec2 localScale = glm::vec2(0, 0);

		glm::vec2 position = glm::vec2(0, 0);
		float rotation = 0;
		glm::vec2 scale = glm::vec2(0, 0);

		glm::vec2 right = glm::vec2(1, 0);
		glm::vec2 up = glm::vec2(0, 1);

		std::weak_ptr<Transform> parent;
		std::weak_ptr<Transform> root;
		int siblingIndex = 0;
		int childCount = 0;

		std::vector<Transform*>* children{};

		Transform();
		~Transform();
	private:
		void awake();
		void start();
		void update();
		void fixedUpdate();
	public:
		// Moves the transform in direction direction by delta units
		void move(glm::vec2 direction, float delta);
		// Moves the transform towards position by delta units
		void moveTowards(glm::vec2 position, float delta);

		// Sets the transform's position to position
		void translate(glm::vec2 position);
		// Sets the transform's position to vec2(x, y)
		void translate(float x, float y);

		// Rotates the transform degrees amount degrees
		void rotate(float angle);

		// Rotates the transform so that the forward vec2 is pointing towards position
		void lookAt(glm::vec2 targetPosition);
		// Rotates the transform so that the forward vec2 is pointing towards target transform
		void lookAt(Transform* target);


		// void detachChildren();
		Transform* getChildByName(char* name);
		Transform* getChildByIndex(int index);
		bool isChildOf(char* name);
		void setAsFirstSibling();
		void setAsLastSibling();
		void setSiblingIndex(int newSiblingIndex);
	};
}

#endif