#ifndef _COLLADA_SCENE_BUILDER_CLASS_H_
#define _COLLADA_SCENE_BUILDER_CLASS_H_

#include "ColladaParser/ColladaParser.class.hpp"

namespace notrealengine
{
	enum cpTextureType
	{
		diffuse,
		specular,
		normal
	};
	struct cpTexture
	{
		unsigned int	mHeight;
	};
	struct cpMaterial
	{
		std::string mName;
		unsigned int mNumTextures;

		unsigned int
			GetTextureCount(cpTextureType type);
		void
			GetTexture(cpTextureType type, unsigned int index, std::string& str);
	};
	struct cpNodeAnim
	{
		std::string mNodeName;
	};
	struct cpAnimation
	{
		cpNodeAnim**	mChannels;
		unsigned int	mNumChannels;

		cpAnimation(): mChannels(nullptr), mNumChannels(0)
		{}

	};
	struct cpVertexWeight
	{
		unsigned int mVertexId;
		float mWeight;
	};
	struct cpBone
	{
		std::string mName;

		cpVertexWeight* mWeights;
		std::string	mNodeName;
		unsigned int	mNumWeights;

		mft::mat4 mOffsetMatrix;

		cpBone(): mName(""), mWeights(nullptr), mNodeName(""), mNumWeights(0)
		{}
	};
	struct cpFace
	{
		unsigned int* mIndices;
		unsigned int  mNumIndices;

		cpFace(): mIndices(nullptr), mNumIndices(0)
		{}

	};
	struct cpMesh
	{
		mft::vec3* mVertices;
		mft::vec3* mNormals;
		mft::vec3* mTextureCoords[MAX_TEXTURE_COORDINATES];
		unsigned int mNumUVComponents[MAX_TEXTURE_COORDINATES];
		mft::vec4* mColors[MAX_TEXTURE_COORDINATES];

		cpBone** mBones;
		cpFace*	mFaces;

		std::string		mName;

		unsigned int	mNumVertices;
		unsigned int	mNumFaces;
		unsigned int	mMaterialIndex;
		unsigned int	mNumBones;

		cpMesh(): mVertices(nullptr), mNormals(nullptr), mBones(nullptr), mFaces(nullptr),
			mNumBones(0), mNumFaces(0), mMaterialIndex(0), mNumVertices(0)
		{
			for (size_t i = 0; i < MAX_TEXTURE_COORDINATES; i++)
			{
				this->mTextureCoords[i] = nullptr;
				this->mNumUVComponents[i] = 0;
				this->mColors[i] = nullptr;
			}
		}

	};
	struct cpNode
	{
		cpNode** mChildren;
		cpNode* mParent;
		unsigned int* mMeshes;

		mft::mat4		mTransformation;

		std::string		mName;

		unsigned int	mNumChildren;
		unsigned int	mNumMeshes;

		cpNode(): mChildren(nullptr), mParent(nullptr), mMeshes(nullptr),
			mTransformation(), mName(""), mNumChildren(0), mNumMeshes()
		{}
	};
	struct cpScene
	{
		cpAnimation** mAnimations;
		cpMesh** mMeshes;
		cpMaterial** mMaterials;
		cpTexture** mTextures;

		cpNode* mRootNode;

		unsigned int	mNumAnimations;
		unsigned int	mNumMeshes;
		unsigned int	mNumMaterials;
		unsigned int	mNumTextures;

		cpScene(): mAnimations(nullptr), mMeshes(nullptr), mMaterials(nullptr),
			mTextures(nullptr), mRootNode(nullptr),
			mNumAnimations(0), mNumMeshes(0), mNumMaterials(0), mNumTextures(0)
		{}

		~cpScene()
		{
			for (unsigned int i = 0; i < mNumAnimations; i++)
			{
				if (this->mAnimations[i] != nullptr)
					deleteAnim(this->mAnimations[i]);
			}
			if (this->mAnimations != nullptr)
				delete [] this->mAnimations;

			for (unsigned int i = 0; i < mNumMeshes; i++)
			{
				if (this->mMeshes[i] != nullptr)
					deleteMesh(this->mMeshes[i]);
			}
			if (this->mMeshes != nullptr)
				delete [] this->mMeshes;

			for (unsigned int i = 0; i < mNumMaterials; i++)
			{
				if (this->mMaterials[i] != nullptr)
					delete this->mMaterials[i];
			}
			if (this->mMaterials != nullptr)
				delete [] this->mMaterials;

			for (unsigned int i = 0; i < mNumTextures; i++)
			{
				if (this->mTextures[i] != nullptr)
					delete this->mTextures[i];
			}
			if (this->mTextures != nullptr)
				delete [] this->mTextures;
			if (this->mRootNode != nullptr)
				deleteNode(this->mRootNode);
		}

	private:
		void deleteBone(cpBone* bone)
		{
			if (bone->mWeights != nullptr)
				delete[] bone->mWeights;
			delete bone;
		}

		void deleteMesh(cpMesh* mesh)
		{
			if (mesh->mVertices != nullptr)
				delete[] mesh->mVertices;
			if (mesh->mNormals != nullptr)
				delete[] mesh->mNormals;
			for (unsigned int i = 0; i < MAX_TEXTURE_COORDINATES; i++)
			{
				if (mesh->mTextureCoords[i] != nullptr)
					delete[] mesh->mTextureCoords[i];
				if (mesh->mColors[i] != nullptr)
					delete[] mesh->mColors[i];
			}
			if (mesh->mFaces != nullptr)
			{
				for (unsigned int i = 0; i < mesh->mNumFaces; i++)
				{
					if (mesh->mFaces[i].mIndices != nullptr)
						delete[] mesh->mFaces[i].mIndices;
				}
				delete[] mesh->mFaces;
			}
			if (mesh->mBones != nullptr)
			{
				for (unsigned int i = 0; i < mesh->mNumBones; i++)
				{
					if (mesh->mBones[i] != nullptr)
						deleteBone(mesh->mBones[i]);
				}
				delete[] mesh->mBones;
			}
			delete mesh;
		}

		void deleteAnim(cpAnimation* anim)
		{
			if (anim->mChannels == nullptr)
				return;
			for (unsigned int i = 0; i < anim->mNumChannels; i++)
			{
				if (anim->mChannels[i] != nullptr)
					delete anim->mChannels[i];
			}
			delete[] anim->mChannels;
			delete anim;
		}

		void deleteNode(cpNode* node)
		{
			if (node->mChildren != nullptr)
			{
				for (unsigned int i = 0; i < node->mNumChildren; i++)
				{
					if (node->mChildren[i] != nullptr)
						deleteNode(node->mChildren[i]);
				}
				delete[] node->mChildren;
			}
			if (node->mMeshes != nullptr)
				delete[] node->mMeshes;
			delete node;
		}
	};
	/**	Build the scene hierarchy with the data
	**	extracted from the collada file
	*/
	class ColladaSceneBuilder
	{
		/**	Meshes references can have many combinations
		**	with their id, their submesh number and
		**	their associated material.
		**	Multiple nodes can refer to the same mesh
		**	so we need to save those ids to avoid
		**	duplication
		*/
		struct MeshID
		{
			std::string	id;
			size_t		subMesh;
			std::string	material;
			MeshID() = delete;
			MeshID(std::string id, size_t subMesh, std::string material)
				: id(id), subMesh(subMesh), material(material)
			{}
			bool	operator==(const MeshID& ref) const
			{
				return (id == ref.id
					&& subMesh == ref.subMesh
					&& material == ref.material);
			}
		};

	public:
		ColladaSceneBuilder();
		~ColladaSceneBuilder();

		/**	Parse the given file and
		**	construct a scene from it
		*/
		cpScene*
			ReadFile(const std::string& path, unsigned int flags);

	private:

		std::vector<cpMesh*> meshes;
		std::vector<cpAnimation*> anims;
		std::vector<cpTexture*> textures;
		std::vector<MeshID>	meshIDs;
		std::map<std::string, size_t> matIndices;
		std::vector<std::pair<ColladaParser::ColladaEffect*, cpMaterial*>> materials;

		//	We may fight nodes with no name, use this to assign auto names to them
		unsigned int unamedNodes;

		/**	Build a cpNode recursively
		*/
		cpNode*
			BuildNode(ColladaParser& parser, ColladaParser::ColladaNode* node);

		/**	Build all the cpMeshes for a node
		**	by creating them or using already existing ones
		*/
		void
			BuildMeshes(ColladaParser& parser, ColladaParser::ColladaNode* node,
				cpNode* newNode);

		/**	Create the array of cpMaterial* from the collada data
		*/
		void
			BuildMaterials(ColladaParser& parser, cpScene* scene);

		/**	Create a new cpMesh ptr with all the collada data
		*/
		cpMesh*
			CreateMesh(ColladaParser& parser,
				const ColladaParser::ColladaMesh* src,
				const ColladaParser::SubMesh& subMesh,
				const size_t vertexStart, const size_t faceStart);
	};

}
#endif