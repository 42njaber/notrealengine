#include "Object/GLObject.class.hpp"
#include "Object/AssetManager.class.hpp"
#include "Object/AssimpHelpers.hpp"
#include "Object/AssimpObjectImporter.class.hpp"
#include "Object/CustomObjectImporter.class.hpp"
#include "GLContext.class.hpp"
#include "CheckFileType.hpp"
#include "mft/mft.hpp"
#include "SDL.h"

//	OpenGL includes
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include <windows.h>
#include <glad/glad.h>
#else
# define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#endif

#include <fstream>
#include <algorithm>
#include <sys/stat.h>

namespace notrealengine
{

	GLObject::~GLObject()
	{
	}

	GLObject::GLObject(GLObject const& ref): Asset(ref.getPaths())
	{
		*this = ref;
	}

	GLObject::GLObject(const std::string& path)
		: Asset({path}),
		transform(), polygonMode(GL_FILL),
		directory(""), meshes(), bones(), nbBones(0),
		shader(GLContext::getShader("default")->programID),
		visible(true), max(0.0f), min(0.0f), isRangeInit(false),
		anim(nullptr), startTime(0.0f), pauseTime(0.0f), currentTime(0.0f),
		animationState(AnimationState::Stopped),
		animationRepeat(AnimationRepeat::Repeat),
		animationSpeed(1.0)
	{
		std::filesystem::path	fPath(path);
		if (!std::filesystem::exists(fPath))
		{
			std::cerr << "nre: Unable to open file \"" << path << "\"" << std::endl;
			return;
		}
		if (!IsReg(path))
		{
			std::cerr << "nre: Invalid file type" << std::endl;
			return ;
		}
		loadObject(path);
		BuildMeshesMap();
	}

	GLObject::GLObject(std::vector<std::shared_ptr<Mesh>>& meshes)
		: Asset(),
		transform(), polygonMode(GL_FILL),
		directory(""), meshes(meshes), bones(), nbBones(0),
		shader(GLContext::getShader("default")->programID),
		visible(true), max(0.0f), min(0.0f), isRangeInit(false),
		anim(nullptr), startTime(0.0f), pauseTime(0.0f), currentTime(0.0f),
		animationState(AnimationState::Stopped),
		animationRepeat(AnimationRepeat::Repeat),
		animationSpeed(0.0)
	{
		std::cout << "Building object from meshes..." << std::endl;
		BuildMeshesMap();
	}

	GLObject& GLObject::operator=(GLObject const& ref)
	{
		// TODO
		return *this;
	}

	void GLObject::SaveMeshInMap(const std::shared_ptr<Mesh>& mesh)
	{
		this->meshesMap[mesh->getName()] = mesh;
		for (const auto child: mesh->getChildren())
		{
			SaveMeshInMap(child);
		}
	}

	void GLObject::BuildMeshesMap()
	{
		for (auto& mesh: this->meshes)
		{
			SaveMeshInMap(mesh);
		}
		for (const auto& pair: this->meshesMap)
		{
			const std::shared_ptr<Mesh>& mesh = pair.second;
		}
	}

	//	Texture utility

	void	GLObject::addTexture(unsigned int mesh, std::shared_ptr < Texture >& text)
	{
		if (mesh >= 0 && mesh < meshes.size())
			(*meshes[mesh]).addTexture(text);
		else
			throw std::runtime_error("Mesh index out of bounds");
	}

	void	GLObject::loadObject(const std::string& path, unsigned int flags)
	{
		std::cout << "Loading object '" << path;

		std::unique_ptr<ObjectImporter>	importer;
//#define USING_EXTERNAL_LIBS
#ifdef USING_EXTERNAL_LIBS
		importer = std::make_unique<AssimpObjectImporter>();
		std::cout << "' with assimp..." << std::endl;
#else
		importer = std::make_unique<CustomObjectImporter>();
		std::cout << "' with custom parser..." << std::endl;
#endif // USING_EXTERNAL_LIBS

		importer->ReadFile(path, flags);

		this->max = importer->max;
		this->min = importer->min;
		this->isRangeInit = importer->isRangeInit;

		this->meshes = importer->meshes;
		this->bones = importer->bones;
		this->nbBones = importer->nbBones;

		//	Scale the object

		float rangeX = this->max.x - this->min.x;
		float rangeY = this->max.y - this->min.y;
		float rangeZ = this->max.z - this->min.z;

		float minRange;
		if (rangeX == 0)
			minRange = std::min(rangeY, rangeZ);
		else if (rangeY == 0)
			minRange = std::min(rangeZ, rangeX);
		else if (rangeZ == 0)
			minRange = std::min(rangeY, rangeX);
		else
			minRange = std::min(std::min(rangeX, rangeY), rangeZ);

		float scale = 2.0f / minRange;
		this->transform.scale(mft::vec3(scale));
	}

	//	Drawing functions

	void	GLObject::draw( void )
	{
		if (this->animationState == AnimationState::Playing)
			this->updateAnim();
		mft::vec3 scale = this->transform.getScale();
		for (size_t i = 0; i < meshes.size(); i++)
		{
			meshes[i]->draw(scale, this->transform.getMatrix(), this->shader);
		}
	}

	void	GLObject::drawBones( void ) const
	{
		unsigned int shader = GLContext::getShader("colorUnlit")->programID;
		GLCallThrow(glUseProgram, shader);
		GLCallThrow(glDisable, GL_DEPTH_TEST);
		Mesh	cube(GLContext::centeredCube);
		cube.setColor(mft::vec3(204.0f / 255.0f, 0.0f, 204.0f / 255.0f));
		cube.setShader(shader);

		cube.setColor(mft::vec3(0.0f, 1.0f, 0.0f));
		const mft::vec3& objScale = this->transform.getScale();
		mft::vec3 invObjScale = 0.05f / objScale;
		mft::mat4 invObjScaleMatrix = mft::mat4::scale(invObjScale);
		std::map<std::string, BoneInfo>::const_iterator it;
		for (it = bones.begin(); it != bones.end(); it++)
		{
			cube.draw(mft::vec3(1.0f, 1.0f, 1.0f), transform.getMatrix() * it->second.modelMatrix * invObjScaleMatrix);
		}
		GLCallThrow(glEnable, GL_DEPTH_TEST);
	}

	void	GLObject::bindBones( void ) const
	{
		unsigned int shader = this->shader == 0 ? GLContext::getShader("default")->programID : this->shader;
		GLCallThrow(glUseProgram, shader);
		GLint location;
		std::string str;
		std::map<std::string, BoneInfo>::const_iterator it;
		for (it = bones.begin(); it != bones.end(); it++)
		{
			str = "bonesMatrices[" + std::to_string(it->second.id) + "]";
			location = GLCallThrow(glGetUniformLocation, shader, str.c_str());
			GLCallThrow(glUniformMatrix4fv, location, 1, GL_TRUE,
				static_cast<const float*>(it->second.localMatrix));
		}
	}

	void	GLObject::resetPose()
	{
		if (this->anim == nullptr)
			return ;
		if (this->anim->getType() == Animation::Skeletal)
		{
			unsigned int shader = this->shader == 0 ? GLContext::getShader("default")->programID : this->shader;
			GLCallThrow(glUseProgram, shader);
			GLint location;
			mft::mat4	mat = mft::mat4();
			std::string str;
			int i = 0;
			std::map<std::string, BoneInfo>::iterator it;
			for (it = bones.begin(); it != bones.end(); it++)
			{
				it->second.localMatrix = it->second.originalMatrix * it->second.offsetMatrix;
				it->second.modelMatrix = it->second.originalMatrix;
				str = "bonesMatrices[" + std::to_string(it->second.id) + "]";
				location = GLCallThrow(glGetUniformLocation, shader, str.c_str());
				GLCallThrow(glUniformMatrix4fv, location, 1, GL_TRUE,
					static_cast<const float*>(it->second.localMatrix));
				i++;
			}
		}
		else if (this->anim->getType() == Animation::Solid)
		{
			for (const auto& pair : this->meshesMap)
			{
				//	Hack to make the transform dirty and recompute it
				pair.second->animTransform.setPos(mft::vec3());
				pair.second->animTransform.setRotation(mft::quat());
				pair.second->animTransform.setScale(mft::vec3(1.0f));
			}
		}
		this->animationState = AnimationState::Stopped;
	}

	void	GLObject::pauseAnimation( void )
	{
		if (this->animationState != AnimationState::Playing)
			return;
		this->pauseTime = static_cast<float>(SDL_GetTicks());
		this->animationState = AnimationState::Paused;
		std::vector<AnimNode>& animNodes = anim->getNodes();
		std::cout << "Anim nodes: " << std::endl;
		for (size_t i = 0; i < animNodes.size(); i++)
		{
			if (i == 6)
			{
				std::cout << animNodes[i].name << " (" << i << "): " << animNodes[i].transform << std::endl;
			}
		}
	}

	void	GLObject::resumeAnimation( void )
	{
		if (this->animationState != AnimationState::Paused)
			return;
		this->startTime += static_cast<float>(SDL_GetTicks()) - this->pauseTime;
		this->animationState = AnimationState::Playing;
	}

	void	GLObject::stopAnimation( void )
	{
		this->animationState = AnimationState::Stopped;
	}

	void	GLObject::updateSolidAnim( void )
	{
		std::map<std::string, Bone> animBones = anim->getBones();
		for (auto& pair: animBones)
		{
			Bone& bone = pair.second;
			//	If a bone of the animation is associated with this node,
			//	use its animation transform


			std::map<std::string, std::shared_ptr<Mesh>>::iterator it =
				this->meshesMap.find(pair.first);
			if (it != this->meshesMap.end())
			{
				it->second->animTransform.setPos(bone.getPosition(this->currentTime));
				it->second->animTransform.setRotation(bone.getRotation(this->currentTime));
				it->second->animTransform.setScale(bone.getScale(this->currentTime));
			}
		}
	}

	void	GLObject::updateSkeletalAnim( void )
	{
		std::map<std::string, Bone> animBones = anim->getBones();
		//	Make sure that we take a copy of it
		//	and not the reference; do not modify the real anim nodes
		//	Could be optimized by saving a new vector each time we bind
		//	an animation and precompute nodes that are not linked to any bone
		//	only once instead of each frame
		std::vector<AnimNode> animNodes = anim->getNodes();
		//animNodes[0].transform *= this->transform.getMatrix();
		for (unsigned int i = 0; i < animNodes.size(); i++)
		{
			AnimNode& node = animNodes[i];

			//	If a bone of the animation is associated with this node,
			//	use its animation transform

			std::map<std::string, Bone>::iterator it =
				animBones.find(node.name);
			if (it != animBones.end())
			{
				node.transform = animNodes[node.parentId].transform
					* it->second.getTransform(this->currentTime);
			}
			//	Otherwise, use the original node's transform
			else if (i > 0)
			{
				node.transform = animNodes[node.parentId].transform
					* node.transform;
			}
			//	If a bone of the object is associated with this node,
			//	update its transform with what we just computed
			std::map<std::string, BoneInfo>::iterator it2 = this->bones.find(node.name);
			if (it2 != this->bones.end())
			{
				BoneInfo& bone = it2->second;

				bone.modelMatrix = node.transform;
				bone.localMatrix = bone.modelMatrix * bone.offsetMatrix;
			}
		}
		this->bindBones();
	}

	void	GLObject::updateAnim( void )
	{
		if (this->anim == nullptr)
		{
			this->resetPose();
			return;
		}
		this->currentTime = (static_cast<float>(SDL_GetTicks()) - this->startTime);
		if (this->currentTime >= anim->getDuration())
		{
			this->animationState = AnimationState::Stopped;
			if (this->animationRepeat == AnimationRepeat::Repeat)
				playAnimation(anim);
			else if (this->animationRepeat == AnimationRepeat::ResetPose)
				resetPose();
			return;
		}
		if (this->anim->getType() == Animation::Skeletal)
			updateSkeletalAnim();
		else if (this->anim->getType() == Animation::Solid)
			updateSolidAnim();
	}

	void	GLObject::setToKeyFrame(unsigned int keyFrame)
	{
		if (this->anim == nullptr)
			return ;
		std::map<std::string, Bone> animBones = anim->getBones();
		std::vector<AnimNode>& animNodes = anim->getNodes();
		for (unsigned int i = 0; i < animNodes.size(); i++)
		{
			AnimNode& node = animNodes[i];

			//	If a bone of the animation is associated with this node,
			//	use its animation transform

			if (animBones.contains(node.name))
			{
				node.transform = animBones[node.name].getKeyFrameTransform(keyFrame)
					* animNodes[node.parentId].transform;
			}
			//	Otherwise, use the original node's transform
			else
			{
				node.transform = node.transform
					* animNodes[node.parentId].transform;
			}
			//	If a bone of the object is associated with this node,
			//	update its transform with what we just computed
			if (this->bones.contains(node.name))
			{
				BoneInfo& bone = this->bones[node.name];

				bone.modelMatrix = node.transform;
				bone.localMatrix = bone.offsetMatrix * bone.modelMatrix;
			}
		}
		this->bindBones();
	}

	void	GLObject::playAnimation(Animation* anim, AnimationRepeat animationRepeat)
	{
		if (anim == nullptr)
			return;
		this->anim = anim;
		this->animationState = AnimationState::Playing;
		this->animationRepeat = animationRepeat;
		this->startTime = static_cast<float>(SDL_GetTicks());
	}

	//	Accessors

	const std::vector<std::shared_ptr<Mesh>>&	GLObject::getMeshes() const
	{
		return meshes;
	}

	std::map<std::string, BoneInfo>&	GLObject::getBones()
	{
		return bones;
	}

	const int GLObject::getNbBones() const
	{
		return nbBones;
	}

	const std::string GLObject::getAssetType() const
	{
		return std::string("GLObject");
	}

	const unsigned int GLObject::getShader() const
	{
		return shader;
	}

	std::map<std::string, std::shared_ptr<Mesh>>& GLObject::getMeshesMap()
	{
		return meshesMap;
	}

	const AnimationState& GLObject::getAnimationState() const
	{
		return animationState;
	}

	const float GLObject::getStartTime() const
	{
		return startTime;
	}

	const float GLObject::getCurrentTime() const
	{
		return currentTime;
	}

	const std::string GLObject::getAnimationStateStr() const
	{
		switch (animationState)
		{
		case Playing:
			return "Playing";
			break;
		case Paused:
			return "Paused";
			break;
		case Stopped:
			return "Stopped";
			break;
		default:
			return "Invalid animation state";
		}
	}

	//	Setters

	void	GLObject::setName(std::string name)
	{
		this->name = name;
	}

	void GLObject::setShader(unsigned int shader)
	{
		this->shader = shader;
	}

	void GLObject::setShader(GLShaderProgram *shader)
	{
		this->shader = shader->programID;
	}

	void GLObject::setAnimation(Animation* anim)
	{
		this->anim = anim;
		AnimationState state = this->animationState;
		this->resetPose();
		if (state == AnimationState::Paused)
			this->animationState = AnimationState::Stopped;
		else
			this->animationState = state;
	}

	std::ostream& operator<<(std::ostream& o, GLObject const& obj)
	{
		std::vector<std::shared_ptr<Mesh>>	meshes = obj.getMeshes();
		std::cout << obj.getName();
		for (size_t i = 0; i < meshes.size(); i++)
		{
			std::cout << "Mesh " << i << ":" << std::endl << *meshes[i];
		}
		return o;
	}
}