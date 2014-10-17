#include "Scene.h"


Scenes::Scene::Scene()
{
}


Scenes::Scene::~Scene()
{
}

void Scenes::Scene::setResources(std::shared_ptr<Dispatcher> EventDispatcher, std::shared_ptr<ModelManager> ModelHandler, std::shared_ptr<Input::Manager> InputManager, std::shared_ptr<Physics::Environment> Physics, std::shared_ptr<Renderer::D3DRenderer> Renderer)
{
	this->EventDispatcher = EventDispatcher;
	this->Renderer = Renderer;
	this->ModelHandler = ModelHandler;
	this->InputManager = InputManager;
	this->Physics = Physics;
}
