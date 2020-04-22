#include "camera.h"

static const glm::vec2 SAMPLE_LOCS_16[16] = {
	glm::vec2(-8.0f, 0.0f) / 8.0f,
	glm::vec2(-6.0f, -4.0f) / 8.0f,
	glm::vec2(-3.0f, -2.0f) / 8.0f,
	glm::vec2(-2.0f, -6.0f) / 8.0f,
	glm::vec2(1.0f, -1.0f) / 8.0f,
	glm::vec2(2.0f, -5.0f) / 8.0f,
	glm::vec2(6.0f, -7.0f) / 8.0f,
	glm::vec2(5.0f, -3.0f) / 8.0f,
	glm::vec2(4.0f, 1.0f) / 8.0f,
	glm::vec2(7.0f, 4.0f) / 8.0f,
	glm::vec2(3.0f, 5.0f) / 8.0f,
	glm::vec2(0.0f, 7.0f) / 8.0f,
	glm::vec2(-1.0f, 3.0f) / 8.0f,
	glm::vec2(-4.0f, 6.0f) / 8.0f,
	glm::vec2(-7.0f, 8.0f) / 8.0f,
	glm::vec2(-5.0f, 2.0f) / 8.0f };

void Camera::UpdateAspect(float width, float height)
{
	aspect = width / height;
	timer += 1;
	this->width = width;
	this->height = height;

	uint32_t sampleIndex = timer % 16;
	const glm::vec2 TexSize = glm::vec2(1.0f / width, 1.0f / height);
	const glm::vec2 SubsampleSize = TexSize * 2.0f;
	const glm::vec2 S = SAMPLE_LOCS_16[sampleIndex]; // In [-1, 1]

	glm::vec2 Subsample = S * SubsampleSize;
	Subsample *= 0.0f;

	jitterMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(Subsample.x, Subsample.y, 0.0f));
}