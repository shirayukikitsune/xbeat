#include "Scene.h"


Renderer::Scene::Scene()
{
}


Renderer::Scene::~Scene()
{
}

void Renderer::Scene::setResources(std::shared_ptr<Dispatcher> EventDispatcher, std::shared_ptr<D3DRenderer> Renderer, std::shared_ptr<ModelManager> ModelHandler, std::shared_ptr<Input::Manager> InputManager)
{
	this->EventDispatcher = EventDispatcher;
	this->Renderer = Renderer;
	this->ModelHandler = ModelHandler;
	this->InputManager = InputManager;
}