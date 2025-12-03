#include "LineRenderer.h"
#include "Engine/Base/Camera/Camera.h"

void LineRenderer::Init(uint32_t maxLines, BlendType type)
{
    pipelineManager_ = std::make_unique<PipelineManager>();
    pipelineManager_->PSOSetting("line", type);

    maxVertices_ = maxLines * 2;

    auto* device = System::GetDxCommon()->GetDevice();
    const uint32_t bufferSize = sizeof(LineVertex) * maxVertices_;

    vertexResource_ = System::GetDxCommon()->CreateBufferResource(device, bufferSize);
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.StrideInBytes = sizeof(LineVertex);
    vertexBufferView_.SizeInBytes = bufferSize;

    cameraResource_ = System::GetDxCommon()->CreateBufferResource(
        device, sizeof(LineCameraMatrix));
    cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraData_));

    cameraData_->viewProj = MyMath::MakeIdentity4x4();

    vertexCount_ = 0;
}

void LineRenderer::SetCamera(Camera* camera)
{
    camera_ = camera;
}

void LineRenderer::Update()
{
    // カメラが設定されていたら viewProj を自動更新
    if (camera_ && cameraData_) {
        Matrix4x4 view = camera_->GetViewMatrix();
        Matrix4x4 proj = camera_->GetProjectionMatrix();
        Matrix4x4 vp = MyMath::Multiply(view, proj);
        cameraData_->viewProj = vp;
    }
}

void LineRenderer::AddLine(const Vector3& start, const Vector3& end, const Vector4& color)
{
    if (vertexCount_ + 2 > maxVertices_) { return; }

    vertexData_[vertexCount_ + 0].position = start;
    vertexData_[vertexCount_ + 0].color = color;

    vertexData_[vertexCount_ + 1].position = end;
    vertexData_[vertexCount_ + 1].color = color;

    vertexCount_ += 2;
}

void LineRenderer::Draw()
{
    if (vertexCount_ == 0) return;

    auto commandList = System::GetDxCommon()->GetCommandList();

    commandList->SetGraphicsRootSignature(pipelineManager_->GetRootSignature());
    commandList->SetPipelineState(pipelineManager_->GetGraphicsPipelineState());

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

    commandList->SetGraphicsRootConstantBufferView(
        0, cameraResource_->GetGPUVirtualAddress());

    commandList->DrawInstanced(vertexCount_, 1, 0, 0);

    Clear();
}

void LineRenderer::Clear()
{
    vertexCount_ = 0;
}