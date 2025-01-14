#include "pch.h"

#include "box2d/b2_math.h"
#include "box2d/b2_world.h"
#include "box2d/b2_body.h"
#include "box2d/b2_fixture.h"
#include "box2d/b2_polygon_shape.h"
#include "box2d/b2_contact.h"

#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>

#include "HorizonEngine/Renderer/Renderer2D.h"

#include "HorizonEngine/Scene/GameObject.h"
#include "HorizonEngine/Scene/Component.h"
#include "HorizonEngine/AssetManagement/AssetManager.h"
#include "HorizonEngine/Physics2D/ContactListener.h"
#include "HorizonEngine/Scene/FunctionRegistry.h"
#include "HorizonEngine/Scripting/ScriptEngine.h"

#include "HorizonEngine/Scene/Scene.h"

namespace Hzn
{
	std::string sceneStringStorage;

	static b2BodyType toBox2DBodyType(RigidBody2DComponent::BodyType bodyType)
	{
		switch (bodyType)
		{
		case RigidBody2DComponent::BodyType::Static : return b2_staticBody;
		case RigidBody2DComponent::BodyType::Kinematic : return b2_kinematicBody;
		case RigidBody2DComponent::BodyType::Dynamic : return b2_dynamicBody;
		}

		HZN_CORE_ASSERT(false, "Unknown body type");
		return b2_staticBody;
	}

	Scene::Scene() : m_Registry(entt::registry()) {}

	Scene::Scene(cereal::JSONInputArchive& inputArchive)
		: m_Registry(entt::registry())
	{
		entt::snapshot_loader loader(m_Registry);
		loader.entities(inputArchive).component<
			NameComponent,
			RelationComponent,
			TransformComponent,
			RigidBody2DComponent,
			BoxCollider2DComponent,
			RenderComponent,
			CameraComponent,
			ScriptComponent
		>(inputArchive);

		// since all valid objects have name components we create a view on name components
		m_Valid = true;
		m_Registry.each([&](auto entity)
			{
				m_GameObjectIdMap.insert({ entt::to_integral(entity), entity });
			});
		m_Valid = false;
	}

	Scene::~Scene()
	{
		invalidate();
	}

	void Scene::ExecuteDeletionQueue()
	{
		for (auto& entity : m_DeletionQueue)
		{
			// convert entity to a game object.
			GameObject obj = { entity, this };

			if (m_State == SceneState::Play) {
				// delete physics component.
				if (obj.hasComponent<RigidBody2DComponent>())
				{
					auto& rb2d = obj.getComponent<RigidBody2DComponent>();
					b2Body* body = (b2Body*)rb2d.m_RuntimeBody;
					body->GetWorld()->DestroyBody(body);
				}
			}

			// break all relations that the game object has in the hierarchy.
			if (obj.getParent())
			{
				obj.getParent().removeChild(obj);
			}
			// remove the game object from all objects list.
			// remove object from the unordered_map.

			/*m_Objects.erase(obj.getComponent<NameComponent>());*/
			m_GameObjectIdMap.erase(entt::to_integral(obj.m_ObjectId));

			m_Registry.destroy(obj.m_ObjectId);
			obj.m_ObjectId = entt::null;
			obj.m_Scene = nullptr;
		}

		m_DeletionQueue.clear();
	}

	void Scene::serialize(cereal::JSONOutputArchive& outputArchive)
	{
		entt::snapshot{ m_Registry }
			.entities(outputArchive)
			.component<NameComponent,
			RelationComponent,
			TransformComponent,
			RigidBody2DComponent,
			BoxCollider2DComponent,
			RenderComponent,
			CameraComponent,
			ScriptComponent
			>(outputArchive);
	}

	void Scene::invalidate()
	{
		m_Registry.clear();
		m_GameObjectIdMap.clear();
		m_Valid = false;
	}

	glm::vec2 Scene::onViewportResize(int32_t width, int32_t height)
	{
		// update all the camera components on viewport resize.
		if (m_Valid)
		{
			const auto& view = m_Registry.view<CameraComponent>();

			for (const auto& entity : view)
			{
				auto& camera = view.get<CameraComponent>(entity).m_Camera;
				camera.setAspectRatio((float)width / height);
			}
		}
		m_lastViewportSize = glm::vec2(width, height);
		return m_lastViewportSize;
	}

	void Scene::onStart()
	{
		if (m_Valid)
		{
			std::ostringstream os;

			cereal::JSONOutputArchive outputArchive(os);
			// serialize data into temporary buffer.
			serialize(outputArchive);

			os << "\n}\n";
			sceneStringStorage = os.str();
			// set scene state to Playing.
			m_State = SceneState::Play;

			// create box 2D world.
			m_Listener = new ContactListener();
			m_World = new b2World({ 0.0f, -9.8f });
			m_World->SetContactListener(m_Listener);

			auto view = m_Registry.view<RigidBody2DComponent>();

			for (auto entity : view)
			{
				GameObject obj = { entity, this };
				auto transform = obj.getTransform();
				auto& transformComponent = obj.getComponent<TransformComponent>();
				auto& rb2d = obj.getComponent<RigidBody2DComponent>();

				glm::vec3 translation = glm::vec3(0.0f);
				glm::quat orientation = glm::quat();
				glm::vec3 scale = glm::vec3(0.0f);
				glm::vec3 skew = glm::vec3(0.0f);
				glm::vec4 perspective = glm::vec4(0.0f);
				glm::decompose(transform, scale, orientation, translation, skew, perspective);

				glm::vec3 rotation = glm::eulerAngles(orientation);
				rotation = glm::degrees(rotation);

				b2BodyDef bodyDef;
				bodyDef.type = (b2BodyType)rb2d.m_Type;
				bodyDef.position.Set(translation.x, translation.y);
				bodyDef.angle = glm::radians(rotation.z);
				auto v = &m_GameObjectIdMap[entt::to_integral(entity)];
				bodyDef.userData.pointer = (uintptr_t)&m_GameObjectIdMap[entt::to_integral(entity)];

				b2Body* body = m_World->CreateBody(&bodyDef);
				body->SetFixedRotation(rb2d.m_FixedRotation);
				rb2d.m_RuntimeBody = body;

				if (!obj.hasComponent<BoxCollider2DComponent>())
				{
					obj.addComponent<BoxCollider2DComponent>();
				}

				auto& bc2d = obj.getComponent<BoxCollider2DComponent>();
				b2PolygonShape polygonShape;

				polygonShape.SetAsBox(scale.x * bc2d.size.x, scale.y * bc2d.size.y);

				b2FixtureDef fixtureDef;
				fixtureDef.shape = &polygonShape;
				fixtureDef.density = bc2d.m_Density;
				fixtureDef.friction = bc2d.m_Friction;
				fixtureDef.restitution = bc2d.m_Restitution;
				fixtureDef.restitutionThreshold = bc2d.m_RestitutionThreshold;
				fixtureDef.isSensor = bc2d.m_IsSensor;
				bc2d.m_RuntimeFixture = (void*)body->CreateFixture(&fixtureDef);
			}

			// scripts
			{
				auto view = m_Registry.view<ScriptComponent>();

				for(auto entity : view)
				{
					GameObject obj = { entity, this };
					ScriptEngine::OnCreateGameObject(obj);
				}
			}
		}
	}

	void Scene::onStop()
	{
		if (m_Valid) {
			/*std::cout << m_GameObjectIdMap.size() << std::endl;*/
			// clear registries.
			m_GameObjectIdMap.clear();
			m_Registry.clear();

			// read from string storage.
			std::istringstream is(sceneStringStorage);
			cereal::JSONInputArchive inputArchive(is);
			entt::snapshot_loader loader(m_Registry);
			loader.entities(inputArchive).component<
				NameComponent,
				RelationComponent,
				TransformComponent,
				RigidBody2DComponent,
				BoxCollider2DComponent,
				RenderComponent,
				CameraComponent,
				ScriptComponent>(inputArchive);

			// update the maps.
			m_Registry.each([&](auto entity)
				{
					m_GameObjectIdMap.insert({ entt::to_integral(entity), entity });
				});

			// clear the sceneStringStorage;
			sceneStringStorage = std::string();

			// delete and set the box 2D world to nullptr.
			delete m_World;
			m_World = nullptr;

			// stop script engine.
			ScriptEngine::OnStop();
			delete m_Listener;
			m_Listener = nullptr;

			// set state back to edit.
			m_State = SceneState::Edit;
			ScriptEngine::ReloadAssembly();
		}
	}

	GameObject Scene::getActiveCamera()
	{

		auto cameras = m_Registry.view<CameraComponent>();

		GameObject activeCamera;

		for (const auto& entity : cameras)
		{
			GameObject camera = { entity, this };
			auto& cameraComponent = camera.getComponent<CameraComponent>();

			if(cameraComponent.m_Primary)
			{
				activeCamera = camera;
				break;
			}
		}

		return activeCamera;
	}

	void Scene::onEditorUpdate(OrthographicCamera& camera, TimeStep ts) {
		if (m_Valid) {
			// delete objects queued for deletion.
			ExecuteDeletionQueue();

			Renderer2D::beginScene(camera);

			auto sprites = m_Registry.view<RenderComponent, TransformComponent>();
			for (const auto& entity : sprites)
			{
				GameObject obj = { entity, this };
				auto& renderComponent = obj.getComponent<RenderComponent>();
				auto sprite = 
					AssetManager::getSprite(renderComponent.spritePath, { renderComponent.m_Pos.x, renderComponent.m_Pos.y });
				renderComponent.m_Sprite = sprite;
				Renderer2D::drawSprite(obj.getTransform(), renderComponent, (int)entity);

			}

			Renderer2D::endScene();
		}
	}

	void Scene::onUpdate(TimeStep ts)
	{
		// render objects in the scene through scene update.
		if (m_Valid) {

			ExecuteDeletionQueue();
			
			//// delete object
			//{
			//	for (int i = 0; i < m_ObjectsToDelete.size(); i++) {
			//		destroyGameObject(getGameObjectById(m_ObjectsToDelete.at(i)));
			//	}

			//	m_ObjectsToDelete.clear();
			//}

			// update scripts
			{
				// C# Entity OnUpdate
				auto view = m_Registry.view<ScriptComponent>();
				for (auto entity : view)
				{
					GameObject obj = { entity, this };
					ScriptEngine::OnUpdateGameObject(obj, ts);
				}
			}


			// update physics
			{
				const int32_t velocityIterations = 6;
				const int32_t positionIterations = 3;

				// HZN_CORE_DEBUG(m_World->GetBodyCount());

				m_World->Step(ts, velocityIterations, positionIterations);

				auto view = m_Registry.view<RigidBody2DComponent>();

				if (view) {
					for (auto entity : view) {
						GameObject obj = { entity, this };

						auto& transform = obj.getComponent<TransformComponent>();
						auto& rb2d = obj.getComponent<RigidBody2DComponent>();

						b2Body* body = (b2Body*)rb2d.m_RuntimeBody;

						const auto& position = body->GetPosition();
						if (obj.getParent())
						{
							auto objLocalMat = glm::mat4(1.0f);
							objLocalMat = glm::translate(objLocalMat, glm::vec3(position.x, position.y, transform.m_Translation.z));
							objLocalMat = glm::rotate(objLocalMat, glm::radians(body->GetAngle()), glm::vec3(0, 0, 1));
							auto modelMat = obj.getParent().getTransform();
							modelMat = glm::inverse(modelMat) * objLocalMat;

							glm::vec3 translation = glm::vec3(0.0f);
							glm::quat orientation = glm::quat();
							glm::vec3 scale = glm::vec3(0.0f);
							glm::vec3 skew = glm::vec3(0.0f);
							glm::vec4 perspective = glm::vec4(0.0f);
							glm::decompose(modelMat, scale, orientation, translation, skew, perspective);

							glm::vec3 rotation = glm::eulerAngles(orientation);

							transform.m_Translation.x = translation.x;
							transform.m_Translation.y = translation.y;
							transform.m_Rotation.z = glm::degrees(rotation.z);

						}
						else
						{
							transform.m_Translation.x = position.x;
							transform.m_Translation.y = position.y;
							transform.m_Rotation.z = glm::degrees(body->GetAngle());
						}
					}
				}
			}


			// scr
			const SceneCamera2D* activeCamera = nullptr;
			glm::mat4 cameraTransform = glm::mat4(1.0f);

			const auto& cameras = m_Registry.view<CameraComponent, TransformComponent>();

			for (const auto& entity : cameras)
			{
				const auto& cameraComponent = cameras.get<CameraComponent>(entity);
				const auto& transformComponent = cameras.get<TransformComponent>(entity);
				if (cameraComponent.m_Primary)
				{
					activeCamera = &cameraComponent.m_Camera;
					cameraTransform = transformComponent.getModelMatrix();
				}
			}

			if (activeCamera)
			{
				const auto& sprites = m_Registry.view<RenderComponent, TransformComponent>();

				Renderer2D::beginScene(*activeCamera, cameraTransform);
				for (const auto& entity : sprites)
				{
					auto [renderComponent, transformComponent] = sprites.get<RenderComponent, TransformComponent>(entity);
					GameObject obj = getGameObjectById(entt::to_integral(entity));

					Renderer2D::drawSprite(obj.getTransform(), renderComponent, (int)entity);

				}
				Renderer2D::endScene();
			}
		}
	}

	GameObject Scene::createGameObject(const std::string& name)
	{
		if (!m_Valid)
		{
			throw std::runtime_error("Adding game object to invalidated scene!");
		}

		GameObject obj = { m_Registry.create(), this };
		// every valid game object has a name component
		obj.addComponent<NameComponent>(name);
		obj.addComponent<RelationComponent>();
		obj.addComponent<TransformComponent>();
		/*m_Objects.insert({ name, obj });*/
		m_GameObjectIdMap.insert({ entt::to_integral(obj.m_ObjectId), obj.m_ObjectId });
		return obj;
	}

	bool Scene::destroyGameObject(GameObject& obj)
	{
		if (!m_Valid)
		{
			HZN_CORE_ERROR("Failed to destroy GameObject that has ID: {0}", entt::to_integral(obj.m_ObjectId));
			return false;
		}

		// add this object to the deletion queue.
		m_DeletionQueue.insert(obj.m_ObjectId);

		// delete all the child objects of that object, before we delete that object.
		auto list = obj.getChildren();

		for (auto& x : list)
		{
			destroyGameObject(x);
		}

		return true;
	}

	GameObject Scene::getGameObjectById(uint32_t id)
	{
		if (!m_Valid)
		{
			throw std::runtime_error("trying to get game objects from invalidated scene!");
		}

		auto it = m_GameObjectIdMap.find(id);

		if (it == m_GameObjectIdMap.end())
		{
			return GameObject();
		}

		return GameObject{ it->second, this };
	}

	GameObject Scene::getGameObjectByName(const std::string& name) {
		if (!m_Valid)
		{
			throw std::runtime_error("trying to get game objects from invalidated scene!");
		}

		std::vector<uint32_t> allIds = getAllObjectIds();
		for (int i = 0; i < allIds.size(); i++) {
			GameObject obj = getGameObjectById(allIds.at(i));

			auto& nameComponent = obj.getComponent<NameComponent>();
			if (nameComponent.m_Name == name) return GameObject{ m_GameObjectIdMap.find(allIds.at(i))->second, this };
		}

		// Object not found
		throw std::runtime_error("Game object not found!");
	}

	std::vector<GameObject> Scene::getGameObjectsByName(const std::string& name) {
		if (!m_Valid)
		{
			throw std::runtime_error("trying to get game objects from invalidated scene!");
		}

		std::vector<uint32_t> allIds = getAllObjectIds();
		std::vector<GameObject> objects;
		for (int i = 0; i < allIds.size(); i++) {
			GameObject obj = getGameObjectById(allIds.at(i));

			auto& nameComponent = obj.getComponent<NameComponent>();
			if (nameComponent.m_Name == name) objects.push_back(GameObject{ m_GameObjectIdMap.find(allIds.at(i))->second, this });
		}

		return objects;
	}

	std::vector<uint32_t> Scene::getAllRootIds() const
	{
		std::vector<uint32_t> roots;
		if (m_Valid) {
			const auto& alls = m_Registry.view<RelationComponent>();
			for (const auto& entity : alls)
			{
				const auto& parent = m_Registry.get<RelationComponent>(entity).m_Parent;
				if (parent == entt::null) roots.emplace_back(entt::to_integral(entity));
			}
		}
		return roots;
	}

	std::vector<uint32_t> Scene::getAllObjectIds() const
	{
		std::vector<uint32_t> ids;
		for (const auto& x : m_GameObjectIdMap)
		{
			ids.emplace_back(x.first);
		}
		return ids;
	}





	void Scene::addBody(GameObject& obj) {
		auto transform = obj.getTransform();
		auto& transformComponent = obj.getComponent<TransformComponent>();
		auto& rb2d = obj.getComponent<RigidBody2DComponent>();

		glm::vec3 translation = glm::vec3(0.0f);
		glm::quat orientation = glm::quat();
		glm::vec3 scale = glm::vec3(0.0f);
		glm::vec3 skew = glm::vec3(0.0f);
		glm::vec4 perspective = glm::vec4(0.0f);
		glm::decompose(transform, scale, orientation, translation, skew, perspective);

		glm::vec3 rotation = glm::eulerAngles(orientation);
		rotation = glm::degrees(rotation);

		b2BodyDef bodyDef;
		bodyDef.type = (b2BodyType)rb2d.m_Type;
		bodyDef.position.Set(translation.x, translation.y);
		bodyDef.angle = glm::radians(rotation.z);
		bodyDef.userData.pointer = (uintptr_t)&m_GameObjectIdMap[obj.getObjectId()];

		b2Body* body = m_World->CreateBody(&bodyDef);
		body->SetFixedRotation(rb2d.m_FixedRotation);
		rb2d.m_RuntimeBody = body;

		if (!obj.hasComponent<BoxCollider2DComponent>())
		{
			obj.addComponent<BoxCollider2DComponent>();
		}

		auto& bc2d = obj.getComponent<BoxCollider2DComponent>();
		b2PolygonShape polygonShape;

		polygonShape.SetAsBox(scale.x * bc2d.size.x, scale.y * bc2d.size.y);

		b2FixtureDef fixtureDef;
		fixtureDef.shape = &polygonShape;
		fixtureDef.density = bc2d.m_Density;
		fixtureDef.friction = bc2d.m_Friction;
		fixtureDef.restitution = bc2d.m_Restitution;
		fixtureDef.restitutionThreshold = bc2d.m_RestitutionThreshold;
		fixtureDef.isSensor = true; // TODO: Change/remove this
		body->CreateFixture(&fixtureDef);
	}
}
