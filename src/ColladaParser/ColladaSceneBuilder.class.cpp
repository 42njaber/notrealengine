#include "ColladaParser/ColladaSceneBuilder.class.hpp"
#include "ColladaParser/ColladaException.class.hpp"

#include <limits>

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

		SaveNodeAsVector(scene->mRootNode);

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

		BuildAnimations(parser, scene);

		return scene;
	}

	void ColladaSceneBuilder::SaveNodeAsVector(cpNode* node)
	{
		this->nodes.push_back(node);
		for (unsigned int i = 0;i < node->mNumChildren; i++)
		{
			SaveNodeAsVector(node->mChildren[i]);
		}
	}

	void ColladaSceneBuilder::CreateAnimation(const ColladaParser& parser,
		cpScene* scene, const ColladaParser::ColladaAnimation& anim,
		const std::string& name)
	{
		std::cout << "Creating animation " << name << std::endl;
		for (const auto& node: this->nodes)
		{
			std::cout << "cpNode name = " << node->mName << std::endl;
			const ColladaParser::ColladaNode *colladaNode = FindNode(parser.rootNode,
				node->mName);
			if (colladaNode == nullptr)
			{
				continue;
			}

			//	For each ColladaNode/cpNode duet, check if the channels of anim
			//	affect them

			std::cout << "ColladaNode name = " << colladaNode->name << std::endl;


			std::vector<Channel> channels;

			for (const auto& channel : anim.channels)
			{
				Channel newChannel;

				std::cout << "Checking channel of target " << channel.target << std::endl;
				newChannel.srcChannel = &channel;
				std::string target;
				size_t slashPos = channel.target.find('/');
				//	Handle targets of format "Node/transform"
				if (slashPos == std::string::npos)
					continue ;
				target = channel.target.substr(0, slashPos);
				//	What interests us
				if (target != colladaNode->id)
					continue ;
				size_t dotPos = channel.target.find('.');
				if (dotPos == std::string::npos)
				{
					newChannel.transformId = channel.target.substr(slashPos + 1);
				}
				else
				{
					newChannel.transformId = channel.target.substr(slashPos + 1, dotPos - slashPos - 1);
					std::cerr << "Warning: anim " << anim.name << " channel has sub elements";
					std::cerr << " but we don't handle it yet :)" << std::endl;
				}
				size_t paranthesisPos = channel.target.find('(');
				if (paranthesisPos != std::string::npos)
				{
					std::cerr << "Warning: anim " << anim.name << " channel has brackets";
					std::cerr << " but we don't handle it yet :)" << std::endl;
				}

				std::cout << "Transform id = '" << newChannel.transformId << "'" << std::endl;

				//	Search for a corresponding transform in the current channel
				//	- skip if not found
				newChannel.transformIndex = -1;
				for (size_t i = 0; i < colladaNode->transforms.size(); i++)
				{
					std::cout << "Comparing with transform[" << i << "] of id '";
					std::cout << colladaNode->transforms[i].id << "'" << std::endl;
					if (colladaNode->transforms[i].id == newChannel.transformId)
					{
						newChannel.transformIndex = i;
						break ;
					}
				}
				if (newChannel.transformIndex == -1)
					continue ;
				std::cout << "Channel transform index is " << newChannel.transformIndex << std::endl;
				channels.push_back(newChannel);
			}
			if (channels.empty())
				continue ;

			//	Now we retrieve every transform for every timestamp in every channel
			//	affecting the current node, so we need to iterate through each channel
			//	to find the closest next timestamp
			//	But first, let's find the minimum and maximum timestamps

			float startTime = std::numeric_limits<float>::max();
			float endTime = std::numeric_limits<float>::min();
			for (auto& channel: channels)
			{
				channel.timesAcc = &ResolveReference(parser.accessors,
					channel.srcChannel->timesSource, "channel" + channel.srcChannel->id
					+ "'s times accessors");
				channel.times = &ResolveReference(parser.floats,
					channel.timesAcc->sourceId, "channel" + channel.srcChannel->id
					+ "'s times array");
				channel.valuesAcc = &ResolveReference(parser.accessors,
					channel.srcChannel->valuesSource, "channel" + channel.srcChannel->id
					+ "'s values accessors");
				channel.values = &ResolveReference(parser.floats,
					channel.valuesAcc->sourceId, "channel" + channel.srcChannel->id
					+ "'s values array");

				//	Only check accessor's count and not array's sizes:
				//	if values are matrix there are 16x more values
				//	than timestamps
				if (channel.timesAcc->count != channel.valuesAcc->count)
					throw ColladaException("Different number of times and values in \
					animation " + channel.srcChannel->id);
				//	Find minimum and maximum times for all the channels affecting this nodes
				startTime = std::min(startTime,
					ReadFloat(*channel.times, *channel.timesAcc, 0, 0));
				endTime = std::max(endTime,
					ReadFloat(*channel.times, *channel.timesAcc, channel.times->size() - 1, 0));
			}
			float	time = startTime;
			while (time != std::numeric_limits<float>::max())
			{
				std::cout << "Time = " << time << std::endl;
				//	Find the closest next timestamp
				float nextTime = std::numeric_limits<float>::max();
				for (const auto& channel: channels)
				{
					for (size_t i = 0; i < channel.times->size(); i++)
					{
						float t = ReadFloat(*channel.times, *channel.timesAcc, i, 0);
						if (t > time)
						{
							//	Take the min with the time of the other channels
							nextTime = std::min(t, nextTime);
							break ;
						}
					}
				}
				time = nextTime;
			}
		}
	}

	void ColladaSceneBuilder::BuildAnimation(const ColladaParser& parser,
		cpScene* scene, const ColladaParser::ColladaAnimation& anim,
		const std::string& prefix)
	{
		//	Keep consistency with assimp data format
		//	-> name sub animations with their parent's name as prefix
		std::string name = prefix.empty() ? anim.name : prefix + "_" + anim.name;
		for (const auto& child: anim.children)
		{
			BuildAnimation(parser, scene, child, name);
		}
		if (!anim.channels.empty())
			CreateAnimation(parser, scene, anim, name);
	}

	void ColladaSceneBuilder::BuildAnimations(const ColladaParser& parser,
		cpScene* scene)
	{
		for (const auto& anim: parser.animations)
		{
			BuildAnimation(parser, scene, anim.second, "");
		}
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
			newNode->mTransformation *= transform.matrix;
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

	void	ColladaSceneBuilder::BuildMeshes(const ColladaParser& parser,
		const ColladaParser::ColladaNode* node, cpNode* newNode)
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

	cpMesh* ColladaSceneBuilder::CreateMesh(const ColladaParser& parser,
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

			const ColladaParser::ColladaAccessor& weightsAcc =
				ResolveReference(parser.accessors, controller->weightInput.id,
					"vertex weights accessor");
			const std::vector<float>& weights =
				ResolveReference(parser.floats, weightsAcc.sourceId,
					"vertex weights");

			if (controller->boneInput.offset != 0 || controller->weightInput.offset != 1)
				throw ColladaException("Invalid bone format: expected bone id at offset 0 \
					and weight value at offset 1, got bone id offset = "
					+ std::to_string(controller->boneInput.offset)
					+ " weight value offset = "
					+ std::to_string(controller->weightInput.offset));

			std::vector<std::vector<cpVertexWeight>> bones(boneNames.size());

			//	Save the start index of each weight by adding the weight count
			std::vector<size_t> weightIndices(controller->weightCounts.size());
			size_t totalIndex = 0;
			for (size_t i = 0; i < controller->weightCounts.size(); i++)
			{
				weightIndices[i] = totalIndex;
				totalIndex += controller->weightCounts[i];
			}

			for (size_t i = vertexStart; i < vertexStart + res->mNumVertices; i++)
			{
				//	Bone weights are given in the same order as the vertex positions array
				//	i.e. if vertex[i] pos was at index X, its weight is at index X
				size_t vertexPosIndex = src->posIndices[i];
				size_t nbWeights = controller->weightCounts[vertexPosIndex];
				size_t index = weightIndices[vertexPosIndex];
				for (size_t j = 0; j < nbWeights; j++)
				{
					size_t boneIndex = controller->weights[index + j].first;
					if (boneIndex >= boneNames.size())
						throw ColladaException("Out of bounds bone name index: "
						+ std::to_string(boneIndex) + "/" + std::to_string(boneNames.size()));
					size_t weightIndex = controller->weights[index + j].second;
					if (weightIndex >= weights.size())
						throw ColladaException("Out of bounds bone weight index: "
						+ std::to_string(weightIndex) + "/" + std::to_string(weights.size()));
					float weight = ReadFloat(weights, weightsAcc, weightIndex, 0);
					//	Apparently files can contain weights of 0.. ignore them
					//if (weight > 0.0f)
					//{
						cpVertexWeight newCpWeight;
						//	Reset index count for each submesh;
						newCpWeight.mVertexId = static_cast<unsigned int>(i - vertexStart);
						newCpWeight.mWeight = weight;
						bones[boneIndex].push_back(newCpWeight);
					//}
				}
			}
			//	Count bones that are used for this subMesh
			res->mNumBones = 0;
			for (const auto& bone: bones)
			{
				if (!bone.empty())
					res->mNumBones++;
			}

			res->mBones = new cpBone * [res->mNumBones];
			for (unsigned int i = 0, boneIndex = 0; i < boneNames.size(); i++)
			{
				if (bones[i].empty())
					continue;
				cpBone* bone = new cpBone;
				bone->mName = ReadString(boneNames, boneNamesAcc, i, 0);
				bone->mNumWeights = bones[i].size();
				bone->mWeights = new cpVertexWeight [bone->mNumWeights];
				for (unsigned int j = 0; j < bone->mNumWeights; j++)
				{
					bone->mWeights[j] = bones[i][j];
				}
				bone->mOffsetMatrix = mft::mat4(
					{ ReadFloat(boneMatrices, bonesMatrixAcc, i, 0),
						ReadFloat(boneMatrices, bonesMatrixAcc, i, 1),
						ReadFloat(boneMatrices, bonesMatrixAcc, i, 2),
						ReadFloat(boneMatrices, bonesMatrixAcc, i, 3) },
					{ ReadFloat(boneMatrices, bonesMatrixAcc, i, 4),
						ReadFloat(boneMatrices, bonesMatrixAcc, i, 5),
						ReadFloat(boneMatrices, bonesMatrixAcc, i, 6),
						ReadFloat(boneMatrices, bonesMatrixAcc, i, 7) },
					{ ReadFloat(boneMatrices, bonesMatrixAcc, i, 8),
						ReadFloat(boneMatrices, bonesMatrixAcc, i, 9),
						ReadFloat(boneMatrices, bonesMatrixAcc, i, 10),
						ReadFloat(boneMatrices, bonesMatrixAcc, i, 11) },
					{ ReadFloat(boneMatrices, bonesMatrixAcc, i, 12),
						ReadFloat(boneMatrices, bonesMatrixAcc, i, 13),
						ReadFloat(boneMatrices, bonesMatrixAcc, i, 14),
						ReadFloat(boneMatrices, bonesMatrixAcc, i, 15) });
				bone->mOffsetMatrix *= controller->bindShapeMatrix;
				res->mBones[boneIndex++] = bone;
			}
		}

		return res;
	}

	cpMaterial* ColladaSceneBuilder::CreateMaterial(const ColladaParser& parser,
		const ColladaParser::ColladaEffect& effect)
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

	float	ColladaSceneBuilder::ReadFloat(const std::vector<float>& array,
		const ColladaParser::ColladaAccessor& acc, size_t index, size_t offset)
	{
		size_t realIndex = acc.stride * index + acc.offset + offset;
		if (realIndex >= array.size())
			throw ColladaException("Out of bound index " + std::to_string(realIndex)
				+ "/" + std::to_string(array.size()) + " of array " + acc.sourceId);
			return array[realIndex];
	}

	std::string	ColladaSceneBuilder::ReadString(const std::vector<std::string>& array,
		const ColladaParser::ColladaAccessor& acc, size_t index, size_t offset)
	{
		size_t realIndex = acc.stride * index + acc.offset + offset;
		if (realIndex >= array.size())
			throw ColladaException("Out of bound index " + std::to_string(realIndex)
				+ "/" + std::to_string(array.size()) + " of array " + acc.sourceId);
		return array[realIndex];
	}
}
