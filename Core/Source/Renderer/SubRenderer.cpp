#include "SubRenderer.h"

Mupfel::SubRenderer::SubRenderer(uint32_t frames_in_flight) : framesInFlight(frames_in_flight), frameIndex(0) {}

void Mupfel::SubRenderer::IncrementFrameIndex() { frameIndex = (frameIndex + 1) % framesInFlight; }
