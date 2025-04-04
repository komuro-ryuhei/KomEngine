#include "Triangle.h"

void Triangle::Initialize(DirectXCommon* dxCommon, PipelineManager* pipelineManager) {

	dxCommon_ = dxCommon;
	pipelineManager_ = pipelineManager;

	mesh_ = std::make_unique<Mesh>();

	mesh_->CreateVertexResource(dxCommon_, sizeof(Vector4) * 3);
	mesh_->CreateVertexBufferView();
	mesh_->WriteDateForResource();
}

void Triangle::Draw() {

	ComPtr<ID3D12GraphicsCommandList> commandList = dxCommon_->GetCommandList();

	commandList->SetGraphicsRootSignature(pipelineManager_->GetRootSignature());
	commandList->SetPipelineState(pipelineManager_->GetGraphicsPipelineState());
	D3D12_VERTEX_BUFFER_VIEW vbv = mesh_->GetVBV();
	commandList->IASetVertexBuffers(0, 1, &vbv);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->SetGraphicsRootConstantBufferView(0, mesh_->GetMateialResource()->GetGPUVirtualAddress());

	commandList->DrawInstanced(3, 1, 0, 0);
}