#pragma once
#ifndef HZN_COMPONENT_DISPLAYS_H
#define HZN_COMPONENT_DISPLAYS_H

#include <vector>
#include <string>
#include <memory>

#include <HorizonEngine/HorizonEngine.h>

namespace Hzn
{
	template<typename ...Component>
	std::vector<const char*> getComponentStringList(ComponentGroup<Component...>)
	{
		std::vector<const char*> result;
		([&]
		{
			const char* val = typeid(Component).name();
			result.emplace_back(val);
		}(), ...);
		return result;
	}

	struct ComponentDisplays
	{
		template<typename T>
		static void display(GameObject& obj) {}

		template<typename ...Component>
		static void displayIfExists(GameObject& obj, ComponentGroup<Component...>)
		{
			([&] {
				if (obj.hasComponent<Component>())
				{
					display<Component>(obj);
				}
				}(), ...);
		}
	};

	template<>
	inline void ComponentDisplays::display<NameComponent>(GameObject& obj)
	{
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
			ImGuiTreeNodeFlags_OpenOnDoubleClick |
			ImGuiTreeNodeFlags_SpanAvailWidth |
			ImGuiTreeNodeFlags_SpanFullWidth |
			ImGuiTreeNodeFlags_Selected |
			ImGuiTreeNodeFlags_DefaultOpen;
		if(ImGui::TreeNodeEx(ICON_FA_SHAPES " Tag", flags))
		{
			char nameString[512]{};
			strcpy(nameString, obj.getComponent<NameComponent>().m_Name.c_str());
			if (ImGui::InputText("Name", nameString, IM_ARRAYSIZE(nameString), ImGuiInputTextFlags_AutoSelectAll)) {
				std::string name(nameString);
				if(!name.empty())
				{
					obj.getComponent<NameComponent>().m_Name = name;
				}
			}
			ImGui::TreePop();
		}
	}

	template<>
	inline void ComponentDisplays::display<TransformComponent>(GameObject& obj)
	{
		auto& transform = obj.getComponent<TransformComponent>();

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
			ImGuiTreeNodeFlags_OpenOnDoubleClick |
			ImGuiTreeNodeFlags_SpanAvailWidth |
			ImGuiTreeNodeFlags_SpanFullWidth |
			ImGuiTreeNodeFlags_Selected |
			ImGuiTreeNodeFlags_DefaultOpen;

		if (ImGui::TreeNodeEx(ICON_FA_SHAPES " Transform", flags)) {
			ImGui::DragFloat3("Position", glm::value_ptr(transform.m_Translation), 0.25f, -50.0f, 50.0f);
			ImGui::DragFloat3("Scale", glm::value_ptr(transform.m_Scale), 0.25f, 1.0f, 50.0f);
			ImGui::DragFloat3("Rotation", glm::value_ptr(transform.m_Rotation), 1.0f,
				-360.0f, 360.0f);
			ImGui::TreePop();
		}
	}

	template<>
	inline void ComponentDisplays::display<RenderComponent>(GameObject& obj)
	{
		auto& renderComponent = obj.getComponent<RenderComponent>();
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | 
			ImGuiTreeNodeFlags_OpenOnDoubleClick | 
			ImGuiTreeNodeFlags_SpanAvailWidth | 
			ImGuiTreeNodeFlags_SpanFullWidth | 
			ImGuiTreeNodeFlags_Selected | 
			ImGuiTreeNodeFlags_DefaultOpen |
			ImGuiTreeNodeFlags_AllowItemOverlap;

		bool treeOpen = ImGui::TreeNodeEx(ICON_FA_SHAPES " Render", flags);
		ImGui::SameLine(ImGui::GetContentRegionAvail().x);

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
		if (ImGui::Button("...", ImVec2(30, 18))) ImGui::OpenPopup("RenderProperties");
		ImGui::PopStyleColor(1);

		bool componentRemoved = false;
		if (ImGui::BeginPopup("RenderProperties")) {
			if (ImGui::MenuItem("Remove")) {
				obj.removeComponent<RenderComponent>();
				componentRemoved = true;
			}

			ImGui::EndPopup();
		}

		if (componentRemoved) {
			if (treeOpen)
				ImGui::TreePop();
			return;
		}

		if (treeOpen) {
			ImGui::ColorEdit3("Color", glm::value_ptr(renderComponent.m_Color));

			auto windowX = ImGui::GetWindowWidth();
			auto val = ImGui::CalcTextSize("Texture");
			ImGui::SetCursorPosX((windowX - (val.x + 50.0f)) * 0.5f);
			ImGui::Button("Texture", ImVec2(val.x + 50.0f, 0));
			if (ImGui::BeginDragDropTarget())
			{

				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM_SPRITE"))
				{

					const wchar_t* filepath = (const wchar_t*)payload->Data;
					std::wstring ws(filepath);
					std::string str(ws.begin(), ws.end());

					std::string::size_type nPos1 = str.find_last_of(";");
					std::string::size_type nPos2 = str.find_last_of(";", nPos1 - 1);

					std::string spriteY = str.substr(nPos1 + 1);
					std::string spriteX = str.substr(nPos2 + 1, nPos1 - nPos2 - 1);

					renderComponent.spritePath = str.substr(0, str.find_last_of("-"));
					renderComponent.texturePath = "";
					renderComponent.m_Pos.x = static_cast<float>(std::stoi(spriteX));
					renderComponent.m_Pos.y = static_cast<float>(std::stoi(spriteY));

				}
				else if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					const std::string* filepath = (const std::string*)payload->Data;

					renderComponent.texturePath = filepath->c_str();
					renderComponent.spritePath = "";

				}
				ImGui::EndDragDropTarget();
			}

			ImGui::TreePop();
		}
	}


	template<>
	inline void ComponentDisplays::display<CameraComponent>(GameObject& obj)
	{
		auto& cameraComponent = obj.getComponent<CameraComponent>();
		float m_CameraZoom = cameraComponent.m_Camera.getZoom();
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
			ImGuiTreeNodeFlags_OpenOnDoubleClick | 
			ImGuiTreeNodeFlags_SpanAvailWidth | 
			ImGuiTreeNodeFlags_SpanFullWidth | 
			ImGuiTreeNodeFlags_Selected | 
			ImGuiTreeNodeFlags_DefaultOpen |
			ImGuiTreeNodeFlags_AllowItemOverlap;

		bool treeOpen = ImGui::TreeNodeEx(ICON_FA_SHAPES " Camera", flags);
		ImGui::SameLine(ImGui::GetContentRegionAvail().x);

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
		if (ImGui::Button("...", ImVec2(30, 18))) ImGui::OpenPopup("CameraProperties");
		ImGui::PopStyleColor(1);

		bool componentRemoved = false;
		if (ImGui::BeginPopup("CameraProperties")) {
			if (ImGui::MenuItem("Remove")) {
				obj.removeComponent<CameraComponent>();
				componentRemoved = true;
			}

			ImGui::EndPopup();
		}

		if (componentRemoved) {
			if (treeOpen)
				ImGui::TreePop();
			return;
		}

		if (treeOpen) {
			if (ImGui::SliderFloat("Zoom", &m_CameraZoom, 0.25f, 10.0f))
				cameraComponent.m_Camera.setZoom(m_CameraZoom);
			ImGui::TreePop();
		}
	}

	template<>
	inline void ComponentDisplays::display<RigidBody2DComponent>(GameObject& obj)
	{
		auto& component = obj.getComponent<RigidBody2DComponent>();

		const char* bodyTypeStrings[] = {"Static", "Kinematic", "Dynamic"};
		const char* currentBodyTypeString = bodyTypeStrings[(int)component.m_Type];
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
			ImGuiTreeNodeFlags_OpenOnDoubleClick |
			ImGuiTreeNodeFlags_SpanAvailWidth |
			ImGuiTreeNodeFlags_SpanFullWidth |
			ImGuiTreeNodeFlags_Selected |
			ImGuiTreeNodeFlags_DefaultOpen |
			ImGuiTreeNodeFlags_AllowItemOverlap;

		bool treeOpen = ImGui::TreeNodeEx(ICON_FA_SHAPES " Rigidbody 2D", flags);
		ImGui::SameLine(ImGui::GetContentRegionAvail().x);

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
		if (ImGui::Button("...", ImVec2(30, 18))) ImGui::OpenPopup("Rigidbody2DProperties");
		ImGui::PopStyleColor(1);

		bool componentRemoved = false;
		if (ImGui::BeginPopup("Rigidbody2DProperties")) {
			if (ImGui::MenuItem("Remove")) {
				obj.removeComponent<RigidBody2DComponent>();
				componentRemoved = true;
			}

			ImGui::EndPopup();
		}

		if (componentRemoved) {
			if (treeOpen)
				ImGui::TreePop();
			return;
		}

		if (treeOpen) {
			if(ImGui::BeginCombo("Body Type", currentBodyTypeString))
			{
				for(int i = 0; i < 3; ++i)
				{
					bool isSelected = currentBodyTypeString == bodyTypeStrings[i];
					if(ImGui::Selectable(bodyTypeStrings[i], isSelected))
					{
						currentBodyTypeString = bodyTypeStrings[i];
						component.m_Type = (RigidBody2DComponent::BodyType)i;
					}

					if(isSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}


				ImGui::EndCombo();
			}

			ImGui::Checkbox("Fixed Rotation", &component.m_FixedRotation);

			ImGui::TreePop();
		}
	}

	template<>
	inline void ComponentDisplays::display<BoxCollider2DComponent>(GameObject& obj)
	{
		auto& component = obj.getComponent<BoxCollider2DComponent>();
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
			ImGuiTreeNodeFlags_OpenOnDoubleClick |
			ImGuiTreeNodeFlags_SpanAvailWidth |
			ImGuiTreeNodeFlags_SpanFullWidth |
			ImGuiTreeNodeFlags_Selected |
			ImGuiTreeNodeFlags_DefaultOpen |
			ImGuiTreeNodeFlags_AllowItemOverlap;

		bool treeOpen = ImGui::TreeNodeEx(ICON_FA_SHAPES " Box Collider 2D", flags);
		ImGui::SameLine(ImGui::GetContentRegionAvail().x);

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
		if (ImGui::Button("...", ImVec2(30, 18))) ImGui::OpenPopup("BoxCollider2DProperties");
		ImGui::PopStyleColor(1);

		bool componentRemoved = false;
		if (ImGui::BeginPopup("BoxCollider2DProperties")) {
			if (ImGui::MenuItem("Remove")) {
				obj.removeComponent<BoxCollider2DComponent>();
				componentRemoved = true;
			}

			ImGui::EndPopup();
		}

		if (componentRemoved) {
			if (treeOpen)
				ImGui::TreePop();
			return;
		}

		if (treeOpen) {
			ImGui::Checkbox("Sensor", &component.m_IsSensor);
			ImGui::DragFloat2("Offset", glm::value_ptr(component.offset));
			ImGui::DragFloat2("Size", glm::value_ptr(component.size));
			ImGui::DragFloat("Density", &component.m_Density, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Friction", &component.m_Friction, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Restitution", &component.m_Restitution, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Restitution Threshold", &component.m_RestitutionThreshold, 0.01f, 0.0f, 1.0f);
			ImGui::TreePop();
		}
	}


	template<>
	inline void ComponentDisplays::display<ScriptComponent>(GameObject& obj)
	{
		auto& component = obj.getComponent<ScriptComponent>();
		std::string selectedScriptName = component.m_ScriptName;

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
			ImGuiTreeNodeFlags_OpenOnDoubleClick |
			ImGuiTreeNodeFlags_SpanAvailWidth |
			ImGuiTreeNodeFlags_SpanFullWidth |
			ImGuiTreeNodeFlags_Selected |
			ImGuiTreeNodeFlags_DefaultOpen |
			ImGuiTreeNodeFlags_AllowItemOverlap;

		bool treeOpen = ImGui::TreeNodeEx(ICON_FA_SHAPES " Scripts", flags);
		ImGui::SameLine(ImGui::GetContentRegionAvail().x);

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
		if (ImGui::Button("...", ImVec2(30, 18))) ImGui::OpenPopup("ScriptProperties");
		ImGui::PopStyleColor(1);

		bool componentRemoved = false;
		if (ImGui::BeginPopup("ScriptProperties")) {
			if (ImGui::MenuItem("Remove")) {
				obj.removeComponent<ScriptComponent>();
				componentRemoved = true;
			}

			ImGui::EndPopup();
		}

		if (componentRemoved) {
			if (treeOpen)
				ImGui::TreePop();
			return;
		}

		auto val = ScriptEngine::GetGameObjectSubClasses();

		if (treeOpen) {
			if(ImGui::BeginCombo("Script", selectedScriptName.c_str()))
			{
				for (const auto& script : val)
				{
					bool isSelected = selectedScriptName == script.first;
					if(ImGui::Selectable(script.first.c_str(), isSelected))
					{
						component.m_ScriptName = selectedScriptName = script.first;
					}

					if (isSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}

				ImGui::EndCombo();
			}
			ImGui::TreePop();
		}
	}
}
#endif // !HZN_COMPONENT_DISPLAYS_H
