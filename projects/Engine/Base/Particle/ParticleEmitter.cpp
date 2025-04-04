#include "ParticleEmitter.h"

void ParticleEmitter::Init(const std::string& name, Vector3 translate, uint32_t count) {

	name_ = name;
	translate_ = translate;
	count_ = count;
}

void ParticleEmitter::Update() {

	frequencyTime += kDeltaTime;
	if (frequency <= frequencyTime) {
		Emit();
		frequencyTime -= frequency;
	}
}

void ParticleEmitter::Emit() { 
	
	ParticleManager::GetInstance()->Emit(name_, translate_, count_);
}