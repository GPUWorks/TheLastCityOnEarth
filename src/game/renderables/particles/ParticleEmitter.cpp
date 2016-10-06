#include "ParticleEmitter.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

extern std::mt19937 rng;

const glm::vec3 ParticleEmitter::GRAVITY = glm::vec3(0.0f, -10.0f, 0.0f);

void ParticleEmitter::init(ParticleEmitterData emitterData)
{
	this->emitterData = emitterData;

	glm::vec3 vertexData[] = {
		glm::vec3(-1.0f, -1.0f, 0.0f),
		glm::vec3(1.0f, -1.0f, 0.0f),
		glm::vec3(-1.0f, 1.0f, 0.0f),
		glm::vec3(1.0f, 1.0f, 0.0f)
	};

	glm::vec2 uvData[] = {
		glm::vec2(0.0f, 0.0f),
		glm::vec2(1.0f, 0.0f),
		glm::vec2(0.0f, 1.0f),
		glm::vec2(1.0f, 1.0f)
	};

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &vertexVBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &positionBuffer);	// Positions and sizes of particles
	glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
	glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &colorBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
	glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &uvBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(uvData), uvData, GL_STATIC_DRAW);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &rotationBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, rotationBuffer);
	glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(glm::mat3), NULL, GL_STREAM_DRAW);
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(glm::mat3), (void*)(sizeof(GLfloat) * 0));
	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(glm::mat3), (void*)(sizeof(GLfloat) * 3));
	glEnableVertexAttribArray(6);
	glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(glm::mat3), (void*)(sizeof(GLfloat) * 6));

	particleTexture.loadTexture2D("data/particles/" + emitterData.particleTextureName, true);
	particleTexture.setFiltering(TEXTURE_FILTER_MAG_BILINEAR, TEXTURE_FILTER_MIN_BILINEAR_MIPMAP);
	particleTexture.setSamplerParameter(GL_TEXTURE_MAX_ANISOTROPY_EXT, 4);
}

void ParticleEmitter::render(ShaderProgram & shaderProgram)
{
	glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
	glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, particleCount * 4 * sizeof(GLfloat), particlePositionAndSizeData);

	glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
	glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, particleCount * 4 * sizeof(GLfloat), particleColorData);

	glBindBuffer(GL_ARRAY_BUFFER, rotationBuffer);
	glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(glm::mat3), NULL, GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, particleCount * sizeof(glm::mat3), particleRotationData);

	particleTexture.bindTexture();
	shaderProgram.setUniform("particleTexture", 0);

	glBindVertexArray(VAO);
	glVertexAttribDivisor(0, 0);
	glVertexAttribDivisor(1, 1);
	glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 0);
	glVertexAttribDivisor(4, 1);
	glVertexAttribDivisor(5, 1);
	glVertexAttribDivisor(6, 1);

	glEnable(GL_BLEND);
	glDepthMask(0);

	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, particleCount);
	
	glDisable(GL_BLEND);
	glDepthMask(1);
}

void ParticleEmitter::update(float deltaTime, Camera& camera)
{
	// Setting up new particles in this frame (only if effect still active)
	if (generateNewParticles)
	{
		generationTimeAccumulator += deltaTime;
		std::cout << "Emitter time accumulator: " << generationTimeAccumulator << std::endl;
		while (generationTimeAccumulator > 1.0 / emitterData.particlesPerSecond)
		{
			int particleIndex = findUnusedParticle();
			Particle& p = particles[particleIndex];
			initParticle(p);
			generationTimeAccumulator -= 1.0 / emitterData.particlesPerSecond;
		}
	}

	// Updating particles
	particleCount = 0;
	for (int i = 0; i < MAX_PARTICLES; ++i)
	{
		Particle& p = particles[i];

		if (p.isAlive())
		{
			p.life -= deltaTime;

			if (p.isAlive())
			{
				GLfloat particleLifetime = p.initialLife - p.life;

				p.position += p.direction * p.speed * deltaTime;
				p.position += GRAVITY * p.weight * deltaTime;
				p.distanceFromCamera = glm::length(p.position - camera.eye);

				// Update particle variables from timelines
				emitterData.size.update(p.size, particleLifetime);
				emitterData.speed.update(p.speed, particleLifetime);
				emitterData.weight.update(p.weight, particleLifetime);
				emitterData.color.update(p.color, particleLifetime);

				// Updating GPU buffers
				particlePositionAndSizeData[particleCount * 4 + 0] = p.position.x;
				particlePositionAndSizeData[particleCount * 4 + 1] = p.position.y;
				particlePositionAndSizeData[particleCount * 4 + 2] = p.position.z;
				particlePositionAndSizeData[particleCount * 4 + 3] = p.size;

				particleColorData[particleCount * 4 + 0] = p.color.x;
				particleColorData[particleCount * 4 + 1] = p.color.y;
				particleColorData[particleCount * 4 + 2] = p.color.z;
				particleColorData[particleCount * 4 + 3] = p.color.a;

				// UPDATE ROTATION GPU BUFFER
				// get mat3 rotation from particle
				particleRotationData[particleCount * 3 + 0] = glm::vec3(cos(p.angle), -sin(p.angle), 0.0f);
				particleRotationData[particleCount * 3 + 1] = glm::vec3(sin(p.angle), cos(p.angle), 0.0f);
				particleRotationData[particleCount * 3 + 2] = glm::vec3(0.0f, 0.0f, 1.0f);

				++particleCount;
			}
			else
			{
				p.distanceFromCamera = -1.0f;
			}
		}
	}
}

void ParticleEmitter::destroy()
{
}

int ParticleEmitter::findUnusedParticle()
{
	for (int i = lastUsedParticle; i < MAX_PARTICLES; ++i)
	{
		if (particles[i].life < 0)
		{
			lastUsedParticle = i;
			return i;
		}
	}
	for (int i = 0; i < lastUsedParticle; ++i)
	{
		if (particles[i].life < 0)
		{
			lastUsedParticle = i;
			return i;
		}
	}

	return 0;
}

void ParticleEmitter::activate()
{
	generateNewParticles = true;
}

void ParticleEmitter::deactivate()
{
	generateNewParticles = false;
}

GLboolean ParticleEmitter::isActive() const
{
	if (generateNewParticles) return true;
	else return false;
}

GLboolean ParticleEmitter::shouldBeDeactivated(float timePassed) const
{
	if (timePassed > emitterData.lifeTimeInSeconds && generateNewParticles)
	{
		return true;
	}
	else return false;
}

GLuint ParticleEmitter::getParticleCount() const
{
	return particleCount;
}

void ParticleEmitter::initParticle(Particle & p)
{
	p.life = emitterData.initialParticleLife;
	p.initialLife = emitterData.initialParticleLife;
	p.position = emitterData.initialParticlePosition;
	p.speed = emitterData.speed.initialValue;
	p.size = emitterData.size.initialValue;
	p.weight = emitterData.weight.initialValue;
	p.color = emitterData.color.initialValue;
	std::uniform_real_distribution<GLfloat> randomDirectionComponent(-1, 1);
	p.direction.x = randomDirectionComponent(rng);
	p.direction.y = randomDirectionComponent(rng);
	p.direction.z = randomDirectionComponent(rng);
	p.direction = glm::normalize(p.direction);
	std::uniform_real_distribution<GLfloat> randomAngleInRadians(0.0f, 6.28f);
	p.angle = randomAngleInRadians(rng);
}