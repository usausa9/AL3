#include "GameScene.h"
#include "TextureManager.h"
#include <cassert>
#include <random>

using namespace DirectX;

GameScene::GameScene() {}

GameScene::~GameScene() { 
	delete model_; 
}

void GameScene::Initialize() {

	dxCommon_ = DirectXCommon::GetInstance();
	input_ = Input::GetInstance();
	audio_ = Audio::GetInstance();
	debugText_ = DebugText::GetInstance();

	// テクスチャの読み込み
	textureHandle_ = TextureManager::Load("mario.jpg");

	// 3Dモデルの生成
	model_ = Model::Create();

	// ワールドトランスフォームの初期化

	constObject[0].translation_ = {0, 0, 7.0f};
	constObject[1].translation_ = {7.0f, 0, 0};
	constObject[2].translation_ = {-7.0f, 0, 0};
	constObject[3].translation_ = {0, 7.0f, 0};

	for (int i = 0; i < constObjectCount; i++) {
		constObject[i].Initialize();
	}

	worldTransform_[PartId::Center].Initialize();

	worldTransform_[PartId::Left].translation_ = {-2.0f, 0, 0};
	worldTransform_[PartId::Left].parent_ = &worldTransform_[PartId::Center];
	worldTransform_[PartId::Left].Initialize();

	worldTransform_[PartId::Right].translation_ = {2.0f, 0, 0};
	worldTransform_[PartId::Right].parent_ = &worldTransform_[PartId::Center];
	worldTransform_[PartId::Right].Initialize();

	worldTransform_[PartId::Head].translation_ = {0, 0, -2.0f};
	worldTransform_[PartId::Head].parent_ = &worldTransform_[PartId::Center];
	worldTransform_[PartId::Head].Initialize();

	// ビュープロジェクション
	viewProjection_.Initialize();
}

void GameScene::Update() {
	// キャラクター移動処理
	{
		if (input_->PushKey(DIK_LEFT)) {
			worldTransform_[PartId::Center].rotation_.y -= 0.1f;
		}
		if (input_->PushKey(DIK_RIGHT)) {
			worldTransform_[PartId::Center].rotation_.y += 0.1f;
		}

		// 行列の再計算
		worldTransform_[PartId::Center].UpdateMatrix();

		// デバッグ用表示
		debugText_->SetPos(50, 50);
		debugText_->Printf(
		  "eye:(%f,%f,%f)", viewProjection_.eye.x, viewProjection_.eye.y, viewProjection_.eye.z);
	}

	// Y軸 回転計算
	XMFLOAT3 frontVec(0, 0, 1);
	XMFLOAT3 resultVec(0, 0, 0);
	resultVec.x =  cosf(worldTransform_[PartId::Center].rotation_.y) * frontVec.x + sinf(worldTransform_[PartId::Center].rotation_.y) * frontVec.z;
	resultVec.z = -sinf(worldTransform_[PartId::Center].rotation_.y) * frontVec.x + cosf(worldTransform_[PartId::Center].rotation_.y) * frontVec.z;


	{
		float kMoveSpeed = 0.2f;

		// 押した方向で移動ベクトルを変更
		if (input_->PushKey(DIK_UP)) {
			worldTransform_[PartId::Center].translation_.x -= kMoveSpeed * resultVec.x;
			worldTransform_[PartId::Center].translation_.z -= kMoveSpeed * resultVec.z;
		}  else if (input_->PushKey(DIK_DOWN)) {
			worldTransform_[PartId::Center].translation_.x += kMoveSpeed * resultVec.x;
			worldTransform_[PartId::Center].translation_.z += kMoveSpeed * resultVec.z;
		}

		// 行列の再計算
		worldTransform_[PartId::Center].UpdateMatrix();

		// デバッグ用表示
		debugText_->SetPos(50, 150);
		debugText_->Printf(
		"Root:(%f,%f,%f)", worldTransform_[PartId::Center].translation_.x,
		worldTransform_[PartId::Center].translation_.y, 
		worldTransform_[PartId::Center].translation_.z);
	}

	// 行列の再計算
	worldTransform_[PartId::Center].UpdateMatrix();
	worldTransform_[PartId::Left].UpdateMatrix();
	worldTransform_[PartId::Right].UpdateMatrix();
	worldTransform_[PartId::Head].UpdateMatrix();

	// 視点移動処理
	// カメラ
	XMFLOAT3 cameraVec(resultVec.x, 0, resultVec.z);

	// 固定距離
	const float cameraDistance = 35.0f;

	viewProjection_.eye.x = worldTransform_[PartId::Center].translation_.x + cameraDistance * cameraVec.x;
	viewProjection_.eye.y = worldTransform_[PartId::Center].translation_.y + cameraDistance * cameraVec.y;
	viewProjection_.eye.z = worldTransform_[PartId::Center].translation_.z + cameraDistance * cameraVec.z;

	viewProjection_.target = worldTransform_[PartId::Center].translation_;

	viewProjection_.UpdateMatrix();
}

void GameScene::Draw() {

	// コマンドリストの取得
	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

#pragma region 背景スプライト描画
	// 背景スプライト描画前処理
	Sprite::PreDraw(commandList);

	/// <summary>
	/// ここに背景スプライトの描画処理を追加できる
	/// </summary>

	// スプライト描画後処理
	Sprite::PostDraw();
	// 深度バッファクリア
	dxCommon_->ClearDepthBuffer();
#pragma endregion

#pragma region 3Dオブジェクト描画
	// 3Dオブジェクト描画前処理
	Model::PreDraw(commandList);

	/// <summary>
	/// ここに3Dオブジェクトの描画処理を追加できる
	/// </summary>
	// 3Dモデル描画
	model_->Draw(worldTransform_[PartId::Center], viewProjection_, textureHandle_);
	model_->Draw(worldTransform_[PartId::Left], viewProjection_, textureHandle_);
	model_->Draw(worldTransform_[PartId::Right], viewProjection_, textureHandle_);
	model_->Draw(worldTransform_[PartId::Head], viewProjection_, textureHandle_);

	for (int i = 0; i < constObjectCount ; i++) {
		model_->Draw(constObject[i], viewProjection_, textureHandle_);
	}


	// 3Dオブジェクト描画後処理
	Model::PostDraw();
#pragma endregion

#pragma region 前景スプライト描画
	// 前景スプライト描画前処理
	Sprite::PreDraw(commandList);

	/// <summary>
	/// ここに前景スプライトの描画処理を追加できる
	/// </summary>

	// デバッグテキストの描画
	debugText_->DrawAll(commandList);
	//
	// スプライト描画後処理
	Sprite::PostDraw();

#pragma endregion
}
