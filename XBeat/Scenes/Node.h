//===-- Renderer/Node.h - Declares a class for scene objects ------*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===------------------------------------------------------------------------===//
///
/// \file
/// \brief This file declares everything related to the Node class, which
/// describes objects in a Scene.
///
//===------------------------------------------------------------------------===//

#pragma once

#include <DirectXMath.h>

namespace Scenes {
	///! Base class for all objects in a scene
	class Node
	{
	public:
		Node(Node *Parent);
		virtual ~Node();

	private:
		///! Offset position relative to parent node
		DirectX::XMVECTOR Position;

		///! Rotation relative to parent node
		DirectX::XMVECTOR Rotation;

		///! Transform relative to parent node
		DirectX::XMMATRIX LocalTransform;

		///! Transform in world units
		DirectX::XMMATRIX WorldTransform;

		///! This node's parent node
		Node *Parent;
	};
}
