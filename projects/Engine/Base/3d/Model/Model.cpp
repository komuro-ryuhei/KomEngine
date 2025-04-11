#include "Model.h"

void Model::Init(DirectXCommon* dxCommon, const std::string& directoryPath, const std::string& filename) {

	dxCommon_ = dxCommon;

	modelData = LoadObjFile(std::move(directoryPath), std::move(filename));

	vertexResource = dxCommon_->CreateBufferResource(dxCommon_->GetDevice(), sizeof(VertexData) * modelData.vertices.size());
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());

	materialResource = dxCommon_->CreateBufferResource(dxCommon_->GetDevice(), sizeof(Material));
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData->enableLighting = false;
	materialData->uvTransform = MyMath::MakeIdentity4x4();
	materialData->shininess = 48.3f;

	TextureManager::GetInstance()->LoadTexture(std::move(modelData.material.textureFilePath));
	modelData.material.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(modelData.material.textureFilePath);
}

void Model::Draw() {

	ComPtr<ID3D12GraphicsCommandList> commandList = dxCommon_->GetCommandList();

	commandList->IASetVertexBuffers(0, 1, &vertexBufferView); // VBVを設定

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());

	commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(modelData.material.textureFilePath));

	// Modelの描画
	commandList->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);
}

MaterialData Model::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {

	// 構築するMatrialData
	MaterialData materialData;
	// ファイルから選んだ1行を格納するもの
	std::string line;
	// ファイルを開く
	std::ifstream file(directoryPath + "/" + filename);
	// 開けなかったら止める
	assert(file.is_open());

	while (std::getline(file, line)) {
		std::string identifer;
		std::istringstream s(line);
		s >> identifer;

		// identiferに応じた処理
		if (identifer == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			// 連結してファイルパスにする
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}
	return materialData;
}

ModelData Model::LoadObjFile(const std::string& directoryPath, const std::string& filename) {

	ModelData modelData;            // 構築するモデルデータ
	std::vector<Vector4> positions; // 位置
	std::vector<Vector3> normals;   // 法線
	std::vector<Vector2> texcoords; // テクスチャ座標
	std::string line;               // ファイルから読んだ1行を格納するもの

	std::ifstream file(directoryPath + "/" + filename); // ファイルを開く
	assert(file.is_open());

	// ファイルを読み、ModelDataを構築
	while (std::getline(file, line)) {
		std::string identifer;
		std::istringstream s(line);
		s >> identifer; // 先頭の識別子を読む

		// identiferに応じた処理
		if (identifer == "v") {
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.x *= -1;
			position.w = 1.0f;
			positions.push_back(position);
		} else if (identifer == "vt") {
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			texcoord.y = 1.0f - texcoord.y;
			texcoords.push_back(texcoord);
		} else if (identifer == "vn") {
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			normal.x *= -1;
			normals.push_back(normal);
		} else if (identifer == "f") {
			// 面は三角形限定
			VertexData triangle[3];
			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
				std::string vertexDefinition;
				s >> vertexDefinition;
				// 頂点の要素へのIndexは「位置/UV/法線」で格納されているので、分解してIndexを取得する
				std::istringstream v(vertexDefinition);
				uint32_t elementIndices[3];
				for (uint32_t element = 0; element < 3; ++element) {
					std::string index;
					std::getline(v, index, '/'); // 区切りでインデックスを読んでいく
					elementIndices[element] = std::stoi(index);
				}
				// 要素へのIndexから、実際の要素の値を取得して、頂点を構築する
				Vector4 position = positions[elementIndices[0] - 1];
				Vector2 texcoord = texcoords[elementIndices[1] - 1];
				Vector3 normal = normals[elementIndices[2] - 1];
				VertexData vertex = {position, texcoord, normal};
				modelData.vertices.push_back(vertex);
				triangle[faceVertex] = {position, texcoord, normal};
			}
			// 頂点を逆順で登録することで周り順を逆にする
			modelData.vertices.push_back(triangle[2]);
			modelData.vertices.push_back(triangle[1]);
			modelData.vertices.push_back(triangle[0]);
		} else if (identifer == "mtllib") {
			// materialTempleteLibraryファイルの名前を取得する
			std::string materialFilename;
			s >> materialFilename;
			// 基本的にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
		}
	}
	return modelData;
}