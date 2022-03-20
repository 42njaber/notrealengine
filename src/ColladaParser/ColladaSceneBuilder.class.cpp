#include "ColladaParser/ColladaSceneBuilder.class.hpp"
#include "ColladaParser/ColladaException.class.hpp"

namespace notrealengine
{
	ColladaSceneBuilder::ColladaSceneBuilder():
		meshes(), anims(), textures(), meshIDs(), matIndices(), materials(),
		unamedNodes(0)
	{
	}

	ColladaSceneBuilder::~ColladaSceneBuilder()
	{
	}

	cpScene* ColladaSceneBuilder::ReadFile(const std::string& path, unsigned int flags)
	{
		cpScene* scene = nullptr;

		ColladaParser	parser;
		parser.ReadFile(path, flags);

		if (parser.rootNode == nullptr)
		{
			throw ColladaException("Empty collada file");
		}
		scene = new cpScene;

		BuildMaterials(parser, scene);
		scene->mRootNode = BuildNode(parser, parser.rootNode);

		if (parser.axis == ColladaParser::X_UP)
		{
			scene->mRootNode->mTransformation *=
				mft::mat4(
					{ 0, -1, 0, 0 },
					{ 1, 0, 0, 0 },
					{ 0, 0, 1, 0 },
					{ 0, 0, 0, 1 });
		}
		else if (parser.axis == ColladaParser::Z_UP)
		{
			scene->mRootNode->mTransformation *=
				mft::mat4(
					{ 1, 0, 0, 0 },
					{ 0, 0, 1, 0 },
					{ 0, -1, 0, 0 },
					{ 0, 0, 0, 1 });
		}

		scene->mNumMeshes = static_cast<unsigned int>(this->meshes.size());
		scene->mMeshes = new cpMesh * [scene->mNumMeshes];
		std::copy(this->meshes.begin(), this->meshes.end(), scene->mMeshes);

		scene->mNumMaterials = static_cast<unsigned int>(this->materials.size());
		scene->mMaterials = new cpMaterial * [scene->mNumMaterials];
		for (unsigned int i = 0; i < scene->mNumMaterials; i++)
		{
			scene->mMaterials[i] = this->materials[i];
		}

		scene->mNumTextures = static_cast<unsigned int>(this->textures.size());
		scene->mTextures = new cpTexture * [scene->mNumTextures];
		std::copy(this->textures.begin(), this->textures.end(), scene->mTextures);

		return scene;
	}

	cpNode* ColladaSceneBuilder::BuildNode(ColladaParser& parser,
		ColladaParser::ColladaNode* node)
	{
		cpNode* newNode = new cpNode();

		if (!node->id.empty())
			newNode->mName = node->id;
		else if (!node->sid.empty())
			newNode->mName = node->sid;
		else
			newNode->mName = "Node_" + std::to_string(this->unamedNodes++);

		newNode->mTransformation = mft::mat4();
		for (const auto& transform : node->transforms)
		{
			newNode->mTransformation *= transform;
		}

		std::vector<ColladaParser::ColladaNode*> instances;
		size_t nbInstances = 0;
		for (const auto& child : node->nodeInstances)
		{
			const std::map<std::string, ColladaParser::ColladaNode>::iterator it =
				parser.nodes.find(child);
			if (it == parser.nodes.end())
			{
				std::cerr << "Unable to resolve node instance " << child << std::endl;
				continue;
			}
			instances.push_back(&it->second);
			nbInstances++;
		}

		newNode->mNumChildren = static_cast<unsigned int>(node->children.size() + nbInstances);
		newNode->mChildren = new cpNode * [newNode->mNumChildren];
		for (size_t i = 0; i < node->children.size(); i++)
		{
			newNode->mChildren[i] = BuildNode(parser, &node->children[i]);
			newNode->mChildren[i]->mParent = newNode;
		}

		for (size_t i = 0; i < nbInstances; i++)
		{
			newNode->mChildren[newNode->mNumChildren + i] = BuildNode(parser, instances[i]);
			newNode->mChildren[newNode->mNumChildren + i]->mParent = newNode;
		}

		BuildMeshes(parser, node, newNode);

		return newNode;
	}

	void	ColladaSceneBuilder::BuildMeshes(ColladaParser& parser,
		ColladaParser::ColladaNode* node, cpNode* newNode)
	{
		//	The index of each mesh combination (mesh id, subMesh number, material)
		//	this node will have
		std::vector<size_t>	meshIndices;

		//std::cout << "Building meshes.." << std::endl;

		for (const ColladaParser::ColladaInstance& meshInstance : node->meshes)
		{
			//	Resolve mesh reference
			//	It can be a ColladaMesh or a ColladaController
			const ColladaParser::ColladaMesh* srcMesh = nullptr;
			const ColladaParser::ColladaController* srcController = nullptr;

			std::map<std::string, ColladaParser::ColladaMesh*>::const_iterator
				meshIt = parser.meshes.find(meshInstance.id);
			if (meshIt == parser.meshes.end())
			{
				//	If it's not from the mesh map, it's probably a controller
				srcController = &ResolveReference(parser.controllers, meshInstance.id,
					"controller instance");
				srcMesh = ResolveReference(parser.meshes, srcController->meshId,
					"controller's mesh");;
			}
			else
				srcMesh = meshIt->second;

			size_t vertexStart = 0;
			size_t faceStart = 0;
			size_t nbSubMeshes = srcMesh->subMeshes.size();
			//	<mesh> tags may have multiple submeshes,
			//	one for every <triangle>/<polylist>/<lines> declaration
			for (size_t subMeshIndex = 0; subMeshIndex < nbSubMeshes; subMeshIndex++)
			{
				const ColladaParser::SubMesh& subMesh = srcMesh->subMeshes[subMeshIndex];
				if (subMesh.nbFaces == 0)
					continue;

				/*
				//	Resolve material reference
				//	Nodes in <library_visual_scenes> give material instances
				//	but also the <triangles>/<vertices>/etc submeshes in 
				//	<library_geometry>
				const std::map<std::string, ColladaParser::MaterialInstance>::const_iterator
					mat = mesh.materials.find(subMesh.material);
				const ColladaParser::MaterialInstance* matInstance = nullptr;
				std::string meshMat = "";
				if (mat == mesh.materials.end())
				{
					std::cerr << "Unknown submesh of " << mesh.id;
					std::cerr << " material reference : " << subMesh.material << std::endl;
					//	Don't skip, create the mesh with no material associated
				}
				else
				{
					matInstance = &mat->second;
					meshMat = matInstance->name;
				}
				std::cout << "Mesh " << mesh.id << " has material " << meshMat << std::endl;
				//	Then search for it in our materials map
				std::map<std::string, size_t>::const_iterator matIt =
					this->matIndices.find(meshMat);
				size_t matIndex = 0;
				if (matIt != this->matIndices.end())
					matIndex = matIt->second;*/

				//	Check if we already created a mesh combination
				//	with this mesh id, subMesh number and material id
				MeshID id(meshInstance.id, subMeshIndex, subMesh.material);
				bool found = false;
				for (size_t meshId = 0; meshId < this->meshIDs.size(); meshId++)
				{
					if (this->meshIDs[meshId] == id)
					{
						found = true;
						//	Add this already existing mesh index to the node
						meshIndices.push_back(meshId);
						break;
					}
				}
				//	If this combination did not exist yet,
				//	create it, save it in the MeshID vector
				//	and in the node
				if (found == false)
				{
					//std::cout << "Creating mesh for node " << node->name << std::endl;
					cpMesh* newMesh = CreateMesh(parser, srcMesh, subMesh,
						srcController, vertexStart, faceStart);

					vertexStart += newMesh->mNumVertices;
					faceStart += newMesh->mNumFaces;

					//	Add this new mesh index to the node
					meshIndices.push_back(this->meshes.size());
					//	Save this new mesh id, submesh and material combination
					this->meshIDs.push_back(id);
					//	Save this new mesh in the meshes array
					this->meshes.push_back(newMesh);

					std::map<std::string, size_t>::const_iterator matIt =
						this->matIndices.find(subMesh.material);
					//	If we can find the material given in <triangles>/<polylist>/etc
					//	bind it.
					if (matIt != this->matIndices.end())
					{
						newMesh->mMaterialIndex = static_cast<unsigned int>(matIt->second);
					}
				}
			}
		}
		newNode->mNumMeshes = meshIndices.size();
		if (newNode->mNumMeshes > 0)
		{
			newNode->mMeshes = new unsigned int [newNode->mNumMeshes];
			for (size_t i = 0; i < newNode->mNumMeshes; i++)
			{
				newNode->mMeshes[i] = meshIndices[i];
			}
		}
	}

	cpMesh* ColladaSceneBuilder::CreateMesh(ColladaParser& parser,
		const ColladaParser::ColladaMesh* src,
		const ColladaParser::SubMesh& subMesh,
		const ColladaParser::ColladaController* controller,
		const size_t vertexStart,const size_t faceStart)
	{
		cpMesh* res = new cpMesh();

		res->mName = src->id;

		if (src->pos.empty())
			return res;

		res->mNumVertices = 0;
		for (size_t i = faceStart; i < faceStart + subMesh.nbFaces; i++)
		{
			res->mNumVertices += src->faceSizes[i];
		}
		//std::cout << "Mesh has " << res->mNumVertices << " vertices" << std::endl;

		//	Positions and normals
		res->mVertices = new mft::vec3[res->mNumVertices]();
		res->mNormals = new mft::vec3[res->mNumVertices]();
		for (size_t i  = 0; i <  res->mNumVertices; i++)
		{
			res->mVertices[i] = src->pos[vertexStart + i];
			//	Apparently there may be cases where
			//	there are more or less normals than positions..
			if (vertexStart + i < src->norm.size())
				res->mNormals[i] = src->norm[vertexStart + i];
		}

		//std::cout << "Vertex start = " << vertexStart << std::endl;
		//	Texture channels
		for (size_t i = 0, currentChannel = 0; i < MAX_TEXTURE_CHANNELS; i++)
		{
			//std::cout << "Channel " << i << " has " << src->tex[i].size() << " textures" << std::endl;
			//std::cout << "Submesh has " << res->mNumVertices << " vertices" << std::endl;
			if (vertexStart + res->mNumVertices <= src->tex[i].size())
			{
				//std::cout << "Channel " << currentChannel << " will have ";
				//std::cout << res->mNumVertices << " textures coord" << std::endl;
				res->mTextureCoords[currentChannel] = new mft::vec3[res->mNumVertices];
				for (size_t j = 0; j < res->mNumVertices; j++)
				{
					res->mTextureCoords[currentChannel][j] = src->tex[i][vertexStart + j];
				}
				//	Only fill and increment the channel if it was valid;
				res->mNumUVComponents[currentChannel] = src->nbUVComponents[i];
				currentChannel++;
			}
		}

		//	Color channels
		for (size_t i = 0, currentChannel = 0; i < MAX_TEXTURE_CHANNELS; i++)
		{
			if (vertexStart + res->mNumVertices < src->colors[i].size())
			{
				res->mColors[currentChannel] = new mft::vec4[res->mNumVertices];
				for (size_t j = 0; j < res->mNumVertices; j++)
				{
					res->mColors[currentChannel][j] = src->colors[i][vertexStart + j];
				}
				//	Only fill and increment the channel if it was valid;
				currentChannel++;
			}
		}

		//	Faces
		res->mNumFaces = subMesh.nbFaces;
		res->mFaces = new cpFace[res->mNumFaces];

		size_t	vertex = 0;
		for (size_t i = 0; i < res->mNumFaces; i++)
		{
			size_t faceSize = src->faceSizes[faceStart + i];
			res->mFaces[i].mNumIndices = faceSize;
			res->mFaces[i].mIndices = new unsigned int[faceSize]();
			for (size_t j = 0; j < faceSize; j++)
			{
				res->mFaces[i].mIndices[j] = static_cast<unsigned int>(vertex++);
			}
		}

		if (controller != nullptr
			&& controller->type == ColladaParser::ControllerType::Skin)
		{
			const ColladaParser::ColladaAccessor& boneNamesAcc =
				ResolveReference(parser.accessors, controller->boneSource,
					"bone names accessor");
			const std::vector<std::string> boneNames =
				ResolveReference(parser.strings, boneNamesAcc.sourceId,
					"bone names source");

			const ColladaParser::ColladaAccessor& bonesMatrixAcc =
				ResolveReference(parser.accessors, controller->boneOffsetMatrixSource,
					"bone offset matrix accessor");
			const std::vector<float> boneMatrices =
				ResolveReference(parser.floats, bonesMatrixAcc.sourceId,
					"bone offset matrix source");

			const ColladaParser::ColladaAccessor& weightBoneNames =
				ResolveReference(parser.accessors, controller->boneInput.id,
					"vertex weights bone names accessor");

			if (&weightBoneNames != &boneNamesAcc)
				throw ColladaException("Bone names source is different in <joints> and <vertex_weights>");
		
			const ColladaParser::ColladaAccessor& weightAcc =
				ResolveReference(parser.accessors, controller->weightInput.id,
					"vertex weights accessor");
			const std::vector<float>& weights =
				ResolveReference(parser.floats, weightAcc.sourceId,
					"vertex weights");

			if (controller->boneInput.offset != 0 || controller->weightInput.offset != 1)
				throw ColladaException("Invalid bone format: expected bone id at offset 0 \
					and weight value at offset 1, got bone id offset = "
					+ std::to_string(controller->boneInput.offset)
					+ " weight value offset = "
					+ std::to_string(controller->weightInput.offset));

			std::vector<std::vector<cpVertexWeight>> dstBones(boneNames.size());

			for (size_t i = vertexStart; i < vertexStart + res->mNumVertices; i++)
			{

			}
		}

		return res;
	}

	cpMaterial* ColladaSceneBuilder::CreateMaterial(ColladaParser& parser, ColladaParser::ColladaEffect& effect)
	{
		cpMaterial* newMat = new cpMaterial();
		for (const auto& pair : effect.params)
		{
			const ColladaParser::EffectParam& param = pair.second;
			if (param.type == ColladaParser::ParamType::SamplerParam)
			{
				const ColladaParser::EffectParam& surface =
					ResolveReference(effect.params, param.ref, "material effect");
				const ColladaParser::ColladaImage& img =
					ResolveReference(parser.images, surface.ref, "effect image");
				cpTexture text;
				text.mName = img.id;
				text.path = img.path;
				text.type = cpTextureType::diffuse;
				newMat->mDiffuse.push_back(text);
				newMat->mNumDiffuses++;
			}
		}
		return newMat;
	}

	void	ColladaSceneBuilder::BuildMaterials(ColladaParser& parser, cpScene* scene)
	{
		for (const auto& pair : parser.materials)
		{
			const ColladaParser::ColladaMaterial& mat = pair.second;
			std::map<std::string, ColladaParser::ColladaEffect>::iterator effectIt =
				parser.effects.find(mat.effectId);
			if (effectIt == parser.effects.end())
				continue;
			ColladaParser::ColladaEffect& effect = effectIt->second;
			cpMaterial* newMat = CreateMaterial(parser, effect);
			newMat->mName = mat.name.empty() ? pair.first : mat.name;
			this->matIndices[mat.id] = this->materials.size();
			this->materials.push_back(newMat);
		}
	}
}
