#ifndef _WORLD_H_
#define _WORLD_H_

#include "Renderable.h"
#include "Terrain.h"
#include "Cube.h"
#include "Cannon.h"
#include "ForceShield.h"
#include "Skybox.h"
#include "EnemyShip.h"
#include "Missile.h"
#include "ParticleEffect.h"

#include <memory>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

/*
Class representing game world
*/
class World
{
public:
	// Shader programs
	ShaderProgram shaderProgram, skyboxShaderProgram, terrainShaderProgram,
		textShaderProgram, screenShaderProgram, colorShaderProgram;

	// Spheres inside which lies a point, which will be the center of circural enemy ship movement
	std::vector<BoundingSphere> enemyShipDestinationSpheres;
	glm::vec3 enemyShipSpawnPoint;

	// Object pools
	static const GLuint ENEMY_SHIP_POOL_SIZE = 20;
	std::vector<std::unique_ptr<EnemyShip>> enemyShips;

	static const GLuint CANNON_MISSILES_POOL_SIZE = 15;
	std::vector<std::unique_ptr<Missile>> cannonMissiles;

	static const GLuint ENEMY_SHIP_MISSILES_POOL_SIZE = 40;
	std::vector<std::unique_ptr<Missile>> enemyShipMissiles;

	static const GLuint SINGLE_EFFECT_TYPE_POOL_SIZE = 40;
	std::map<std::string, std::vector<std::unique_ptr<ParticleEffect>>> particleEffects;

	// Objects
	std::vector<std::unique_ptr<Renderable>> staticObjects;

	std::unique_ptr<ForceShield> forceShield;
	std::unique_ptr<Cannon> cannon;
	Skybox skybox;
	Terrain terrain;

	// Camera
	Camera camera;

	// Class managing point lights
	LightManager lightManager;

	// Directional light
	glm::vec3 sunDirection;
	glm::vec3 sunLightColor;
	GLfloat sunLightAmbientIntensity;
	glm::vec3 sunRotateAxisX;
	float sunRotateAngleX, sunRotateAngleY, sunSpeed;

	// World status
	GLboolean gamePaused;
	GLboolean gameFinished;

	// Timers
	GLfloat timePassed;
	GLfloat timePassedFromLastShipSpawn;

	GLfloat maxEnemyShipSpawnTime;
	GLfloat minEnemyShipSpawnTime;
	GLfloat enemyShipSpawnTime;

public:
	void init();
	void update(GLfloat deltaTime);
	void release();

	void initCamera(GLFWwindow* window, int viewportWidth, int viewportHeight);
	Camera& getCamera() { return camera; };

	// Collisions
	void checkCollisions();

	void checkMissileBoundaryCollisions(std::vector<std::unique_ptr<Missile>>& missiles);
	void checkMissileEnemyShipCollisions(std::vector<std::unique_ptr<Missile>>& missiles);
	void checkMissileForceShieldCollisions(std::vector<std::unique_ptr<Missile>>& missiles);
	void checkMissileTerrainCollisions(std::vector<std::unique_ptr<Missile>>& missiles);

	bool missileCollidesWithObject(Missile* missile, Renderable* object);

	// Game functions
	void spawnEnemyShip();
	void spawnParticleEffect(std::string effectName, glm::vec3 position);
	void shootCannonMissile();

	void reset();
};

#endif